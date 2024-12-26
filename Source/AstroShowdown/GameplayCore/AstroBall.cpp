/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroBall.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AstroCharacter.h"
#include "AstroCustomDepthStencilConstants.h"
#include "AstroGameplayTags.h"
#include "AstroTimeDilationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "FMODStudio/Classes/FMODBlueprintStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "SubsystemUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroBall, Log, All);
DEFINE_LOG_CATEGORY(LogAstroBall);

namespace AstroBallVars
{
	static bool bIsFriendlyFireSupported = false;
	static FAutoConsoleVariableRef CVarIsFriendlyFireSupported(
		TEXT("AstroBall.IsFriendlyFireSupported"),
		bIsFriendlyFireSupported,
		TEXT("When enabled, balls can deal damage to friendly units."),
		ECVF_Default);
	

	static bool bEnableSimulationDebugTrace = false;
	static FAutoConsoleVariableRef CVarEnableSimulationDebugTrace(
		TEXT("AstroBall.EnableSimulationDebugTrace"),
		bEnableSimulationDebugTrace,
		TEXT("When enabled, will show the debug trace for ball simulation."),
		ECVF_Default);


	static float BallDestroyDelay = 2.f;
	static FAutoConsoleVariableRef CVarBallDestroyDelay(
		TEXT("AstroBall.DestroyDelay"),
		BallDestroyDelay,
		TEXT("How long it takes for the ball to be destroyed after it hits something."),
		ECVF_Default);

	static float BallThrowHeight = 120.f;
	static FAutoConsoleVariableRef CVarBallThrowHeight(
		TEXT("AstroBall.ThrowHeight"),
		BallThrowHeight,
		TEXT("Determines the height at which the ball will travel when thrown."),
		ECVF_Default);

	static float MinimumDeflectDistanceFromOwner = 120.f;
	static FAutoConsoleVariableRef CVarMinimumDeflectDistanceFromOwner(
		TEXT("AstroBall.MinimumDeflectDistanceFromOwner"),
		BallThrowHeight,
		TEXT("Determines the minimum distance between the ball and the player when the ball is deflected. This is useful to avoid clipping."),
		ECVF_Default);

	static float DeathRagdollDelay = 0.15f;
	static FAutoConsoleVariableRef CVarDeathRagdollDelay(
		TEXT("AstroBall.DeathRagdollDelay"),
		DeathRagdollDelay,
		TEXT("Determines how long the ball will take to enter ragdoll state after hitting something (0 == instant)."),
		ECVF_Default);
}

namespace AstroBallStatics
{
	static const FName BallDefaultCollisionProfile = "CustomProfile_Projectile";
	static const FName BallRagdollCollisionProfile = "CustomProfile_BallRagdoll";
	static const FName BallMaterialHueParameter = "hue";
	static const FName BallMaterialRotationParameter = "SpinSensibility";
	static const FName BallTrailAllyParameter = "AllyF";
}

AAstroBall::AAstroBall()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("ProjectileStaticMesh");
	CollisionComponent = CreateDefaultSubobject<USphereComponent>("CollisionComponent");
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	BallTrailComponent = CreateDefaultSubobject<UNiagaraComponent>("BallTrailComponent");

	// Sets up component hierarchy
	const FAttachmentTransformRules DefaultAttachmentRules { EAttachmentRule::KeepRelative, false };
	SetRootComponent(CollisionComponent);
	ProjectileMesh->AttachToComponent(CollisionComponent, DefaultAttachmentRules);
	BallTrailComponent->AttachToComponent(CollisionComponent, DefaultAttachmentRules);

	// Sets up collision
	CollisionComponent->SetCollisionProfileName(AstroBallStatics::BallDefaultCollisionProfile);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// This might be reenabled when the ball's physics state is changed
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->SetAngularDamping(0.25f);								// Prevents the ball from rolling indefinitely
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Sets up rendering
	ProjectileMesh->SetRenderCustomDepth(true);

	// Sets up default properties
	UpdateBallMovementProperties();
}

void AAstroBall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!ProjectileMesh)
	{
		return;
	}

	// Converts the material to dynamic. This is required by PlayBallHueAnimation.
	if (UMaterialInterface* FirstMaterial = ProjectileMesh->GetMaterial(0))
	{
		ProjectileMesh->CreateDynamicMaterialInstance(0, FirstMaterial);
	}
}

void AAstroBall::BeginPlay()
{
	Super::BeginPlay();

	// Forces the projectile movement component to update the collision component instead of the mesh
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
	}

	// Registers ball hit callback
	CollisionComponent->OnComponentHit.AddUniqueDynamic(this, &AAstroBall::OnBallHit);

	UpdateBallMovementProperties();
}

