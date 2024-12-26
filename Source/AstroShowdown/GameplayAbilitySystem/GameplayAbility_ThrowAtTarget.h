/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_ThrowAtTarget.generated.h"

class ABallMachine;
struct FThrowAtTargetParameters;
class UAbilityTask_WaitDelay;
class UAbilityTask_BallMachineDynamicTargeting;
class UNiagaraComponent;

/** Gameplay ability used by the ball machine to continuously throw balls. */
UCLASS()
class ASTROSHOWDOWN_API UGameplayAbility_ThrowAtTarget : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_ThrowAtTarget();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** Plays the ball throw montage. */
	UFUNCTION()
	void PlayThrowBallMontage();
	/** ThrowBallAt current target location. */
	void ThrowNextBall();
	/** Throws a ball towards the target location. */
	bool ThrowBallAt(const FVector& ThrowTargetLocation);

private:
	/** Gets the aim pointer particle, or creates one if it doesn't exist yet. */
	TObjectPtr<UNiagaraComponent> GetOrCreateAimPointerParticle();
	/** Updates the timestamp for the loading animation performed by the material. */
	void UpdateLoadingMaterialAnimation(const bool bEnabled, const float PlayRate = 1.f);
	/** Updates the end position for the aim pointer particle. */
	void UpdateAimPointerTargetLocation(const FVector& BeamTargetPosition);
	/** Calculates the target location when aiming forward. */
	FVector CalculateForwardTargetLocation();

protected:
	UFUNCTION()
	void OnShootMontageEventReceived(const FGameplayTag EventTag, const FGameplayEventData EventData);

	UFUNCTION()
	void OnShootMontageCompleted(const FGameplayTag EventTag, const FGameplayEventData EventData, const float NextBallDelay);

private:
	bool GetThrowParameters(ABallMachine*& OutBallMachine, FThrowAtTargetParameters& OutThrowParameters);

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;

private:
	UPROPERTY()
	FGameplayEffectSpecHandle OutgoingDamageEffectHandle;

	UPROPERTY()
	UAbilityTask_WaitDelay* NextBallWaitTask = nullptr;

	UPROPERTY()
	UAbilityTask_BallMachineDynamicTargeting* DynamicTargetingTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> CachedAimPointerParticleInstance = nullptr;

private:
	int32 CurrentStaticTargetIndex = 0;

};
