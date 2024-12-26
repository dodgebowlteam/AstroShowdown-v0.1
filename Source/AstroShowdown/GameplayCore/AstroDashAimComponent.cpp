/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroDashAimComponent.h"
#include "AbilitySystemInterface.h"
#include "AstroBall.h"
#include "AstroController.h"
#include "AstroTimeDilationSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "SubsystemUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroDashAimComponent)

namespace AstroUtils
{
	namespace Private
	{
		static void CalculateDeflectionDirection(const FVector& DeflectionSourcePosition, AAstroBall* DeflectedBall, FVector& OutDeflectionDirection)
		{
			if (ensure(DeflectedBall))
			{
				const FVector BallPosition = DeflectedBall->GetActorLocation();
				OutDeflectionDirection = (BallPosition - DeflectionSourcePosition);
				OutDeflectionDirection = FVector::VectorPlaneProject(OutDeflectionDirection, { 0.f, 0.f, 1.f });
				OutDeflectionDirection.Normalize();
			}
		}
	}
}

namespace AstroCVars
{
	bool bEnableAimDebugTrace = false;
	static FAutoConsoleVariableRef CVarEnableAimDebugTrace(
		TEXT("AstroDashAim.EnableAimDebugTrace"),
		bEnableAimDebugTrace,
		TEXT("When enabled, will show a debug trace for the aim cursor."),
		ECVF_Default);
}

namespace AstroStatics
{
	static const FName DashingPawnCollisionProfileName = "CustomProfile_DashingPawn";
}

void UAstroDashAimComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AAstroController* PlayerController = Cast<AAstroController>(GetOwner()))
	{
		CachedOwnerController = PlayerController;
	}
}

void UAstroDashAimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (TObjectPtr<UNiagaraComponent> DeflectAimPointerParticleComponent : DeflectAimPointerParticleComponents)
	{
		DeflectAimPointerParticleComponent->ReleaseToPool();
	}
}

void UAstroDashAimComponent::ActivateAim()
{
	if (CachedOwnerCharacterRadius <= 0.f)
	{
		if (ACharacter* OwnerCharacter = CachedOwnerController.IsValid() ? Cast<ACharacter>(CachedOwnerController->GetPawn()) : nullptr)
		{
			const FVector AimStartPosition = OwnerCharacter->GetActorLocation();
			const UCapsuleComponent* OwnerCharacterCapsule = OwnerCharacter->GetCapsuleComponent();
			CachedOwnerCharacterRadius = OwnerCharacterCapsule ? OwnerCharacterCapsule->GetScaledCapsuleRadius() : 0.f;
			CachedOwnerCharacterHalfHeight = OwnerCharacterCapsule ? OwnerCharacterCapsule->GetScaledCapsuleHalfHeight() : 0.f;
		}
	}
	ensure(CachedOwnerCharacterRadius > 0.f && CachedOwnerCharacterHalfHeight > 0.f);

	CreateDashAimDecalCursor();
	CreateDeflectAimDecalCursor();
}

void UAstroDashAimComponent::DeactivateAimPreview()
{
	if (DeflectAimDecalCursor)
	{
		DeflectAimDecalCursor->SetHiddenInGame(true);
	}

	if (DashAimDecalCursor)
	{
		DashAimDecalCursor->SetHiddenInGame(true);
	}

	DisableAllDeflectAimPointers();
}