void AAstroBall::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	CollisionComponent->OnComponentHit.RemoveDynamic(this, &AAstroBall::OnBallHit);

	// Deflected balls may ignore time dilation. We're unregistering them here to avoid issues.
	if (UAstroTimeDilationSubsystem* AstroTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		AstroTimeDilationSubsystem->UnregisterIgnoreTimeDilation(this);
	}
}

void AAstroBall::LifeSpanExpired()
{
	if (CurrentBallPhysicsState == EBallPhysicsState::Projectile)
	{
		UE_LOG(LogAstroBall, Warning, TEXT("[%s] Called while ball was still a projectile. This was probably out of bounds."), __FUNCTION__);
	}

	ReturnToPool();
}

#if WITH_EDITOR
void AAstroBall::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName BallMovementTypeName = GET_MEMBER_NAME_CHECKED(AAstroBall, BallMovementType);
	if (PropertyChangedEvent.Property->GetFName() == BallMovementTypeName)
	{
		UpdateBallMovementProperties();
	}
}
#endif

namespace AstroBallUtils
{
	FGameplayTag GetTeamTagFromInstigator(FGameplayEffectSpecHandle InDamageGameplayEffectSpecHandle)
	{
		if (!InDamageGameplayEffectSpecHandle.IsValid())
		{
			return AstroGameplayTags::Gameplay_Team_Neutral;
		}

		AActor* Instigator = InDamageGameplayEffectSpecHandle.Data ? InDamageGameplayEffectSpecHandle.Data->GetContext().GetInstigator() : nullptr;
		if (UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator))
		{
			if (InstigatorASC->HasMatchingGameplayTag(AstroGameplayTags::Gameplay_Team_Ally))
			{
				return AstroGameplayTags::Gameplay_Team_Ally;
			}
			if (InstigatorASC->HasMatchingGameplayTag(AstroGameplayTags::Gameplay_Team_Enemy))
			{
				return AstroGameplayTags::Gameplay_Team_Enemy;
			}
		}

		return AstroGameplayTags::Gameplay_Team_Neutral;
	}
}

