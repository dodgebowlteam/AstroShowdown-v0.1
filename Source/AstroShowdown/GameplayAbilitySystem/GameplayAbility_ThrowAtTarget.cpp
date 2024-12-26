/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "GameplayAbility_ThrowAtTarget.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilityTask_BallMachineDynamicTargeting.h"
#include "AbilityTask_SpawnBall.h"
#include "Animation/AnimMontage.h"
#include "AstroBall.h"
#include "AstroGameplayTags.h"
#include "AstroTimeDilationSubsystem.h"
#include "BallMachine.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "FMODStudio/Classes/FMODBlueprintStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "SubsystemUtils.h"


namespace BallMachineStatics
{
	static const FName LoadStartTimestamp = "LoadStartTimestamp";
	static const FName LoadDuration = "LoadDuration";

	static const FName AimSocketName = "CannonAim";
	static const FName AimPointerDurationParameterName = "BeamDuration";
	static const FName AimPointerStartParameterName = "BeamStartPosition";
	static const FName AimPointerEndParameterName = "BeamEndPosition";
}

UGameplayAbility_ThrowAtTarget::UGameplayAbility_ThrowAtTarget()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGameplayAbility_ThrowAtTarget::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ABallMachine* Thrower = nullptr;
	FThrowAtTargetParameters ThrowParameters;
	if (!GetThrowParameters(Thrower, ThrowParameters))
	{
		constexpr bool bReplicateEndAbility = true;
		constexpr bool bWasCancelled = true;
		EndAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, GetCurrentActivationInfo(), bReplicateEndAbility, bWasCancelled);
		return;
	}

	// Starts listening to target location changes
	Thrower->OnTargetLocationUpdated.AddUObject(this, &UGameplayAbility_ThrowAtTarget::UpdateAimPointerTargetLocation);

	// Forward: Updates throw target during activation. This should never change afterwards.
	if (ThrowParameters.ThrowTargetType == EBallMachineTargetType::Forward)
	{
		const FVector NewTargetLocation = CalculateForwardTargetLocation();
		Thrower->SetCurrentTargetLocation(NewTargetLocation);
	}

	// Dynamic: Starts dynamic targeting task
	if (ThrowParameters.ThrowTargetType == EBallMachineTargetType::Dynamic)
	{
		DynamicTargetingTask = UAbilityTask_BallMachineDynamicTargeting::ExecuteBallMachineDynamicTargeting(this, Thrower);
		DynamicTargetingTask->ReadyForActivation();
	}

	// Static: Validates static targeting validity
	if (ThrowParameters.ThrowTargetType == EBallMachineTargetType::Static)
	{
		if (!ensureMsgf(ThrowParameters.ThrowTargets.Num() > 0, TEXT("Not enough target locations.")))
		{
			constexpr bool bReplicateEndAbility = true;
			constexpr bool bWasCancelled = true;
			EndAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, GetCurrentActivationInfo(), bReplicateEndAbility, bWasCancelled);
			return;
		}
	}

	// Prepares damage GE
	OutgoingDamageEffectHandle = MakeOutgoingGameplayEffectSpec(DamageGameplayEffectClass);

	// Starts shooting balls
	PlayThrowBallMontage();
}

void UGameplayAbility_ThrowAtTarget::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (NextBallWaitTask && NextBallWaitTask->IsActive())
	{
		NextBallWaitTask->EndTask();
	}

	if (DynamicTargetingTask)
	{
		DynamicTargetingTask->EndTask();
	}

	if (CachedAimPointerParticleInstance)
	{
		CachedAimPointerParticleInstance->ReleaseToPool();
	}

	ABallMachine* Thrower = nullptr;
	FThrowAtTargetParameters ThrowParameters;
	if (GetThrowParameters(Thrower, ThrowParameters))
	{
		Thrower->OnTargetLocationUpdated.RemoveAll(this);
	}

	constexpr bool bMaterialAnimationEnabled = false;
	UpdateLoadingMaterialAnimation(bMaterialAnimationEnabled);
}