void UAstroDashAimComponent::UpdateAim()
{
	FVector DashTargetWorldPosition;
	if (const bool bIsDashLocationInvalid = !ValidateDashTargetLocation(DashTargetWorldPosition))
	{
		CurrentAimResult.Reset();
		DeactivateAimPreview();
		return;
	}

	if (!CurrentAimResult.IsSet())
	{
		CurrentAimResult = FAstroDashAimResult();
	}

	AActor* CurrentDashTarget = FindDashTarget(DashTargetWorldPosition);
	if (CurrentDashTarget)
	{
		if (AAstroBall* CurrentTargetBall = Cast<AAstroBall>(CurrentDashTarget))
		{
			TArray<FHitResult> BallHits;
			SimulateBallTrajectory(DashTargetWorldPosition, CurrentTargetBall, /*OUT*/ BallHits);

			const FHitResult& DeflectionTarget = BallHits.IsEmpty() ? FHitResult::FHitResult() : BallHits[0];
			const FHitResult& DeflectionDamageTarget = BallHits.IsEmpty() ? FHitResult::FHitResult() : BallHits.Last();
			if (AAstroBall::CanDamageActor(DeflectionDamageTarget.GetActor()))
			{
				// If aiming at a valid damageable target, we update the current target
				FHitResult NewDeflectionTarget = DeflectionTarget;

				// If aiming directly at a target, we snap the aim to its center
				if (const bool bAimingDirectlyAtTarget = BallHits.Num() == 1)
				{
					NewDeflectionTarget.Location = DeflectionTarget.GetActor()->GetActorLocation();
				}

				// Updates the target only if it has changed or wasn't set yet
				const bool bIsDeflectionTargetEmpty = CurrentAimResult->DeflectionTrajectoryHits.IsEmpty();
				const bool bShouldUpdateTarget = bIsDeflectionTargetEmpty || (DeflectionDamageTarget.GetActor() != CurrentAimResult->DeflectionTrajectoryHits.Last().GetActor());
				if (bShouldUpdateTarget)
				{
					CurrentAimResult->DeflectionTargetHit = NewDeflectionTarget;
					CurrentAimResult->DeflectionTrajectoryHits = BallHits;
					CurrentAimResult->DeflectionTrajectoryHits.Last().Location = DeflectionDamageTarget.GetActor()->GetActorLocation();
					CurrentAimResult->LastValidDashEndPositionWithDeflectionTarget = DashTargetWorldPosition;
				}
			}
			else if (ShouldAimAssistDeflection(DashTargetWorldPosition))
			{
				// If aim assist works, we do nothing and keep the current ball target
			}
			else
			{
				// If both the aiming and aim assist fail, replace the deflection target for whatever non-damageable obj was hit (usually walls)
				CurrentAimResult->DeflectionTargetHit = DeflectionTarget;
				CurrentAimResult->DeflectionTrajectoryHits = BallHits;
			}
		}
	}

	APawn* OwnerPawn = CachedOwnerController.IsValid() ? CachedOwnerController->GetPawn() : nullptr;
	CurrentAimResult->DashStartPosition = OwnerPawn ? OwnerPawn->GetActorLocation() : CurrentAimResult->DashStartPosition;
	CurrentAimResult->DashEndPosition = DashTargetWorldPosition;
	CurrentAimResult->DashTargetActor = CurrentDashTarget;
	CurrentAimResult->bIsValidTrajectory = ValidateDashTrajectory(CurrentAimResult->DashStartPosition, CurrentAimResult->DashEndPosition);

	UpdateAimPreview(DashTargetWorldPosition);
}

AActor* UAstroDashAimComponent::FindDashTarget(const FVector& DashTargetWorldPosition)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Sets up trace debug parameters
	const EDrawDebugTrace::Type DebugTraceType = AstroCVars::bEnableAimDebugTrace ? EDrawDebugTrace::Type::ForOneFrame : EDrawDebugTrace::Type::None;
	constexpr float DebugDuration = 0.f;

	// Sphere traces on the cursor's position to try and find targets
	constexpr bool bTraceComplex = false;
	constexpr bool bIgnoreSelf = true;
	const TArray<AActor*> ActorsToIgnore { };
	TArray<FHitResult> OutCandidateTargets;
	UKismetSystemLibrary::SphereTraceMulti(GetOwner(), DashTargetWorldPosition, DashTargetWorldPosition, DashAimRadius, DashTargetChannel, bTraceComplex, ActorsToIgnore,
		DebugTraceType, OUT OutCandidateTargets, bIgnoreSelf, FLinearColor::Red, FLinearColor::Green, DebugDuration);

	// Finds the nearest target
	AActor* Target = nullptr;
	float NearestTargetDistance = 999999.f;
	for (const FHitResult& CandidateTarget : OutCandidateTargets)
	{
		// Currently, one can only target balls with dashes
		AAstroBall* CandidateTargetActor = Cast<AAstroBall>(CandidateTarget.GetActor());
		if (!CandidateTargetActor)
		{
			continue;
		}

		const float TargetDistance = FVector::Distance(DashTargetWorldPosition, CandidateTargetActor->GetActorLocation());
		if (TargetDistance < NearestTargetDistance)
		{
			Target = CandidateTargetActor;
			NearestTargetDistance = TargetDistance;
		}
	}

	return Target;
}

namespace AstroStatics
{
	static const FName FloorCollisionProfileName = "CustomProfile_LevelFloor";
	static const FName DashBlockTagName = "BlockDash";
}

