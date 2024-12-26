/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_AstroDash.generated.h"

enum class EAstroDashResult : uint8;
class AAstroCharacter;
class UAbilitySystemComponent;
class UGameplayAbility_AstroDash;

UCLASS()
class ASTROSHOWDOWN_API UAbilityTask_AstroDash : public UAbilityTask
{
	GENERATED_BODY()

#pragma region UAbilityTask
public:
	UAbilityTask_AstroDash(const FObjectInitializer& ObjectInitializer);
	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual void OnDestroy(bool AbilityEnded) override;
#pragma endregion


#pragma region UAbilityTask_AstroDash
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_AstroDash* ExecuteAstroDash(UGameplayAbility* OwningAbility, AAstroCharacter* InDashOwner, TSubclassOf<UGameplayAbility_AstroDash> InAstroDashAbilityClass, FGameplayEventData InAstroDashGameplayEvent, FName TaskInstanceName = NAME_None);

protected:
	UFUNCTION()
	void OnAstroDashFinished(EAstroDashResult DashResult);

	UFUNCTION()
	void OnGameplayAbilityActivated(UGameplayAbility* ActivatedAbility);

	UFUNCTION()
	void OnGameplayAbilityFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags);

protected:
	FORCEINLINE bool IsDashing() const { return bIsDashing; }

protected:
	void UnregisterAbilityActivationDelegates();

public:
	/** Called when the dash ability is activated. */
	DECLARE_MULTICAST_DELEGATE(FOnDashStartedDelegate);
	FOnDashStartedDelegate OnStarted;

	/** Called when the dash motion is finished. Returns the status for the dash. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDashFinishedDelegate, EAstroDashResult, DashResult);
	UPROPERTY(BlueprintAssignable)
	FOnDashFinishedDelegate OnFinished;

private:
	/** Character who is playing the dash */
	UPROPERTY()
	TWeakObjectPtr<AAstroCharacter> DashOwner;

	/** Astro Dash GA class */
	UPROPERTY()
	TSubclassOf<UGameplayAbility_AstroDash> AstroDashAbilityClass = nullptr;

	/** Astro Dash GA handle */
	UPROPERTY()
	FGameplayAbilitySpecHandle DashAbilitySpecHandle;

	/** Astro Dash GA instance */
	UPROPERTY()
	TObjectPtr<UGameplayAbility_AstroDash> AstroDashAbility = nullptr;

	UPROPERTY()
	FGameplayEventData CachedAstroDashGameplayEvent;

	bool bIsDashing = false;

#pragma endregion

};