void UGameplayAbility_ThrowAtTarget::PlayThrowBallMontage()
{
	ABallMachine* Thrower = nullptr;
	FThrowAtTargetParameters ThrowParameters;
	if (!GetThrowParameters(Thrower, ThrowParameters))
	{
		constexpr bool bReplicateEndAbility = true;
		constexpr bool bWasCancelled = true;
		EndAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, GetCurrentActivationInfo(), bReplicateEndAbility, bWasCancelled);
		return;
	}

	// Updates Static throw target right before making the next shot
	if (ThrowParameters.ThrowTargetType == EBallMachineTargetType::Static)
	{
		const FVector ThrowStartLocation = Thrower->GetActorLocation();
		const FVector ThrowTargetLocation = ThrowStartLocation + ThrowParameters.ThrowTargets[CurrentStaticTargetIndex];
		Thrower->SetCurrentTargetLocation(ThrowTargetLocation);
	}

	// Calculates shoot montage play rate/delay, and then plays it
	// If shoot delay > montage duration: We're shooting slow, so play the montage at regular speed and delay the next shot
	// If shoot delay < montage duration: We're shooting fast, so play the accelerated montage and don't delay
	if (ensure(ThrowParameters.ShootMontage))
	{
		const float ShootMontageDuration = ThrowParameters.ShootMontage->GetPlayLength();
		const bool bShootFast = ThrowParameters.ThrowDelay < ShootMontageDuration;
		const float ShootMontagePlayRate = bShootFast ? (ShootMontageDuration / ThrowParameters.ThrowDelay) : 1.f;
		const float ShootDelay = bShootFast ? 0.f : (ThrowParameters.ThrowDelay - ShootMontageDuration);
		const FGameplayTagContainer AnimNotifyTag { AstroGameplayTags::GameplayEvent_Generic_AnimNotify };

		UAbilityTask_PlayMontageAndWaitForEvent* PlayMontageTask =
			UAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(this, FName(), ThrowParameters.ShootMontage, AnimNotifyTag, ShootMontagePlayRate);
		PlayMontageTask->EventReceivedDelegate.AddUObject(this, &UGameplayAbility_ThrowAtTarget::OnShootMontageEventReceived);
		PlayMontageTask->OnCompletedDelegate.AddUObject(this, &UGameplayAbility_ThrowAtTarget::OnShootMontageCompleted, ShootDelay);
		PlayMontageTask->ReadyForActivation();

		constexpr bool bMaterialAnimationEnabled = true;
		UpdateLoadingMaterialAnimation(bMaterialAnimationEnabled, ShootMontagePlayRate);

		// Waits a bit before showing the aim pointer.
		// NOTE: This is also useful to avoid pointing at weird positions on the first shot.
		if (UWorld* World = GetWorld())
		{
			constexpr float ShootTriggerDuration = 1.5f;		// TODO (#cleanup): Derive this value from the montage, instead of hard-coding
			constexpr float AimPointerStartDelay = 0.2f;		// TODO (#cleanup): Move to tweakable parameter, ranges from [0, 1]

			const float ScaledAimPointerStartDelay = (ShootTriggerDuration * AimPointerStartDelay) / ShootMontagePlayRate;
			const float CorrectedShootTriggerDuration = (ShootTriggerDuration / ShootMontagePlayRate) - ScaledAimPointerStartDelay;

			auto ActivateAimPointer = [this, CorrectedShootTriggerDuration]()
			{
				if (CachedAimPointerParticleInstance)
				{
					CachedAimPointerParticleInstance->SetFloatParameter(BallMachineStatics::AimPointerDurationParameterName, CorrectedShootTriggerDuration);
					CachedAimPointerParticleInstance->Activate(true);
				}
			};

			FTimerHandle UpdateAimTimerHandle;
			World->GetTimerManager().SetTimer(UpdateAimTimerHandle, FTimerDelegate::CreateWeakLambda(this, ActivateAimPointer), ScaledAimPointerStartDelay, false);
		}
	}
}

void UGameplayAbility_ThrowAtTarget::ThrowNextBall()
{
	ABallMachine* Thrower = nullptr;
	FThrowAtTargetParameters ThrowParameters;
	if (!GetThrowParameters(Thrower, ThrowParameters))
	{
		constexpr bool bReplicateEndAbility = true;
		constexpr bool bWasCancelled = true;
		EndAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, GetCurrentActivationInfo(), bReplicateEndAbility, bWasCancelled);
		return;
	}

	const TOptional<FVector> CurrentTargetLocation = Thrower->GetCurrentTargetLocation();
	if (!ensure(CurrentTargetLocation.IsSet()))
	{
		return;
	}

	// Shoots the ball at the current target location.
	ThrowBallAt(CurrentTargetLocation.GetValue());

	// Increments/loops target index if throwing at static targets.
	if (ThrowParameters.ThrowTargetType == EBallMachineTargetType::Static)
	{
		CurrentStaticTargetIndex = (CurrentStaticTargetIndex + 1) % ThrowParameters.ThrowTargets.Num();
	}
}

