/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "HealthAttributeSet.h"

#include "AstroGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UHealthAttributeSet::UHealthAttributeSet(const FObjectInitializer& ObjectInitializer)
{
}

void UHealthAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.Condition = COND_None;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS_FAST(UHealthAttributeSet, CurrentHealth, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UHealthAttributeSet, MaxHealth, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UHealthAttributeSet, ReviveCounter, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UHealthAttributeSet, ReviveDuration, SharedParams);
}

void UHealthAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealthAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHealthAttributeSet, MaxHealth, OldMaxHealthAttribute);
}

void UHealthAttributeSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldHealthAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHealthAttributeSet, CurrentHealth, OldHealthAttribute);
}

void UHealthAttributeSet::OnRep_ReviveCounter(const FGameplayAttributeData& OldReviveCounterAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHealthAttributeSet, ReviveCounter, OldReviveCounterAttribute);
}

void UHealthAttributeSet::OnRep_ReviveDuration(const FGameplayAttributeData& OldReviveDurationAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHealthAttributeSet, ReviveDuration, OldReviveDurationAttribute);
}

void UHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetDamageAttribute())
	{
		const UAbilitySystemComponent* OwnerAbilitySystemComponent = GetOwningAbilitySystemComponentChecked();
		if (OwnerAbilitySystemComponent->HasMatchingGameplayTag(AstroGameplayTags::Gameplay_Damage_Invulnerable))
		{
			NewValue = 0.f;
		}
		else
		{
			NewValue = FMath::Max(NewValue, 0.f);
		}
	}

	// Clamps revive counter to revive duration
	if (Attribute == GetReviveCounterAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetReviveDuration());
	}
}

void UHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetReviveCounterAttribute())
	{
		OnReviveCounterChanged.Broadcast(OldValue, NewValue);
	}
}

bool UHealthAttributeSet::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void UHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetDamageAttribute())
	{
		const float TotalDamage = GetDamage();
		if (TotalDamage > 0.f)
		{
			const float OldHealthValue = GetCurrentHealth();
			const float NewHealthValue = FMath::Clamp(GetCurrentHealth() - TotalDamage, 0.f, GetMaxHealth());
			SetCurrentHealth(NewHealthValue);

			const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult();
			OnDamaged.Broadcast(OldHealthValue, NewHealthValue, (HitResult ? *HitResult : FHitResult()));
		}
		
		// Resets damage counter
		SetDamage(0.f);
	}

	if (ModifiedAttribute == GetReviveCounterAttribute())
	{
		const float CurrentReviveCounter = GetReviveCounter();
		const float CurrentReviveDuration = GetReviveDuration();
		if (const bool bHasReviveFinished = CurrentReviveCounter >= CurrentReviveDuration)
		{
			// Resets the revive counter
			SetReviveCounter(0.f);

			OnReviveFinished.Broadcast();
		}
	}
}