void AAstroBall::Throw(FGameplayEffectSpecHandle InDamageGameplayEffectSpecHandle, const FVector& InDirection, const float InSpeed)
{
	// Ensures that the ball is being thrown as a projectile
	if (CurrentBallPhysicsState != EBallPhysicsState::Projectile)
	{
		SetBallPhysicsState(EBallPhysicsState::Projectile);
	}

	// Sets the ball's throw velocity
	const FVector CorrectedThrowDirection = FVector(InDirection.X, InDirection.Y, 0.f).GetSafeNormal();
	DamageGameplayEffectSpecHandle = InDamageGameplayEffectSpecHandle;
	ProjectileMovementComponent->Velocity = CorrectedThrowDirection * InSpeed;

	// Assumes velocity won't change and caches it as PreviousVelocity, so that we can use it during impact
	PreviousVelocity = ProjectileMovementComponent->Velocity;

	// Sets state parameters
	CurrentBounceCount = 0;		// Resets bounces when the ball is thrown

	// Disables time dilation on balls thrown by allies
	// NOTE: Assumes that ally == player, and that the player always wants its throws to ignore time dilation
	const FGameplayTag TeamTag = AstroBallUtils::GetTeamTagFromInstigator(InDamageGameplayEffectSpecHandle);
	if (TeamTag == AstroGameplayTags::Gameplay_Team_Ally)
	{
		if (UAstroTimeDilationSubsystem* AstroTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
		{
			AstroTimeDilationSubsystem->RegisterIgnoreTimeDilation(this);
			AstroTimeDilationSubsystem->RegisterIgnoreTimeDilationParticle(BallTrailComponent);
		}
	}

	// Updates the trail's ally parameter
	if (BallTrailComponent)
	{
		const float AllyValue = TeamTag == AstroGameplayTags::Gameplay_Team_Ally ? 1.f : 0.f;
		BallTrailComponent->SetFloatParameter(AstroBallStatics::BallTrailAllyParameter, AllyValue);
	}

	// Broadcasts the ball throw event
	OnAstroBallThrown.Broadcast();
}

void AAstroBall::SetBallPhysicsState(const EBallPhysicsState BallPhysicsState)
{
	const EBallPhysicsState OldBallPhysicsState = BallPhysicsState;
	switch (BallPhysicsState)
	{
	case EBallPhysicsState::Inactive:
		SetBallPhysicsStateInactive();
		break;
	case EBallPhysicsState::Projectile:
		SetBallPhysicsStateProjectile();
		break;
	case EBallPhysicsState::Attachment:
		SetBallPhysicsStateAttachment();
		break;
	case EBallPhysicsState::Ragdoll:
		SetBallPhysicsStateRagdoll();
		break;
	default:
		ensureMsgf(false, TEXT("This state is not supported yet."));
		return;
	}

	CurrentBallPhysicsState = BallPhysicsState;

	OnAstroBallPhysicsStateChanged.Broadcast(OldBallPhysicsState, CurrentBallPhysicsState);
}

void AAstroBall::SetBallPhysicsStateInactive()
{
	// Disables: Collision, projectile movement, physics simulation, rotation animation
	if (CollisionComponent)
	{
		CollisionComponent->SetSimulatePhysics(false);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetUseCCD(false);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->SetComponentTickEnabled(false);
	}

	SetBallRotationAnimation(false);
}

void AAstroBall::SetBallPhysicsStateProjectile()
{
	// Enables: Collision, projectile movement, rotation animation
	// Disables: Physics simulation
	if (CollisionComponent)
	{
		constexpr bool bUpdateOverlaps = false;
		CollisionComponent->SetCollisionProfileName(AstroBallStatics::BallDefaultCollisionProfile, bUpdateOverlaps);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionComponent->SetSimulatePhysics(false);
		CollisionComponent->SetUseCCD(true);										// Prevents the ball from clipping through walls
	}

	if (BallTrailComponent)
	{
		BallTrailComponent->Activate();
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->SetComponentTickEnabled(true);
		
		// Re-registers the owner's collision component to ProjectileMovementComponent, as it may be unset when the projectile movement
		// component is deactivated, and we want to guarantee that this works no matter the previous state it was in.
		if (CollisionComponent)
		{
			ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
		}
	}

	SetBallRotationAnimation(true);

	// If the ball keeps travelling for a given amount of time, it's automatically destroyed.
	// This prevents the case where out of bound balls would be kept alive indefinitely.
	// NOTE: This is overridden during Die and reset during Grab, so we don't have to worry about balls suddenly vanishing
	constexpr float BallExpirationDuration = 20.f;
	SetLifeSpan(BallExpirationDuration);
}

void AAstroBall::SetBallPhysicsStateAttachment()
{
	// Enables: Rotation animation
	// Disables: Collision, projectile movement and physics simulation
	if (CollisionComponent)
	{
		CollisionComponent->SetSimulatePhysics(false);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->SetComponentTickEnabled(false);
	}

	SetBallRotationAnimation(true);
}

void AAstroBall::SetBallPhysicsStateRagdoll()
{
	// Enables: Physics simulation, collision (with ragdoll profile)
	// Disables: Projectile movement, rotation animation
	if (CollisionComponent)
	{
		constexpr bool bUpdateOverlaps = false;
		CollisionComponent->SetCollisionProfileName(AstroBallStatics::BallRagdollCollisionProfile, bUpdateOverlaps);
		CollisionComponent->SetUseCCD(true);										// Prevents the ball from clipping through walls
		
		// Prevents rubberbanding when ragdolling during low time dilation. Don't ask me why.
		CollisionComponent->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->SetComponentTickEnabled(false);
	}

	if (BallTrailComponent)
	{
		BallTrailComponent->Deactivate();
	}

	SetBallRotationAnimation(false);

	// Effectively enables ragdoll physics.
	// bInstant = Instantly enables ragdoll
	// !bInstant = Enables ragdoll after X seconds. This considers time dilation, so it should take longer if the time is slowed down.
	const FVector ImpactVelocity = PreviousVelocity;
	auto EnableRagdoll = [WeakThis = TWeakObjectPtr<AAstroBall>(this), ImpactVelocity]()
	{
		if (WeakThis.IsValid() && WeakThis->CollisionComponent)
		{
			WeakThis->CollisionComponent->SetSimulatePhysics(true);
			WeakThis->CollisionComponent->SetAllPhysicsLinearVelocity(ImpactVelocity);
		}
	};

	// NOTE: Ally balls are always instant during BT, to prevent the player from being able to abuse the delay to catch recently thrown balls
	const bool bIsInTimeDilation = UAstroTimeDilationSubsystem::GetGlobalTimeDilation(this) < 1.f;
	const FGameplayTag CurrentTeamTag = AstroBallUtils::GetTeamTagFromInstigator(DamageGameplayEffectSpecHandle);
	if (const bool bInstant = AstroBallVars::DeathRagdollDelay == 0.f || PreviousVelocity == FVector::ZeroVector || (bIsInTimeDilation && CurrentTeamTag == AstroGameplayTags::Gameplay_Team_Ally))
	{
		EnableRagdoll();
	}
	else if (UWorld* World = GetWorld())
	{
		// Delays the simulation a bit, to make the ball hit look more impactful
		FTimerHandle RagdollTimerHandle;
		constexpr bool bShouldLoop = false;
		const float DilatedRagdollDelay = UAstroTimeDilationSubsystem::GetDilatedTime(this, AstroBallVars::DeathRagdollDelay);
		World->GetTimerManager().SetTimer(RagdollTimerHandle, FTimerDelegate::CreateLambda(EnableRagdoll), DilatedRagdollDelay, bShouldLoop);
	}
}

// TODO (#perf): Try to make this timestamp-based
void AAstroBall::PlayBallHueAnimation()
{
	if (!ProjectileMesh)
	{
		return;
	}

	UMaterialInstanceDynamic* BallMaterial = Cast<UMaterialInstanceDynamic>(ProjectileMesh->GetMaterial(0));
	if (!BallMaterial)
	{
		return;
	}

	// Stops animating when hue reaches 1
	float CurrentHue;
	BallMaterial->GetScalarParameterValue(AstroBallStatics::BallMaterialHueParameter, CurrentHue);
	if (const bool bFinishedAnimation = CurrentHue >= 1.f)
	{
		return;
	}

	// Increments the hue using real-time delta seconds, so it will animate the same in normal and slowed time
	if (UAstroTimeDilationSubsystem* AstroTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		const float NewHue = FMath::Clamp(CurrentHue + AstroTimeDilationSubsystem->GetRealTimeDeltaSeconds(), 0.f, 1.f);
		BallMaterial->SetScalarParameterValue(AstroBallStatics::BallMaterialHueParameter, NewHue);
	}

	// Loops the animation
	if (UWorld* World = GetWorld())
	{
		BallHueAnimationTimerHandle = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AAstroBall::PlayBallHueAnimation));
	}
}