bool UGameplayAbility_ThrowAtTarget::ThrowBallAt(const FVector& ThrowTargetLocation)
{
	ABallMachine* Thrower = nullptr;
	FThrowAtTargetParameters ThrowParameters;
	if (!GetThrowParameters(Thrower, ThrowParameters))
	{
		constexpr bool bReplicateEndAbility = true;
		constexpr bool bWasCancelled = true;
		EndAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, GetCurrentActivationInfo(), bReplicateEndAbility, bWasCancelled);
		return false;
	}

	// Calculates throw start location
	FVector ThrowStartLocation = Thrower->GetActorLocation();
	if (const USkeletalMeshComponent* BallMachineMesh = Thrower->GetMeshComponent())
	{
		ensure(BallMachineMesh->DoesSocketExist(ThrowParameters.MuzzleSocketName));
		ThrowStartLocation = BallMachineMesh->GetSocketLocation(ThrowParameters.MuzzleSocketName);
	}

	// Prepares SpawnBall task parameters
	FSpawnBallParameters SpawnBallParameters;
	SpawnBallParameters.BallClass = ThrowParameters.ThrownBallClass;
	SpawnBallParameters.DamageEffectHandle = OutgoingDamageEffectHandle;
	SpawnBallParameters.ThrowStartLocation = ThrowStartLocation;
	SpawnBallParameters.ThrowTargetLocation = ThrowTargetLocation;
	SpawnBallParameters.ThrowSpeed = ThrowParameters.ThrowSpeed;
	SpawnBallParameters.ThrowStartOffset = 0.f;
	SpawnBallParameters.bApplyPlaneHeightFixup = false;		// Balls are thrown at the muzzle height, so no need to do the fixup

	// ReadyForActivation() is how you activate the AbilityTask in C++. Blueprint has magic from K2Node_LatentGameplayTaskCall that will automatically call ReadyForActivation().
	UAbilityTask_SpawnBall* SpawnBallTask = UAbilityTask_SpawnBall::SpawnBall(this, SpawnBallParameters);
	SpawnBallTask->ReadyForActivation();

	// Updates the ball machine's animation after shooting
	// NOTE: If we ever need to add more stuff here, consider having a OnShootBall callback and registering to it, instead of adding
	constexpr bool bMaterialAnimationEnabled = false;
	UpdateLoadingMaterialAnimation(bMaterialAnimationEnabled);

	// Plays shoot SFX
	const FTransform ActorTransform = Thrower->GetActorTransform();
	UFMODBlueprintStatics::PlayEventAtLocation(this, ThrowParameters.ShootSFX, ActorTransform, true /*bAutoPlay*/);

	return true;
}

TObjectPtr<UNiagaraComponent> UGameplayAbility_ThrowAtTarget::GetOrCreateAimPointerParticle()
{
	if (!CachedAimPointerParticleInstance)
	{
		ABallMachine* Thrower = nullptr;
		FThrowAtTargetParameters ThrowParameters;
		if (!GetThrowParameters(Thrower, ThrowParameters))
		{
			return nullptr;
		}

		if (!ensure(ThrowParameters.AimPointerParticleTemplate))
		{
			return nullptr;
		}

		USkeletalMeshComponent* BallMachineMesh = Thrower->GetMeshComponent();
		ensure(BallMachineMesh);
		ensure(BallMachineMesh && BallMachineMesh->DoesSocketExist(BallMachineStatics::AimSocketName));

		FFXSystemSpawnParameters SpawnParams;
		SpawnParams.WorldContextObject = this;
		SpawnParams.SystemTemplate = ThrowParameters.AimPointerParticleTemplate;
		SpawnParams.bAutoActivate = false;
		SpawnParams.bAutoDestroy = false;
		SpawnParams.PoolingMethod = EPSCPoolMethod::ManualRelease;
		SpawnParams.bPreCullCheck = false;
		SpawnParams.AttachToComponent = Thrower->GetMeshComponent();				// Attaches the aim beam to the ball machine's aim socket.
		SpawnParams.AttachPointName = BallMachineStatics::AimSocketName;
		SpawnParams.LocationType = EAttachLocation::SnapToTarget;

		if (CachedAimPointerParticleInstance = UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(SpawnParams))
		{
			CachedAimPointerParticleInstance->SetRenderCustomDepth(true);
		}
		ensureMsgf(CachedAimPointerParticleInstance, TEXT("Failed to create BallMachine aim pointer."));
	}

	return CachedAimPointerParticleInstance;
}