bool UAstroDashAimComponent::ValidateDashTargetLocation(OUT FVector& DashTargetWorldPosition)
{
	if (!CachedOwnerController.IsValid())
	{
		return false;
	}

	constexpr bool bTraceComplex = false;
	const TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes =
	{
		EObjectTypeQuery::ObjectTypeQuery1,		// WorldStatic (walls, floors)
		EObjectTypeQuery::ObjectTypeQuery8,		// CustomObject_InvisibleRift
	};

	TArray<FHitResult> CursorTraceHits;
	const bool bFoundHit = CachedOwnerController->GetHitResultsUnderCursorForObjects(TraceObjectTypes, bTraceComplex, OUT CursorTraceHits);
	if (!bFoundHit)
	{
		return false;
	}

	for (const FHitResult& CursorTraceHit : CursorTraceHits)
	{
		const UPrimitiveComponent* HitComponent = CursorTraceHit.GetComponent();
		if (CursorTraceHit.bBlockingHit && HitComponent && HitComponent->ComponentHasTag(AstroStatics::DashBlockTagName))
		{
			return false;
		}

		if (CursorTraceHit.bBlockingHit && HitComponent && HitComponent->GetCollisionProfileName() == AstroStatics::FloorCollisionProfileName)
		{
			DashTargetWorldPosition = CursorTraceHit.Location;
			return true;
		}
	}

	return false;
}

bool UAstroDashAimComponent::ValidateDashTrajectory(const FVector& DashStartPosition, const FVector& DashTargetPosition)
{
	if (!CachedOwnerController.IsValid())
	{
		return false;
	}

	// Sets up trace parameters
	const EDrawDebugTrace::Type DebugTraceType = AstroCVars::bEnableAimDebugTrace ? EDrawDebugTrace::Type::ForOneFrame : EDrawDebugTrace::Type::None;
	const float DebugDuration = 0.f;
	constexpr bool bIgnoreSelf = true;
	constexpr bool bTraceComplex = false;
	const TArray<AActor*> ActorsToIgnore = { CachedOwnerController->GetPawn() };
	const FLinearColor TraceColor = FLinearColor::Red;
	const FLinearColor TraceHitColor = FLinearColor::Green;

	// Performs trace to check if the dash would hit anything
	FHitResult TraceResult;
	bool bHitSomething = UKismetSystemLibrary::CapsuleTraceSingleByProfile(this, DashStartPosition, DashTargetPosition,
		CachedOwnerCharacterRadius, CachedOwnerCharacterHalfHeight, AstroStatics::DashingPawnCollisionProfileName, bTraceComplex, ActorsToIgnore,
		DebugTraceType, OUT TraceResult, bTraceComplex, TraceColor, TraceHitColor, DebugDuration);

	return !TraceResult.bBlockingHit;
}

void UAstroDashAimComponent::SimulateBallTrajectory(const FVector& DashTargetWorldPosition, AAstroBall* Ball, OUT TArray<FHitResult>& Hits)
{
	// Inflates the ball a bit, to avoid false positives when aiming through corners.
	constexpr float BallRadiusMultiplier = 1.05f;

	FVector DeflectionDirection;
	AstroUtils::Private::CalculateDeflectionDirection(DashTargetWorldPosition, Ball, DeflectionDirection);
	Ball->SimulateTrajectory(DeflectionDirection, Hits, BallRadiusMultiplier);
}

bool UAstroDashAimComponent::ShouldAimAssistDeflection(const FVector& DashTargetWorldPosition)
{
	if (!CurrentAimResult.IsSet())
	{
		return false;
	}

	if (AAstroBall* DeflectedBall = Cast<AAstroBall>(CurrentAimResult->DashTargetActor))
	{
		if (AActor* DeflectionTargetActor = CurrentAimResult->DeflectionTargetHit.GetActor())
		{
			AActor* DeflectionDamageTargetActor = CurrentAimResult->DeflectionTrajectoryHits.IsEmpty() ? nullptr : CurrentAimResult->DeflectionTrajectoryHits.Last().GetActor();
			if (AAstroBall::CanDamageActor(DeflectionDamageTargetActor))
			{
				FVector LastDeflectionDirectionWithTarget;
				AstroUtils::Private::CalculateDeflectionDirection(CurrentAimResult->LastValidDashEndPositionWithDeflectionTarget, DeflectedBall, /*OUT*/ LastDeflectionDirectionWithTarget);

				FVector NewDeflectionDirection;
				AstroUtils::Private::CalculateDeflectionDirection(DashTargetWorldPosition, DeflectedBall, /*OUT*/ NewDeflectionDirection);

				return FVector::DotProduct(NewDeflectionDirection, LastDeflectionDirectionWithTarget) >= DeflectionAimAssistAngleThreshold;
			}
		}
	}

	return false;
}