void AAstroBall::ResetBallHueAnimation()
{
	if (BallHueAnimationTimerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BallHueAnimationTimerHandle);
		}
	}

	if (ProjectileMesh)
	{
		if (UMaterialInstanceDynamic* BallMaterial = Cast<UMaterialInstanceDynamic>(ProjectileMesh->GetMaterial(0)))
		{
			BallMaterial->SetScalarParameterValue(AstroBallStatics::BallMaterialHueParameter, 0.f);
		}
	}
}

void AAstroBall::SetBallRotationAnimation(const bool bEnabled)
{
	UMaterialInstanceDynamic* BallMaterial = ProjectileMesh ? Cast<UMaterialInstanceDynamic>(ProjectileMesh->GetMaterial(0)) : nullptr;
	if (!BallMaterial)
	{
		return;
	}

	const float RotationSensibility = bEnabled ? 1.f : 0.f;
	BallMaterial->SetScalarParameterValue(AstroBallStatics::BallMaterialRotationParameter, RotationSensibility);
}

void AAstroBall::StartGrab(USceneComponent* GrabParent, FName GrabParentSocketName)
{
	if (const bool bAlreadyGrabbing = GrabInput.IsSet())
	{
		ensureMsgf(false, TEXT("Make sure that StartGrab is paired with a ReleaseGrab call."));
		return;
	}

	if (!ensureMsgf(GrabParent, TEXT("Tried to StartGrab without a GrabParent.")))
	{
		return;
	}

	// Caches deflection parameters
	GrabInput = FAstroBallGrabInput();
	GrabInput->DeflectStartPosition = GetActorLocation();
	GrabInput->DeflectStartPosition.Z = GetBallTravelHeight();

	// Disables collision and movement
	SetBallPhysicsState(EBallPhysicsState::Attachment);

	// Attaches AstroBall to the GrabParent
	constexpr EAttachmentRule AttachmentLocationRule = EAttachmentRule::SnapToTarget;
	constexpr EAttachmentRule AttachmentRotationRule = EAttachmentRule::KeepRelative;
	constexpr EAttachmentRule AttachmentScaleRule = EAttachmentRule::KeepWorld;
	constexpr bool bShouldWeld = false;
	const FAttachmentTransformRules GrabAttachmentRules = { AttachmentLocationRule, AttachmentRotationRule, AttachmentScaleRule, bShouldWeld };
	AttachToComponent(GrabParent, GrabAttachmentRules, GrabParentSocketName);

	// Stops ball expiration, in case it's dead
	if (bDead)
	{
		bDead = false;
		SetLifeSpan(-1.f);
		ResetBallHueAnimation();
	}
}

