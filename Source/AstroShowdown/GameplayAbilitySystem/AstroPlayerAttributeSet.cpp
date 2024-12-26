/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroPlayerAttributeSet.h"
#include "AstroGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UAstroPlayerAttributeSet::UAstroPlayerAttributeSet(const FObjectInitializer& ObjectInitializer)
{
}

void UAstroPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.Condition = COND_None;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS_FAST(UAstroPlayerAttributeSet, Stamina, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAstroPlayerAttributeSet, MaxStamina, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAstroPlayerAttributeSet, StaminaLimit, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAstroPlayerAttributeSet, StaminaRechargeCounter, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAstroPlayerAttributeSet, StaminaRechargeDuration, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAstroPlayerAttributeSet, CurrentMovementSpeed, SharedParams);
}

void UAstroPlayerAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStaminaAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAstroPlayerAttributeSet, Stamina, OldStaminaAttribute);
}

void UAstroPlayerAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStaminaAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAstroPlayerAttributeSet, MaxStamina, OldMaxStaminaAttribute);
}

void UAstroPlayerAttributeSet::OnRep_StaminaLimit(const FGameplayAttributeData& OldStaminaLimitAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAstroPlayerAttributeSet, StaminaLimit, OldStaminaLimitAttribute);
}

void UAstroPlayerAttributeSet::OnRep_StaminaRechargeCounter(const FGameplayAttributeData& OldStaminaRechargeAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAstroPlayerAttributeSet, StaminaRechargeCounter, OldStaminaRechargeAttribute);
}

void UAstroPlayerAttributeSet::OnRep_StaminaRechargeDuration(const FGameplayAttributeData& OldStaminaRechargeDurationAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAstroPlayerAttributeSet, StaminaRechargeDuration, OldStaminaRechargeDurationAttribute);
}

void UAstroPlayerAttributeSet::OnRep_CurrentMovementSpeed(const FGameplayAttributeData& OldCurrentMovementSpeedAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UAstroPlayerAttributeSet, CurrentMovementSpeed, OldCurrentMovementSpeedAttribute);
}

void UAstroPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamps Stamina to the [0, Max] range
	if (Attribute == GetStaminaAttribute())
	{
		UAbilitySystemComponent* OwnerAbilitySystemComponent = GetCachedOwnerAbilitySystemComponent();
		if (OwnerAbilitySystemComponent && OwnerAbilitySystemComponent->HasMatchingGameplayTag(AstroGameplayTags::Gameplay_Stamina_Infinite))
		{
			NewValue = GetMaxStamina();
		}
		else
		{
			NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
		}
	}

	// Clamps StaminaRecharge to the [0, Max] range
	if (Attribute == GetStaminaRechargeCounterAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetStaminaRechargeDuration());
	}
}

void UAstroPlayerAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetStaminaAttribute())
	{
		OnStaminaChanged.Broadcast(OldValue, NewValue);
	}
	if (Attribute == GetStaminaRechargeCounterAttribute())
	{
		OnStaminaRechargeCounterChanged.Broadcast(OldValue, NewValue);
	}
	if (Attribute == GetCurrentMovementSpeedAttribute())
	{
		OnCurrentMovementSpeedChanged.Broadcast(OldValue, NewValue);
	}
}

void UAstroPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetStaminaRechargeCounterAttribute())
	{
		const float CurrentRecharge = GetStaminaRechargeCounter();
		const float RechargeDuration = GetStaminaRechargeDuration();
		if (const bool bReachedFullRecharge = CurrentRecharge >= RechargeDuration)
		{
			// Recharges all stamina and resets the stamina recharge counter
			const float OldStaminaValue = Stamina.GetCurrentValue();
			const float MaxStaminaValue = MaxStamina.GetCurrentValue();
			SetStamina(MaxStaminaValue);
			SetStaminaRechargeCounter(0.f);

			// Broadcasts stamina and recharge change events
			OnStaminaChanged.Broadcast(OldStaminaValue, MaxStaminaValue);
			OnStaminaRechargeCounterCompleted.Broadcast();
			OnStaminaRechargeCounterChanged.Broadcast(CurrentRecharge, 0.f);
		}
	}
}

UAbilitySystemComponent* UAstroPlayerAttributeSet::GetCachedOwnerAbilitySystemComponent() const
{
	if (!CachedOwnerAbilitySystemComponent.IsValid())
	{
		CachedOwnerAbilitySystemComponent = GetOwningAbilitySystemComponentChecked();
	}

	return CachedOwnerAbilitySystemComponent.Get();
}
