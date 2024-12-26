/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AbilitySystemComponent.h"
#include "AstroAttributeSetUtils.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "AstroPlayerAttributeSet.generated.h"

UCLASS()
class ASTROSHOWDOWN_API UAstroPlayerAttributeSet : public UAttributeSet
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Stamina;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UAstroPlayerAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStamina, meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxStamina;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UAstroPlayerAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaLimit, meta = (AllowPrivateAccess = true))
	FGameplayAttributeData StaminaLimit;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UAstroPlayerAttributeSet, StaminaLimit)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRechargeCounter, meta = (AllowPrivateAccess = true))
	FGameplayAttributeData StaminaRechargeCounter;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UAstroPlayerAttributeSet, StaminaRechargeCounter)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaminaRechargeDuration, meta = (AllowPrivateAccess = true))
	FGameplayAttributeData StaminaRechargeDuration;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UAstroPlayerAttributeSet, StaminaRechargeDuration)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentMovementSpeed, meta = (AllowPrivateAccess = true))
	FGameplayAttributeData CurrentMovementSpeed;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UAstroPlayerAttributeSet, CurrentMovementSpeed)

private:
	UPROPERTY()
	mutable TWeakObjectPtr<UAbilitySystemComponent> CachedOwnerAbilitySystemComponent = nullptr;

public:
	FOnAstroAttributeChangedEvent OnStaminaChanged;
	FOnAstroAttributeChangedEvent OnStaminaRechargeCounterChanged;
	FOnAstroAttributeChangedEvent OnCurrentMovementSpeedChanged;

	DECLARE_MULTICAST_DELEGATE(FStaminaRechargeCompleted);
	FStaminaRechargeCompleted OnStaminaRechargeCounterCompleted;

private:
	// NOTE: AstroShowdown is a single-player game, so we don't actually need to set this up, but we'll do it anyways
	// just in case replay is ever implemented, as the replay system's DemoNetDriver relies on replication states.
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStaminaAttribute);

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStaminaAttribute);

	UFUNCTION()
	void OnRep_StaminaLimit(const FGameplayAttributeData& OldStaminaLimitAttribute);

	UFUNCTION()
	void OnRep_StaminaRechargeCounter(const FGameplayAttributeData& OldStaminaRechargeAttribute);

	UFUNCTION()
	void OnRep_StaminaRechargeDuration(const FGameplayAttributeData& OldStaminaRechargeDurationAttribute);

	UFUNCTION()
	void OnRep_CurrentMovementSpeed(const FGameplayAttributeData& OldCurrentMovementSpeedAttribute);

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

private:
	UAbilitySystemComponent* GetCachedOwnerAbilitySystemComponent() const;

};