void AAstroBall::ReleaseGrabDeflect(FGameplayEffectSpecHandle DeflectDamageEffect, const FVector& DeflectDirection)
{
	if (const bool bMissingGrabInput = !GrabInput.IsSet())
	{
		ensureMsgf(false, TEXT("Make sure that ReleaseGrab is paired with a StartGrab call."));
		return;
	}

	// Detaches from grabbing actor, and caches its position
	TOptional<FVector> AttachmentOwnerPosition;
	if (const AActor* AttachParent = RootComponent->GetAttachParentActor())
	{
		AttachmentOwnerPosition = AttachParent->GetActorLocation();

		constexpr bool bCallModifyOnComponents = true;
		const FDetachmentTransformRules GrabDetachmentRules = { EDetachmentRule::KeepWorld, bCallModifyOnComponents };
		DetachFromActor(GrabDetachmentRules);
	}

	// Teleports the ball to the deflect start position
	if (AttachmentOwnerPosition.IsSet())
	{
		const float DistanceFromAttachmentOwner = FVector::Dist2D(AttachmentOwnerPosition.GetValue(), GrabInput->DeflectStartPosition);
		const float OffsetFromOrigin = FMath::Max(AstroBallVars::MinimumDeflectDistanceFromOwner - DistanceFromAttachmentOwner, 0.f);
		const FVector CorrectedDeflectStartPosition = GrabInput->DeflectStartPosition + DeflectDirection * OffsetFromOrigin;

		constexpr bool bSweepActorMove = false;
		FHitResult* ActorMoveHitResult = nullptr;
		SetActorLocation(CorrectedDeflectStartPosition, bSweepActorMove, ActorMoveHitResult, ETeleportType::TeleportPhysics);
	}

	// Deflects the ball. This is similar to throwing, except it reuses the current ball speed and multiplies by the deflect multiplier.
	const float CurrentSpeed = ProjectileMovementComponent->Velocity.Length();
	Throw(DeflectDamageEffect, DeflectDirection, DeflectSpeed);

	// Resets the grab input so that we know the ball isn't currently being grabbed
	GrabInput.Reset();
}

void AAstroBall::ReleaseGrabThrow(FGameplayEffectSpecHandle ThrowDamageEffect, const FVector& ThrowOrigin, const FVector& ThrowDirection, const float ThrowSpeed)
{
	if (const bool bMissingGrabInput = !GrabInput.IsSet())
	{
		ensureMsgf(false, TEXT("Make sure that ReleaseGrab is paired with a StartGrab call."));
		return;
	}

	// Detaches from grabbing actor
	constexpr bool bCallModifyOnComponents = true;
	const FDetachmentTransformRules GrabDetachmentRules = { EDetachmentRule::KeepWorld, bCallModifyOnComponents };
	DetachFromActor(GrabDetachmentRules);

	// Teleports the ball to the corrected deflect position
	{
		constexpr bool bSweepActorMove = false;
		FHitResult* ActorMoveHitResult = nullptr;
		const FVector CorrectedThrowOrigin = AAstroBall::ApplyTravelHeightFixupToPosition(ThrowOrigin);
		SetActorLocation(CorrectedThrowOrigin, bSweepActorMove, ActorMoveHitResult, ETeleportType::TeleportPhysics);
	}

	// Throws the ball
	Throw(ThrowDamageEffect, ThrowDirection, ThrowSpeed);

	// Checks if the ball is being released on top of a target
	{
		const float OverlapRadius = GetSimpleCollisionRadius();
		const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { EObjectTypeQuery::ObjectTypeQuery3 };		// Pawns
		const TArray<AActor*> IgnoredActors{ };
		constexpr UClass* ObjectClassFilter = nullptr;

		TArray<AActor*> OutHitActors;
		UKismetSystemLibrary::SphereOverlapActors(this, ThrowOrigin, OverlapRadius, ObjectTypes, ObjectClassFilter, IgnoredActors, OUT OutHitActors);

		if (AActor* HitActor = OutHitActors.IsEmpty() ? nullptr : OutHitActors[0])
		{
			// Simulates a hit result to send to the OnActorHit event.
			FHitResult SweepHitResult;
			SweepHitResult.HitObjectHandle = HitActor;
			SweepHitResult.Location = ThrowOrigin;
			SweepHitResult.Normal = -ThrowDirection;
			SweepHitResult.Component = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
			OnActorHit(HitActor, SweepHitResult);
		}
	}

	// Resets the grab input so that we know the ball isn't currently being grabbed
	GrabInput.Reset();
}

void AAstroBall::DropGrab(const bool bSilent/* = false*/)
{
	if (const bool bMissingGrabInput = !GrabInput.IsSet())
	{
		ensureMsgf(false, TEXT("Make sure that DropGrab is paired with a StartGrab call."));
		return;
	}

	// Detaches from grabbing actor
	constexpr bool bCallModifyOnComponents = true;
	const FDetachmentTransformRules GrabDetachmentRules = { EDetachmentRule::KeepWorld, bCallModifyOnComponents };
	DetachFromActor(GrabDetachmentRules);

	// Turns the ball into a ragdoll again and kills it
	// NOTE: We're killing the ball to avoid exploits with using Grab > Drop to generate infinite balls
	SetBallPhysicsState(EBallPhysicsState::Ragdoll);
	Die();

	// Dropping the ball counts as an interaction
	const UWorld* World = GetWorld();
	LastInteractionTimestamp = World ? World->GetTimeSeconds() : 0.f;

	// Plays the ball drop SFX
	if (ensure(BallDropSFX) && !bSilent)
	{
		constexpr bool bAutoPlay = true;
		UFMODBlueprintStatics::PlayEventAtLocation(this, BallDropSFX, GetActorTransform(), bAutoPlay);
	}

	// Resets the grab input so that we know the ball isn't currently being grabbed
	GrabInput.Reset();

	// Broadcasts the ball drop event
	OnAstroBallDropped.Broadcast();
}