void UAstroDashAimComponent::CreateDashAimDecalCursor()
{
	if (!DashAimDecalCursor)
	{
		if (ensure(DashAimDecalCursorMaterial))
		{
			const FVector DecalSize = FVector::OneVector;
			DashAimDecalCursor = UGameplayStatics::SpawnDecalAtLocation(this, DashAimDecalCursorMaterial, DecalSize, {});
			DashAimDecalCursor->FadeScreenSize = 0.f;
			DashAimDecalCursor->SetWorldScale3D(FVector::OneVector);
			DashAimDecalCursor->SetHiddenInGame(true);

			// Sets the decal cursor's width to equal the character's radius
			DashAimDecalCursor->DecalSize.Y = CachedOwnerCharacterRadius;
			DashAimDecalCursor->DecalSize.Z = CachedOwnerCharacterRadius;
		}
	}
}

void UAstroDashAimComponent::CreateDeflectAimDecalCursor()
{
	if (!DeflectAimDecalCursor)
	{
		if (ensure(DeflectAimDecalCursorMaterial))
		{
			const FVector DecalSize = { DashAimRadius, DashAimRadius, DashAimRadius };
			DeflectAimDecalCursor = UGameplayStatics::SpawnDecalAtLocation(this, DeflectAimDecalCursorMaterial, DecalSize, {});
			DeflectAimDecalCursor->FadeScreenSize = 0.f;
			DeflectAimDecalCursor->SetHiddenInGame(true);
		}
	}
}

TObjectPtr<UNiagaraComponent> UAstroDashAimComponent::GetOrCreateDeflectAimPointer(int32 AimPointerIndex)
{
	constexpr int32 MaxAimPointerCount = 8;
	ensure(AimPointerIndex < MaxAimPointerCount);

	if (!DeflectAimPointerParticleComponents.IsValidIndex(AimPointerIndex))
	{
		DeflectAimPointerParticleComponents.SetNumZeroed(AimPointerIndex + 1);

		FFXSystemSpawnParameters SpawnParams;
		SpawnParams.WorldContextObject = this;
		SpawnParams.SystemTemplate = DeflectAimPointerParticle;
		SpawnParams.bAutoActivate = false;
		SpawnParams.bAutoDestroy = false;
		SpawnParams.PoolingMethod = EPSCPoolMethod::ManualRelease;
		SpawnParams.bPreCullCheck = false;

		if (DeflectAimPointerParticleComponents[AimPointerIndex] = UNiagaraFunctionLibrary::SpawnSystemAtLocationWithParams(SpawnParams))
		{
			DeflectAimPointerParticleComponents[AimPointerIndex]->SetRenderCustomDepth(true);
		}
		ensureMsgf(DeflectAimPointerParticleComponents[AimPointerIndex], TEXT("Failed to create"));

		if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
		{
			TimeDilationSubsystem->RegisterIgnoreTimeDilationParticle(DeflectAimPointerParticleComponents[AimPointerIndex]);
		}
	}
	return DeflectAimPointerParticleComponents[AimPointerIndex];
}

void UAstroDashAimComponent::EnableDeflectAimPointer(int32 AimPointerIndex, const FVector& AimStartPosition, const FVector& AimEndPosition, const bool bHasValidTarget)
{
	if (TObjectPtr<UNiagaraComponent> DeflectAimPointerParticleComponent = GetOrCreateDeflectAimPointer(AimPointerIndex))
	{
		const FLinearColor PointerColor = bHasValidTarget ? DeflectAimPointerTargetFoundColor : DeflectAimPointerNoTargetColor;
		DeflectAimPointerParticleComponent->SetVariablePosition("BeamStartPosition", AimStartPosition);
		DeflectAimPointerParticleComponent->SetVariablePosition("BeamEndPosition", AimEndPosition);
		DeflectAimPointerParticleComponent->SetVariableLinearColor("BeamColor", PointerColor);
		DeflectAimPointerParticleComponent->Activate();
	}
}