void UGameplayAbility_ThrowAtTarget::UpdateLoadingMaterialAnimation(const bool bEnabled, const float PlayRate)
{
	const ABallMachine* Thrower = Cast<ABallMachine>(GetActorInfo().OwnerActor);
	const USkeletalMeshComponent* BallMachineMesh = Thrower ? Cast<USkeletalMeshComponent>(Thrower->GetRootComponent()) : nullptr;
	const UWorld* World = GetWorld();
	if (!Thrower || !BallMachineMesh || !World)
	{
		return;
	}

	const float CurrentTimestamp = World->GetTimeSeconds();
	constexpr float InfiniteDuration = 999999.f;
	const float ShootingMotionDuration = 1.5f / PlayRate;
	for (UMaterialInterface* Material : BallMachineMesh->GetMaterials())
	{
		UMaterialInstanceDynamic* MaterialDynamic = Cast<UMaterialInstanceDynamic>(Material);
		if (!ensure(MaterialDynamic))
		{
			continue;
		}

		MaterialDynamic->SetScalarParameterValue(BallMachineStatics::LoadStartTimestamp, CurrentTimestamp);
		MaterialDynamic->SetScalarParameterValue(BallMachineStatics::LoadDuration, bEnabled ? ShootingMotionDuration : InfiniteDuration);
	}
}

void UGameplayAbility_ThrowAtTarget::UpdateAimPointerTargetLocation(const FVector& BeamTargetPosition)
{
	if (TObjectPtr<UNiagaraComponent> AimPointerParticleInstance = GetOrCreateAimPointerParticle())
	{
		AimPointerParticleInstance->SetVariablePosition(BallMachineStatics::AimPointerEndParameterName, BeamTargetPosition);
	}
}

FVector UGameplayAbility_ThrowAtTarget::CalculateForwardTargetLocation()
{
	ABallMachine* Thrower = nullptr;
	FThrowAtTargetParameters ThrowParameters;
	if (!GetThrowParameters(Thrower, ThrowParameters))
	{
		return FVector::ZeroVector;
	}

	const USkeletalMeshComponent* BallMachineMesh = Thrower->GetMeshComponent();
	if (!BallMachineMesh)
	{
		return FVector::ZeroVector;
	}

	if (const UWorld* World = GetWorld())
	{
		constexpr float ThrowStartOffsetConstant = 200.f;
		constexpr float ThrowAheadConstant = 10000.f;
		const FVector ThrowStartPosition = Thrower->GetActorLocation() + Thrower->GetActorForwardVector() * ThrowStartOffsetConstant;

		FVector ThrowEndPosition = ThrowStartPosition + Thrower->GetActorForwardVector() * ThrowAheadConstant;
		UAbilityTask_BallMachineDynamicTargeting::ClampTargetPositionWithLineOfSight(Thrower, ThrowStartPosition, OUT ThrowEndPosition);
		
		return ThrowEndPosition;
	}

	return FVector::ZeroVector;
}

void UGameplayAbility_ThrowAtTarget::OnShootMontageEventReceived(const FGameplayTag EventTag, const FGameplayEventData EventData)
{
	ThrowNextBall();
}

void UGameplayAbility_ThrowAtTarget::OnShootMontageCompleted(const FGameplayTag EventTag, const FGameplayEventData EventData, const float NextBallDelay)
{
	if (NextBallDelay <= 0.f)
	{
		PlayThrowBallMontage();
	}
	else
	{
		NextBallWaitTask = UAbilityTask_WaitDelay::WaitDelay(this, NextBallDelay);
		NextBallWaitTask->OnFinish.AddDynamic(this, &UGameplayAbility_ThrowAtTarget::PlayThrowBallMontage);
		NextBallWaitTask->ReadyForActivation();
	}
}

bool UGameplayAbility_ThrowAtTarget::GetThrowParameters(ABallMachine*& OutBallMachine, FThrowAtTargetParameters& OutThrowParameters)
{
	ABallMachine* Thrower = Cast<ABallMachine>(GetActorInfo().OwnerActor);
	if (!Thrower)
	{
		return false;
	}

	const FThrowAtTargetParameters& ThrowParameters = Thrower->ThrowParameters;
	if (!ensureMsgf(ThrowParameters.ThrownBallClass.Get(), TEXT("Invalid Ball class")))
	{
		return false;
	}

	OutBallMachine = Thrower;
	OutThrowParameters = ThrowParameters;

	return true;
}
