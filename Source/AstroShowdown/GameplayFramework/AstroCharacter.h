/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayAbilitySpecHandle.h"
#include "ModularCharacter.h"
#include "ScalableFloat.h"
#include "AstroCharacter.generated.h"

class AAstroBall;
class UAstroInteractionComponent;
class UAstroDashBlockComponent;
class UAstroStateBase;
class UAstroStateMachine;
class UGameplayAbility_BulletTime;
class UGameplayAbility_AstroDash;
class UGameplayEffect;
struct FAstroRoomGenericMessage;

UCLASS()
class ASTROSHOWDOWN_API AAstroCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAstroCharacter();

#pragma region AActor
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
#pragma endregion


#pragma region IAbilitySystemInterface
protected:
	UPROPERTY(Transient)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;

	UPROPERTY(Transient)
	class UHealthAttributeSet* HealthAttributeSet = nullptr;

	UPROPERTY(Transient)
	class UAstroPlayerAttributeSet* PlayerAttributeSet = nullptr;

public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

private:
	void InitializeAbilitySystemComponent();
#pragma endregion


#pragma region AAstroCharacter
public:
	UPROPERTY(EditDefaultsOnly, Transient, Category = "Setup")
	TSubclassOf<UGameplayEffect> AstroPlayerStatsGE;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Transient, Category = "States")
	TSubclassOf<UAstroStateBase> IdleStateAbilityClass;
	UPROPERTY(EditDefaultsOnly, Transient, Category = "States")
	TSubclassOf<UAstroStateBase> FocusStateAbilityClass;
	UPROPERTY(EditDefaultsOnly, Transient, Category = "States")
	TSubclassOf<UAstroStateBase> WinStateAbilityClass;
	UPROPERTY(EditDefaultsOnly, Transient, Category = "States")
	TSubclassOf<UAstroStateBase> LoseStateAbilityClass;

	UPROPERTY(EditDefaultsOnly, Transient, Category = "Abilities")
	TSubclassOf<UGameplayAbility_BulletTime> BulletTimeAbilityClass;	
	UPROPERTY(EditDefaultsOnly, Transient, Category = "Abilities")
	TSubclassOf<UGameplayAbility_AstroDash> AstroDashAbilityClass;

	/** Stamina cost per AstroDash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities|Dash")
	FScalableFloat AstroDashCost;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient)
	UAstroInteractionComponent* AstroInteractionComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient)
	UAstroDashBlockComponent* AstroDashBlockComponent = nullptr;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstroCharacterGenericDelegate);
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FAstroCharacterGenericDelegate OnAstroCharacterDied;
	UPROPERTY(BlueprintAssignable)
	FAstroCharacterGenericDelegate OnAstroFocusStarted;
	UPROPERTY(BlueprintAssignable)
	FAstroCharacterGenericDelegate OnAstroFocusEnded;

	DECLARE_MULTICAST_DELEGATE(FAstroCharacterGenericStaticDelegate);
	FAstroCharacterGenericStaticDelegate OnAstroCharacterFatigued;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstroPropertyChangedDelegate, float, OldValue, float, NewValue);
	UPROPERTY(BlueprintAssignable)
	FOnAstroPropertyChangedDelegate OnStaminaChangedDelegate;
	UPROPERTY(BlueprintAssignable)
	FOnAstroPropertyChangedDelegate OnStaminaRechargeCounterChangedDelegate;
	UPROPERTY(BlueprintAssignable)
	FAstroCharacterGenericDelegate OnStaminaRechargeCounterStartedDelegate;
	UPROPERTY(BlueprintAssignable)
	FAstroCharacterGenericDelegate OnStaminaRechargeCounterCompletedDelegate;

	FAstroCharacterGenericStaticDelegate OnBallPickedUp;
	FAstroCharacterGenericStaticDelegate OnBallDropped;

protected:
	UPROPERTY(Transient)
	UAstroStateMachine* AstroStateMachine = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient)
	TWeakObjectPtr<AAstroBall> BallPickup = nullptr;

	UPROPERTY()
	FGameplayAbilitySpecHandle BulletTimeAbilityHandle;

private:
	FGameplayMessageListenerHandle RoomEnterMessageHandle;

public:
	UFUNCTION(BlueprintCallable)
	void SwitchState(TSubclassOf<UAstroStateBase> NewState);

	void EnableInteraction();
	void DisableInteraction();
	void BroadcastFocusStateEntered();
	void BroadcastFocusStateEnded();

private:
	/** Drops the ball AND does cleanup. This is preferred over HandleBallReleased if you want to forcibly drop the ball. */
	UFUNCTION(BlueprintCallable)
	void DropBallPickup();
	/** Does cleanup after the ball is dropped. */
	void HandleBallReleased();

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnMoveAction(const FVector& MovementInputAxis);
	void OnMoveCancelAction();
	void OnMoveReleasedAction();
	void OnFocusAction();
	void OnCancelFocusAction();
	void OnAstroDashAction(const struct FAstroDashAimResult& DashAimResult);
	UFUNCTION(BlueprintNativeEvent)
	void OnAstroThrowAction(class UAstroThrowAimProvider* ThrowAimProvider);
	void OnAstroThrowReleasedAction();
	void OnAstroThrowCanceledAction();
	void OnInteractAction();
	void OnExitPracticeModeAction();
	void OnDropBallAction();
	UFUNCTION()
	void OnPickupBallDeactivated(AAstroBall* Ball);

	virtual void OnMoveAction_Implementation(const FVector& MovementInputAxis);
	virtual void OnAstroThrowAction_Implementation(class UAstroThrowAimProvider* ThrowAimProvider);

private:
	UFUNCTION()
	void OnGameResolved();
	UFUNCTION()
	void OnGameReset();
	UFUNCTION()
	void OnStaminaChanged(float OldStamina, float NewStamina);
	UFUNCTION()
	void OnStaminaRechargeCounterChanged(float OldStaminaRecharge, float NewStaminaRecharge);
	UFUNCTION()
	void OnStaminaRechargeCounterCompleted();
	UFUNCTION()
	void OnCurrentMovementSpeedChanged(float OldMovementSpeed, float NewMovementSpeed);
	UFUNCTION()
	void OnDamaged(float OldHealth, float NewHealth, const FHitResult& HitResult);
	UFUNCTION()
	void OnFatigueStateChanged(const FGameplayTag GameplayTag, const int32 GameplayTagCount);
	UFUNCTION()
	void OnRoomEntered(const FGameplayTag GameplayTag, const FAstroRoomGenericMessage& EnterRoomPayload);

public:
	void OnBallInteract(AAstroBall* NewBallPickup);
	UFUNCTION(BlueprintCallable)
	void OnBallThrowFinished();

public:
	void ActivateBulletTimeAbility();
	void CancelBulletTimeAbility();

public:
	UFUNCTION(BlueprintPure)
	float GetCurrentStamina() const;
	UFUNCTION(BlueprintPure)
	float GetMaxStamina() const;
	UFUNCTION(BlueprintPure)
	float GetStaminaLimit() const;
	UFUNCTION(BlueprintPure)
	float GetCurrentStaminaRecharge() const;
	UFUNCTION(BlueprintPure)
	float GetStaminaRechargeDuration() const;
	UFUNCTION(BlueprintPure)
	float GetAstroDashStaminaCost() const;

	AAstroBall* GetBallPickup() const { return BallPickup.Get(); }

	bool IsHoldingBall() const { return BallPickup != nullptr; }

	UAstroInteractionComponent* GetInteractionComponent() const;

#pragma endregion
};