void AAstroBall::Die_Implementation()
{
	bDead = true;

	// Unregisters the ball from ignore time dilation, just in case it's been told to ignore it by Deflect.
	if (UAstroTimeDilationSubsystem* AstroTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		AstroTimeDilationSubsystem->UnregisterIgnoreTimeDilation(this);
		AstroTimeDilationSubsystem->UnregisterIgnoreTimeDilationParticle(BallTrailComponent);
	}

	// Plays ball death animations
	PlayHitFX();
	PlayBallHueAnimation();

	// Starts simulating physics
	SetBallPhysicsState(EBallPhysicsState::Ragdoll);

	// Destroys ball after X seconds
	SetLifeSpan(AstroBallVars::BallDestroyDelay);
}

void AAstroBall::SimulateTrajectory(const FVector& StartDirection, OUT TArray<FHitResult>& OutBounces, const float RadiusMultiplier)
{
	SimulateTrajectory(GetActorLocation(), StartDirection, OUT OutBounces, RadiusMultiplier);
}

void AAstroBall::SimulateTrajectory(const FVector& StartPosition, const FVector& StartDirection, OUT TArray<FHitResult>& OutBounces, const float RadiusMultiplier)
{
	if (StartDirection.IsNearlyZero())
	{
		return;
	}

	// Assumes ball radius is roughly half the size of any of its AABB axes
	const FBox BallBounds = GetComponentsBoundingBox();
	const float BallRadius = (BallBounds.GetSize().X / 2.f) * RadiusMultiplier;
	FVector SimulatedBallPosition = FVector(StartPosition.X, StartPosition.Y, GetBallTravelHeight());
	FVector SimulatedBallDirection = StartDirection;

	OutBounces.Empty();
	const int32 MaxBallBounces = BallMovementType == EBallMovementType::Ricochet ? MaxRicochetBounces : 1;
	for (int32 SimCurrentBounce = 0; SimCurrentBounce < MaxBallBounces; SimCurrentBounce++)
	{
		// Simulates until a generic far position is reached
		static constexpr float SimulationRange = 5000.f;
		const FVector SimulationEndPosition = SimulatedBallPosition + (SimulatedBallDirection * SimulationRange);

		// Ignores the local player. Deflections should never be able to target the player as the dash should place it behind the ball
		TArray<AActor*> ActorsToIgnore{ this };
		if (UWorld* World = GetWorld())
		{
			ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();
			APawn* LocalPlayerPawn = LocalPlayer ? LocalPlayer->GetPlayerController(World)->GetPawn() : nullptr;
			ActorsToIgnore.Add(LocalPlayerPawn);
		}

		// Prevents the case where the simulation trace would hit the same object as the previous iteration because the trace started too close from it.
		// This follows a similar rationale as ignoring the local player.
		if (OutBounces.Num() > 0)
		{
			if (OutBounces.Last().HasValidHitObjectHandle())
			{
				ActorsToIgnore.Add(OutBounces.Last().GetActor());
			}
		}

		// Sets up trace parameters
		const EDrawDebugTrace::Type DebugTraceType = AstroBallVars::bEnableSimulationDebugTrace ? EDrawDebugTrace::Type::ForOneFrame : EDrawDebugTrace::Type::None;
		const float DebugDuration = 0.f;
		constexpr bool bIgnoreSelf = true;
		constexpr bool bTraceComplex = false;
		const FLinearColor TraceColor = FLinearColor::Red;
		const FLinearColor TraceHitColor = FLinearColor::Green;
		const TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes =
		{
			EObjectTypeQuery::ObjectTypeQuery1,		// WorldStatic (walls)
			EObjectTypeQuery::ObjectTypeQuery3,		// Pawn (enemies)
		};

		// Performs trace to see if the ball will hit something
		FHitResult& TraceResult = OutBounces.Emplace_GetRef();
		bool bHitSomething = UKismetSystemLibrary::SphereTraceSingleForObjects(this, SimulatedBallPosition, SimulationEndPosition, BallRadius,
			TraceObjectTypes, bTraceComplex, ActorsToIgnore, DebugTraceType,
			OUT TraceResult, bIgnoreSelf, TraceColor, TraceHitColor, DebugDuration);

		// If nothing is hit, we assume that the ball will go forward infinitely and stop the loop
		if (!bHitSomething)
		{
			TraceResult.Location = SimulationEndPosition;
			TraceResult.Distance = SimulationRange;
			return;
		}

		// If a damageable object is hit, we assume that the ball will hit it and stop, so we stop the loop as well
		if (AAstroBall::CanDamageActor(TraceResult.GetActor()))
		{
			return;
		}

		// Updates the ball's position with the simulation end position
		SimulatedBallPosition = TraceResult.Location;
		SimulatedBallDirection = SimulateDeflection(TraceResult, SimulatedBallDirection * DeflectSpeed);
	}
}

