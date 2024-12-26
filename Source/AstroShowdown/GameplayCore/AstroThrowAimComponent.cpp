/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroThrowAimComponent.h"
#include "AstroBall.h"
#include "AstroCharacter.h"
#include "AstroController.h"
#include "AstroTimeDilationSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "SubsystemUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroThrowAimComponent)

DECLARE_LOG_CATEGORY_EXTERN(LogAstroThrowAim, Log, All);
DEFINE_LOG_CATEGORY(LogAstroThrowAim);

namespace AstroThrowAimUtils
{
	namespace Private
	{
		void CalculateThrowDirection(const FVector& BallStartPosition, const FVector& BallTargetPosition, FVector& OutDeflectionDirection)
		{
			OutDeflectionDirection = (BallTargetPosition - BallStartPosition);
			OutDeflectionDirection = FVector::VectorPlaneProject(OutDeflectionDirection, FVector::UpVector);
			OutDeflectionDirection.Normalize();
		}

		FVector GetActorCenterOfMassLocation(AActor* Actor)
		{
			static const FName TargetSocketName = "TargetSocket";
			if (!Actor)
			{
				return FVector::ZeroVector;
			}

			USceneComponent* ActorRootComponent = Actor->GetRootComponent();
			if (!ActorRootComponent)
			{
				return Actor->GetActorLocation();
			}

			if (ActorRootComponent->DoesSocketExist(TargetSocketName))
			{
				return ActorRootComponent->GetSocketLocation(TargetSocketName);
			}

			return ActorRootComponent->Bounds.GetSphere().Center;
		}

		FVector GetCharacterAimStartPosition(AAstroCharacter* Character, const FVector& ThrowDirection)
		{
			static const FName CenterOfMassSocketName = "CenterOfMassSocket";

			const FVector ActorCenter = GetActorCenterOfMassLocation(Character);
			FVector CharacterCenterOfMass = ActorCenter;
			if (Character && Character->GetMesh())
			{
				if (Character->GetMesh()->DoesSocketExist(CenterOfMassSocketName))
				{
					CharacterCenterOfMass = Character->GetMesh()->GetSocketLocation(CenterOfMassSocketName);
				}
			}

			// Offsets the aim by the distance from the character's center of mass.
			// This ensures that even when the character's SKM is animated away from its collision, its aim will follow along,
			// instead of looking like it's attached to the actor's collision capsule.
			//
			// CenterOfMassOffsetCoefficient will correct the aim in the right direction. If we multiplied the offset directly
			// by the throw direction and the SKM was animated in the opposite direction, it would still look a bit off.
			const FVector CenterOfMassDirection = (CharacterCenterOfMass - ActorCenter).GetSafeNormal2D();
			const float CenterOfMassOffsetCoefficient = FVector::DotProduct(CenterOfMassDirection, ThrowDirection);
			const float DistanceFromCenterOfMass = FVector::Distance(ActorCenter, CharacterCenterOfMass);
			return ActorCenter + (ThrowDirection * DistanceFromCenterOfMass * CenterOfMassOffsetCoefficient);
		}
	}
}

namespace AstroStatics
{
	static const FName AimPointerStartParameterName = "BeamStartPosition";
	static const FName AimPointerEndParameterName = "BeamEndPosition";
}

void UAstroThrowAimComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AAstroController* PlayerController = Cast<AAstroController>(GetOwner()))
	{
		CachedOwnerController = PlayerController;
		CachedOwnerCharacter = Cast<AAstroCharacter>(PlayerController->GetPawn());
	}

	TWeakObjectPtr<UAstroThrowAimComponent> WeakThis = this;
	ThrowAimProvider = NewObject<UAstroThrowAimProvider>();
	ThrowAimProvider->AimDataProviderCallback = [WeakThis]() { return WeakThis.IsValid() ? WeakThis->CurrentAimResult : FAstroThrowAimData(); };

	ensure(CachedOwnerController.IsValid() && CachedOwnerCharacter.IsValid());
}

void UAstroThrowAimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (TObjectPtr<UNiagaraComponent> AimPointerParticleComponent : AimPointerParticleComponents)
	{
		AimPointerParticleComponent->ReleaseToPool();
	}

	if (AimCursorParticleComponent)
	{
		AimCursorParticleComponent->ReleaseToPool();
	}
}

void UAstroThrowAimComponent::ActivateAim()
{
}

void UAstroThrowAimComponent::DeactivateAim()
{
	DisableAllThrowAimPointers();
	DisableThrowAimCursor();
}

void UAstroThrowAimComponent::UpdateAim()
{
	if (!CachedOwnerController.IsValid())
	{
		return;
	}

	// If aiming directly at a damageable target, snaps the aim to its center
	// Otherwise, sets the target location to equal the mouse's world position
	FHitResult CurrentThrowTarget = FindThrowTarget();
	if (AAstroBall::CanDamageActor(CurrentThrowTarget.GetActor()))
	{
		// TODO (#polish): Add aim assist snapping
		CurrentThrowTarget.Location = AstroThrowAimUtils::Private::GetActorCenterOfMassLocation(CurrentThrowTarget.GetActor());
	}
	else
	{
		FVector OutMouseWorldPosition, OutMouseWorldDirection;
		CachedOwnerController->DeprojectMousePositionToWorld(OutMouseWorldPosition, OutMouseWorldDirection);

		// NOTE: For whatever reason, DeprojectMousePositionToWorld doesn't actually return the mouse's world position,
		// so we need to project it to the character's plane
		// FROM: https://www.reddit.com/r/unrealengine/comments/iop0gh/having_trouble_with_deprojectmousepositiontoworld/
		const FVector ActorLocation = CachedOwnerCharacter->GetActorLocation();
		OutMouseWorldPosition = FMath::LinePlaneIntersection(
			OutMouseWorldPosition,
			OutMouseWorldPosition + (OutMouseWorldDirection * 10000.f),
			ActorLocation,
			FVector{ 0.f, 0.f, 1.f }
		);

		// Also adjusts the target location to the ball's travel height
		CurrentThrowTarget.Location = OutMouseWorldPosition;
		CurrentThrowTarget.Location.Z = AAstroBall::GetBallTravelHeight();
	}

	if (AAstroBall* CurrentBallPickup = CachedOwnerCharacter.IsValid() ? CachedOwnerCharacter->GetBallPickup() : nullptr)
	{
		// Simulates the ball's trajectory
		TArray<FHitResult> BallHits;
		SimulateBallTrajectory(CurrentThrowTarget.Location, CurrentBallPickup, /*OUT*/ BallHits);

		// Updates CurrentAimResult with the current target
		CurrentAimResult.TargetHitResult = CurrentThrowTarget;
		CurrentAimResult.TrajectoryHits = BallHits;

		const FVector BallStartPosition = CachedOwnerCharacter.IsValid() ? CachedOwnerCharacter->GetActorLocation() : CurrentThrowTarget.Location;
		AstroThrowAimUtils::Private::CalculateThrowDirection(BallStartPosition, CurrentThrowTarget.Location, CurrentAimResult.CachedThrowDirection);
	}

	UpdateAimPreview();
}

FHitResult UAstroThrowAimComponent::FindThrowTarget()
{
	if (CachedOwnerController.IsValid())
	{
		FHitResult CursorTraceHit;
		constexpr bool bTraceComplex = false;
		if (const bool bFoundHit = CachedOwnerController->GetHitResultUnderCursorByChannel(ThrowAimTargetChannel, bTraceComplex, CursorTraceHit))
		{
			return CursorTraceHit;
		}
	}

	return FHitResult();
}

