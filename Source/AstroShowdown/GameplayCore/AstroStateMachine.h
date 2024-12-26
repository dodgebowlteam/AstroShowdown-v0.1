/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "AstroStateMachine.generated.h"

class UAstroStateBase;
class AAstroCharacter;

/** Manages the state machine for the player. */
UCLASS()
class ASTROSHOWDOWN_API UAstroStateMachine final : public UObject
{
	GENERATED_BODY()

public:
	void SwitchState(AAstroCharacter* InOwner, TSubclassOf<UAstroStateBase> NewState);

};

/** Base abstract class for states in the player's state machine. */
UCLASS(Blueprintable)
class ASTROSHOWDOWN_API UAstroStateBase : public UGameplayAbility
{
	GENERATED_BODY()

	friend class UAstroStateMachine;


#pragma region UGameplayAbility
public:
	UAstroStateBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
#pragma endregion


#pragma region UAstroStateBase
protected:
	UFUNCTION(BlueprintNativeEvent)
	void EnterState(AAstroCharacter* InOwner);
	virtual void EnterState_Implementation(AAstroCharacter* InOwner);

	UFUNCTION(BlueprintNativeEvent)
	void ExitState(AAstroCharacter* InOwner);
	virtual void ExitState_Implementation(AAstroCharacter* InOwner);

	/* Override this to intercept gameplay events from the owning character. */
	UFUNCTION(BlueprintNativeEvent)
	void OnGameplayEventReceived(const struct FGameplayEventData& Payload);
	virtual void OnGameplayEventReceived_Implementation(const struct FGameplayEventData& Payload) { }

	/* Override this to intercept input events from the owning character. */
	UFUNCTION(BlueprintNativeEvent)
	void OnInputEventReceived(FGameplayTag MatchingTag, const struct FGameplayEventData& Payload);
	virtual void OnInputEventReceived_Implementation(FGameplayTag MatchingTag, const struct FGameplayEventData& Payload) { }

private:
	void OnGameplayEventReceivedInternal(const struct FGameplayEventData* Payload);
	void OnInputEventReceivedInternal(FGameplayTag MatchingTag, const struct FGameplayEventData* Payload);

protected:
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AAstroCharacter> Owner = nullptr;

private:
	FDelegateHandle InputEventDelegateHandle;
#pragma endregion

};

/**
* Idle:
* - Available actions: Move, stop time.
*/
UCLASS(Abstract)
class ASTROSHOWDOWN_API UAstroState_Idle : public UAstroStateBase
{
	GENERATED_BODY()

	friend class UAstroStateMachine;

protected:
	virtual void EnterState_Implementation(AAstroCharacter* InOwner) override;
	virtual void ExitState_Implementation(AAstroCharacter* InOwner) override;
	virtual void OnInputEventReceived_Implementation(FGameplayTag MatchingTag, const struct FGameplayEventData& Payload) override;

protected:
	UFUNCTION()
	void OnStaminaChanged(float OldValue, float NewValue);

	UFUNCTION()
	void OnAstroGameBeginLevelCue();

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnStaminaRechargeChanged_BP(float OldRechargeValue, float NewRechargeValue);

	UFUNCTION(BlueprintImplementableEvent)
	void OnFatigueRemoved_BP();

protected:
	void AddFatigue();
	void RemoveFatigue();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Fatigued")
	TSubclassOf<UGameplayEffect> FatiguedGE;
	UPROPERTY(EditDefaultsOnly, Category = "Fatigued")
	TSubclassOf<UGameplayEffect> StaminaRechargeGE;

private:
	UPROPERTY()
	FActiveGameplayEffectHandle ActiveFatigueEffect;
	UPROPERTY()
	FActiveGameplayEffectHandle ActiveStaminaRechargeEffect;

};

enum class EAstroDashResult : uint8;

/**
* Focus:
* - Available actions: Dash.
*/
UCLASS()
class ASTROSHOWDOWN_API UAstroState_Focus : public UAstroStateBase
{
	GENERATED_BODY()

	friend class UAstroStateMachine;	

protected:
	virtual void EnterState_Implementation(AAstroCharacter* InOwner) override;
	virtual void ExitState_Implementation(AAstroCharacter* InOwner) override;

	virtual void OnInputEventReceived_Implementation(FGameplayTag MatchingTag, const struct FGameplayEventData& Payload) override;

private:
	void GrantInputs();
	void UngrantInputs();
	void BlockInputs();
	void UnblockInputs();

	/** @param bShouldCheckStaminaDepletion When enabled, will run CheckStaminaDepletion after consuming stamina. */
	void ConsumeStamina(float Value, const bool bShouldCheckStaminaDepletion = true);
	/** Checks if the stamina is depleted, and if so, may change the character's state. */
	void CheckStaminaDepletion();
	bool HasStamina() const;

protected:
	UFUNCTION(BlueprintNativeEvent)
	void OnAstroDashFinished(EAstroDashResult DashResult);
	virtual void OnAstroDashFinished_Implementation(EAstroDashResult DashResult);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Focus|Stamina")
	TSubclassOf<UGameplayEffect> StaminaCostGE;

	/** Stamina cost per-second while moving. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Focus|Stamina")
	FScalableFloat MovementCostPerSecond;

protected:
	UPROPERTY()
	TWeakObjectPtr<class UAbilityTask_PlayMontageAndWait> FocusAnimationTask = nullptr;

	UPROPERTY()
	TWeakObjectPtr<class UAbilityTask_AstroDash> DashTask = nullptr;

	UPROPERTY()
	FTimerHandle InputInterruptTimerHandle;

	float AccumulatedSpentStamina = 0.f;

	/**
	* Prevents blocking/unblocking the same inputs twice.
	* This is a workaround, and at some point should be replaced by a block-instigator tracker on AstroController.
	*/
	uint8 bHasBlockedInputs : 1 = false;
};