void AAstroBall::Interact_Implementation(AAstroCharacter* InteractionInstigator)
{
	const UWorld* World = GetWorld();
	LastInteractionTimestamp = World ? World->GetTimeSeconds() : 0.f;

	if (InteractionInstigator)
	{
		InteractionInstigator->OnBallInteract(this);
	}

	if (ensure(BallInteractSFX))
	{
		constexpr bool bAutoPlay = true;
		UFMODBlueprintStatics::PlayEventAtLocation(this, BallInteractSFX, GetActorTransform(), bAutoPlay);
	}
}

bool AAstroBall::IsInteractable_Implementation() const
{
	// Assumes that all ragdoll balls are interactable
	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.f;
	return CurrentBallPhysicsState == EBallPhysicsState::Ragdoll && CurrentTime - InteractionCooldown >= LastInteractionTimestamp;
}

void AAstroBall::OnEnterInteractionRange_Implementation()
{
	if (ProjectileMesh)
	{
		ProjectileMesh->SetCustomDepthStencilValue(AstroCustomDepthStencilConstants::InteractableDepthStencilOutlineValue);
	}
}

void AAstroBall::OnExitInteractionRange_Implementation()
{
	if (ProjectileMesh)
	{
		ProjectileMesh->SetCustomDepthStencilValue(AstroCustomDepthStencilConstants::DefaultDepthStencilOutlineValue);
	}
}

float AAstroBall::GetBallTravelHeight()
{
	return AstroBallVars::BallThrowHeight;
}

FVector AAstroBall::ApplyTravelHeightFixupToPosition(const FVector& InPosition)
{
	return { InPosition.X, InPosition.Y, AAstroBall::GetBallTravelHeight() };
}

FVector AAstroBall::SimulateDeflection(FHitResult& Hit, const FVector& CurrentVelocity)
{
	FVector NewVelocity = CurrentVelocity;
	if (ProjectileMovementComponent)
	{
		const FVector Normal = ProjectileMovementComponent->ConstrainNormalToPlane(Hit.Normal);
		const float VDotNormal = (NewVelocity | Normal);
		if (VDotNormal <= 0.f)
		{
			const FVector ProjectedNormal = Normal * -VDotNormal;
			NewVelocity += ProjectedNormal;

			const float ScaledFriction = ProjectileMovementComponent->Friction;
			NewVelocity *= FMath::Clamp(1.f - ScaledFriction, 0.f, 1.f);
			NewVelocity += (ProjectedNormal * FMath::Max(ProjectileMovementComponent->Bounciness, 0.f));
		}
	}


	FVector DeflectionDirection = NewVelocity;
	DeflectionDirection.Normalize();
	return DeflectionDirection;
}

void AAstroBall::ActivateFromPool()
{
	// Enables: Rendering
	SetActorHiddenInGame(false);

	if (BallTrailComponent)
	{
		BallTrailComponent->Activate();
	}

	// Resets the ball's dead state. We might use this to check if a ball can be pooled.
	bDead = false;

	OnSpawn_BP();

	// Broadcasts pool activation event
	OnAstroBallActivated.Broadcast(this);
}

void AAstroBall::ReturnToPool()
{
	// Disables: Collision, movement, rendering, etc
	SetBallPhysicsState(EBallPhysicsState::Inactive);
	SetActorHiddenInGame(true);
	ResetBallHueAnimation();

	if (BallTrailComponent)
	{
		BallTrailComponent->Deactivate();
	}

	// Resets the ball's state
	bDead = true;
	CurrentBounceCount = 0;
	PreviousVelocity = FVector::ZeroVector;
	DamageGameplayEffectSpecHandle.Clear();

	// Broadcasts pool deactivation event
	OnAstroBallDeactivated.Broadcast(this);
}

void AAstroBall::UpdateBallMovementProperties()
{
	// Constraints projectile movement to XY plane
	ProjectileMovementComponent->bConstrainToPlane = true;
	ProjectileMovementComponent->SetPlaneConstraintNormal({ 0.f, 0.f, 1.f });
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->Friction = 0.f;
	ProjectileMovementComponent->Bounciness = 1.0f;

	switch (BallMovementType)
	{
	case EBallMovementType::Regular:
		ProjectileMovementComponent->bShouldBounce = false;
		break;
	case EBallMovementType::Ricochet:
		ProjectileMovementComponent->bShouldBounce = true;
		break;
	default:
		ensure(false && "::ERROR: Undefined ball movement type");
		break;
	}
}