void UAstroThrowAimComponent::SimulateBallTrajectory(const FVector& TargetPosition, AAstroBall* Ball, OUT TArray<FHitResult>& Hits)
{
	FVector DeflectionDirection;
	const FVector BallStartPosition = AstroThrowAimUtils::Private::GetActorCenterOfMassLocation(CachedOwnerCharacter.Get());
	AstroThrowAimUtils::Private::CalculateThrowDirection(BallStartPosition, TargetPosition, OUT DeflectionDirection);

	if (Ball)
	{
		Ball->SimulateTrajectory(BallStartPosition, DeflectionDirection, Hits);
	}
}

TObjectPtr<UNiagaraComponent> UAstroThrowAimComponent::GetOrCreateThrowAimPointer(int32 AimPointerIndex)
{
	constexpr int32 MaxAimPointerCount = 8;
	ensure(AimPointerIndex < MaxAimPointerCount);

	if (!AimPointerParticleComponents.IsValidIndex(AimPointerIndex))
	{
		AimPointerParticleComponents.SetNumZeroed(AimPointerIndex + 1);

		FFXSystemSpawnParameters SpawnParams;
		SpawnParams.WorldContextObject = this;
		SpawnParams.SystemTemplate = AimPointerParticleTemplate;
		SpawnParams.bAutoActivate = false;
		SpawnParams.bAutoDestroy = false;
		SpawnParams.PoolingMethod = EPSCPoolMethod::ManualRelease;
		SpawnParams.bPreCullCheck = false;

		if (AimPointerParticleComponents[AimPointerIndex] = UNiagaraFunctionLibrary::SpawnSystemAtLocationWithParams(SpawnParams))
		{
			AimPointerParticleComponents[AimPointerIndex]->SetRenderCustomDepth(true);
		}
		ensureMsgf(AimPointerParticleComponents[AimPointerIndex], TEXT("Failed to create throw aim pointer"));

		if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
		{
			TimeDilationSubsystem->RegisterIgnoreTimeDilationParticle(AimPointerParticleComponents[AimPointerIndex]);
		}
	}
	return AimPointerParticleComponents[AimPointerIndex];
}


TObjectPtr<UNiagaraComponent> UAstroThrowAimComponent::GetOrCreateThrowAimCursor()
{
	if (!AimCursorParticleComponent)
	{
		FFXSystemSpawnParameters SpawnParams;
		SpawnParams.WorldContextObject = this;
		SpawnParams.SystemTemplate = AimCursorParticleTemplate;
		SpawnParams.bAutoActivate = false;
		SpawnParams.bAutoDestroy = false;
		SpawnParams.PoolingMethod = EPSCPoolMethod::ManualRelease;
		SpawnParams.bPreCullCheck = false;

		AimCursorParticleComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocationWithParams(SpawnParams);
		check(AimCursorParticleComponent);
		AimCursorParticleComponent->SetRenderCustomDepth(true);

		if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
		{
			TimeDilationSubsystem->RegisterIgnoreTimeDilationParticle(AimCursorParticleComponent);
		}
	}
	return AimCursorParticleComponent;
}

void UAstroThrowAimComponent::EnableThrowAimPointer(int32 AimPointerIndex, const FVector& AimStartPosition, const FVector& AimEndPosition)
{
	if (TObjectPtr<UNiagaraComponent> ThrowAimPointerParticleComponent = GetOrCreateThrowAimPointer(AimPointerIndex))
	{
		ThrowAimPointerParticleComponent->SetVariablePosition(AstroStatics::AimPointerStartParameterName, AimStartPosition);
		ThrowAimPointerParticleComponent->SetVariablePosition(AstroStatics::AimPointerEndParameterName, AimEndPosition);
		ThrowAimPointerParticleComponent->Activate();
	}
}

void UAstroThrowAimComponent::DisableThrowAimPointer(int32 AimPointerIndex)
{
	if (AimPointerParticleComponents.IsValidIndex(AimPointerIndex))
	{
		const TObjectPtr<UNiagaraComponent>& AimPointerParticleComponent = AimPointerParticleComponents[AimPointerIndex];
		if (AimPointerParticleComponent && AimPointerParticleComponent->IsActive())
		{
			AimPointerParticleComponent->Deactivate();
		}
	}
}