void UAstroDashAimComponent::DisableDeflectAimPointer(int32 AimPointerIndex)
{
	if (DeflectAimPointerParticleComponents.IsValidIndex(AimPointerIndex))
	{
		if (DeflectAimPointerParticleComponents[AimPointerIndex]->IsActive())
		{
			DeflectAimPointerParticleComponents[AimPointerIndex]->Deactivate();
		}
	}
}

void UAstroDashAimComponent::DisableAllDeflectAimPointers()
{
	for (int32 DashAimPointerIndex = 0; DashAimPointerIndex < DeflectAimPointerParticleComponents.Num(); DashAimPointerIndex++)
	{
		DisableDeflectAimPointer(DashAimPointerIndex);
	}
}

void UAstroDashAimComponent::UpdateAimPreview(const FVector& DashTargetWorldPosition)
{
	// Disables all visuals if the aim result is not set
	if (!CurrentAimResult.IsSet())
	{
		DeactivateAimPreview();
		return;
	}

	// Shows the dash aim decal cursor
	if (DashAimDecalCursor)
	{
		const FVector AimStartPosition = CurrentAimResult->DashStartPosition;
		const FVector PlanarDashTargetWorldPosition = FVector::VectorPlaneProject(DashTargetWorldPosition, FVector::UpVector);
		const FVector PlanarAimStartPosition = FVector::VectorPlaneProject(AimStartPosition, FVector::UpVector);

		const float DecalLength = FVector::Dist(PlanarDashTargetWorldPosition, PlanarAimStartPosition);
		const FVector DecalPosition = (PlanarAimStartPosition + PlanarDashTargetWorldPosition) / 2.f;
		const FVector DecalDirection = (PlanarDashTargetWorldPosition - PlanarAimStartPosition).GetSafeNormal();
		const FLinearColor DecalColor = CurrentAimResult->bIsValidTrajectory ? DashAimCursorValidColor : DashAimCursorInvalidColor;
		DashAimDecalCursor->SetWorldLocation(DecalPosition);
		DashAimDecalCursor->DecalSize.X = DecalLength / 2.f;
		DashAimDecalCursor->SetHiddenInGame(false);
		DashAimDecalCursor->SetWorldRotation(DecalDirection.Rotation());
		DashAimDecalCursor->SetDecalColor(DecalColor);
	}

	// Shows the deflect aim decal cursor
	if (DeflectAimDecalCursor)
	{
		const FVector DecalPosition{ DashTargetWorldPosition.X, DashTargetWorldPosition.Y, 0.f };
		DeflectAimDecalCursor->SetWorldLocation(DecalPosition);
		DeflectAimDecalCursor->SetHiddenInGame(false);

		const FLinearColor DecalColor = CurrentAimResult->DashTargetActor ? DeflectAimCursorYesTargetColor : DeflectAimCursorNoTargetColor;
		DeflectAimDecalCursor->SetDecalColor(DecalColor);
	}

	// Shows all the deflect aim pointer particles
	if (AAstroBall* DeflectedBall = Cast<AAstroBall>(CurrentAimResult->DashTargetActor))
	{
		// Cycles all deflect pointer particles and checks if they should be enabled for the current trajectory
		const TArray<FHitResult>& DeflectionTrajectory = CurrentAimResult->DeflectionTrajectoryHits;
		const int32 DashPointerComponentCount = FMath::Max(DeflectAimPointerParticleComponents.Num(), DeflectionTrajectory.Num());
		
		FVector CurrentAimStartPosition = DeflectedBall->GetActorLocation();
		for (int32 DashPointerComponentIndex = 0; DashPointerComponentIndex < DashPointerComponentCount; DashPointerComponentIndex++)
		{
			if (DeflectionTrajectory.IsValidIndex(DashPointerComponentIndex))
			{
				const bool bHasValidTarget = AAstroBall::CanDamageActor(CurrentAimResult->DeflectionTargetHit.GetActor());
				const FVector CurrentAimEndPosition = DeflectionTrajectory[DashPointerComponentIndex].Location;
				EnableDeflectAimPointer(DashPointerComponentIndex, CurrentAimStartPosition, CurrentAimEndPosition, bHasValidTarget);

				// Updates start position so that the next beam will start from the same position as the previous, which creates a path effect
				CurrentAimStartPosition = CurrentAimEndPosition;
			}
			else
			{
				DisableDeflectAimPointer(DashPointerComponentIndex);
			}
		}
	}
	else
	{
		DisableAllDeflectAimPointers();
	}
}