void AAstroBall::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (bDead || CollisionComponent->IsSimulatingPhysics())
	{
		return;
	}
	
	OnActorHit(OtherActor, HitResult);
}

void AAstroBall::OnActorHit(AActor* HitActor, const FHitResult& HitResult)
{
	// Ignores hit dead balls
	if (AAstroBall* HitActorBall = Cast<AAstroBall>(HitActor); HitActorBall && HitActorBall->CurrentBallPhysicsState == EBallPhysicsState::Ragdoll)
	{
		return;
	}

	if (IAbilitySystemInterface* AbilitySystemInterface = HitActor ? Cast<IAbilitySystemInterface>(HitActor) : nullptr)
	{
		TObjectPtr<UFMODEvent> HitSFX = BallHitSFX;

		UAbilitySystemComponent* TargetASC = AbilitySystemInterface->GetAbilitySystemComponent();
		if (ShouldFilterTarget(TargetASC))
		{
			// Target was filtered, do nothing
		}
		else if (TargetASC && DamageGameplayEffectSpecHandle.IsValid())
		{
			// NOTE: Uses PenetrationDepth to pass the ball speed forward. This is important so that the event handler
			// may use it for physics computations. Ideally, we would handle this more gracefully by using a component
			// that handles physics changes, and applying directly to it, but since we have very few use cases, we'll
			// use this workaround instead.
			const float CurrentThrowSpeed = ProjectileMovementComponent->Velocity.Length();
			const float HitThrowSpeed = CurrentThrowSpeed > 0.f ? CurrentThrowSpeed : PreviousVelocity.Length();
			FHitResult ModifiedHitResult = HitResult;
			ModifiedHitResult.PenetrationDepth = HitThrowSpeed;

			// Sets the hit result. Will reset the existing one if there was any, to avoid having multiple hit results.
			constexpr bool bResetHitResult = true;
			DamageGameplayEffectSpecHandle.Data->GetContext().AddHitResult(ModifiedHitResult, bResetHitResult);
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageGameplayEffectSpecHandle.Data.Get());

			HitSFX = nullptr;		// Uses the GameplayCue to play the SFX when damaging an object
		}

		if (HitSFX)
		{
			constexpr bool bAutoPlay = true;
			UFMODBlueprintStatics::PlayEventAtLocation(this, HitSFX, GetActorTransform(), bAutoPlay);
		}

		Die();
	}
	else
	{
		if (BallMovementType == EBallMovementType::Ricochet)
		{
			if (++CurrentBounceCount >= MaxRicochetBounces)
			{
				Die();
			}
			else
			{
				OnAstroBallBounce.Broadcast();
			}
		}
		else
		{
			Die();
		}

		if (BallHitSFX)
		{
			constexpr bool bAutoPlay = true;
			UFMODBlueprintStatics::PlayEventAtLocation(this, BallHitSFX, GetActorTransform(), bAutoPlay);
		}
	}
}

// NOTE: We're only using this method for now because we want an easy way to toggle friendly fire on/off, but whenever we've made up our minds, we should
// remove it and replace it for damage GEs with the appropriate tag checks that we do here
bool AAstroBall::ShouldFilterTarget(UAbilitySystemComponent* TargetASC)
{
	if (AstroBallVars::bIsFriendlyFireSupported)
	{
		return false;
	}

	if (!TargetASC)
	{
		return true;
	}

	// Ignores target if the ball is neutral (i.e., doesn't belong to any team)
	const FGameplayTag TeamTag = AstroBallUtils::GetTeamTagFromInstigator(DamageGameplayEffectSpecHandle);
	if (TeamTag.MatchesTagExact(AstroGameplayTags::Gameplay_Team_Neutral))
	{
		return true;
	}

	// Ignores target if it belongs to the same team
	if (TargetASC->HasMatchingGameplayTag(TeamTag))
	{
		return true;
	}

	return false;
}

void AAstroBall::PlayHitFX()
{
	if (BallHitVFX)
	{
		FFXSystemSpawnParameters SpawnParams;
		SpawnParams.WorldContextObject = this;
		SpawnParams.SystemTemplate = BallHitVFX;
		SpawnParams.bAutoActivate = true;
		SpawnParams.bAutoDestroy = true;
		SpawnParams.PoolingMethod = EPSCPoolMethod::AutoRelease;
		SpawnParams.bPreCullCheck = false;
		SpawnParams.Location = GetActorLocation();

		UNiagaraFunctionLibrary::SpawnSystemAtLocationWithParams(SpawnParams);
	}
}

bool AAstroBall::CanDamageActor(const AActor* TargetActor)
{
	return TargetActor && Cast<IAbilitySystemInterface>(TargetActor);
}