void UAstroThrowAimComponent::DisableAllThrowAimPointers()
{
	for (int32 DashAimPointerIndex = 0; DashAimPointerIndex < AimPointerParticleComponents.Num(); DashAimPointerIndex++)
	{
		DisableThrowAimPointer(DashAimPointerIndex);
	}
}

void UAstroThrowAimComponent::EnableThrowAimCursor(const FVector& AimTargetPosition)
{
	if (TObjectPtr<UNiagaraComponent> ThrowAimCursorParticleComponent = GetOrCreateThrowAimCursor())
	{
		ThrowAimCursorParticleComponent->Activate();
		ThrowAimCursorParticleComponent->SetWorldLocation(AimTargetPosition);
	}
}

void UAstroThrowAimComponent::DisableThrowAimCursor()
{
	if (TObjectPtr<UNiagaraComponent> ThrowAimCursorParticleComponent = GetOrCreateThrowAimCursor())
	{
		ThrowAimCursorParticleComponent->Deactivate();
	}
}

void UAstroThrowAimComponent::UpdateAimPreview()
{
	if (const bool bShouldDisplayThrowionAim = CachedOwnerCharacter.IsValid()/* && CurrentAimResult.TargetHitResult.bBlockingHit*/)
	{
		// Cycles all Throw pointer particles and checks if they should be enabled for the current trajectory
		const TArray<FHitResult>& BallTrajectoryHits = CurrentAimResult.TrajectoryHits;
		const int32 DashPointerComponentCount = FMath::Max(AimPointerParticleComponents.Num(), BallTrajectoryHits.Num());

		constexpr float AimStartOffset = 50.f;
		FVector NextAimStartPosition;
		for (int32 AimPointerComponentIndex = 0; AimPointerComponentIndex < DashPointerComponentCount; AimPointerComponentIndex++)
		{
			// If first pointer, calculate the aim's start position.
			if (const bool bIsFirstPointer = AimPointerComponentIndex == 0)
			{
				NextAimStartPosition = AstroThrowAimUtils::Private::GetCharacterAimStartPosition(CachedOwnerCharacter.Get(), CurrentAimResult.CachedThrowDirection);
				NextAimStartPosition += CurrentAimResult.CachedThrowDirection * AimStartOffset;			// Moves the aim beam start away from the player
			}

			if (BallTrajectoryHits.IsValidIndex(AimPointerComponentIndex))
			{
				// Finds the aim's end.
				// If bounce is enabled (i.e., ricochet): Gets the end position from the trajectory
				// Otherwise: Gets the end position from the aim, which will consider aim assist
				const bool bIsSingleBounce = AimPointerComponentIndex == 0 && BallTrajectoryHits.Num() == 1;
				const FVector CurrentAimEndPosition = bIsSingleBounce ? CurrentAimResult.TargetHitResult.Location : BallTrajectoryHits[AimPointerComponentIndex].Location;
				EnableThrowAimPointer(AimPointerComponentIndex, NextAimStartPosition, CurrentAimEndPosition);

				// Updates start position so that the next beam will start from the same position
				// as the previous, which creates a path effect
				NextAimStartPosition = CurrentAimEndPosition;
			}
			else
			{
				DisableThrowAimPointer(AimPointerComponentIndex);
			}
		}

		// Also enables the aim cursor
		EnableThrowAimCursor(CurrentAimResult.TargetHitResult.Location);
	}
	else
	{
		DisableAllThrowAimPointers();
		DisableThrowAimCursor();
	}
}

void UAstroThrowAimProvider::GetAimThrowData(FAstroThrowAimData& OutAimThrowData, bool& bSuccess)
{
	bSuccess = false;
	if (AimDataProviderCallback)
	{
		bSuccess = true;
		OutAimThrowData = AimDataProviderCallback();
	}
}
