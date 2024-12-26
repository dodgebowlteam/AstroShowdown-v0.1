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
#include "HealthAttributeSet.generated.h"

UCLASS()
class ASTROSHOWDOWN_API UHealthAttributeSet : public UAttributeSet
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxHealth;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UHealthAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, meta=(AllowPrivateAccess=true))
	FGameplayAttributeData CurrentHealth;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UHealthAttributeSet, CurrentHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReviveCounter, meta=(AllowPrivateAccess=true))
	FGameplayAttributeData ReviveCounter;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UHealthAttributeSet, ReviveCounter)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReviveDuration, meta=(AllowPrivateAccess=true))
	FGameplayAttributeData ReviveDuration;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UHealthAttributeSet, ReviveDuration)

	/*
	* Meta-attribute for damage, meaning we'll use it to modify other attributes, but the attribute itself won't have any gameplay consequences.
	* This attribute will be useful in case we want to add an extra layer of abstraction to health (e.g., first deal damage to shield, then health).
	*/
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess=true))
	FGameplayAttributeData Damage;
	ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(private, UHealthAttributeSet, Damage)

public:
	FOnAstroAttributeChangedEvent OnReviveCounterChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamaged, float, OldValue, float, NewValue, const FHitResult&, HitResult);
	UPROPERTY(BlueprintAssignable)
	FOnDamaged OnDamaged;

	DECLARE_MULTICAST_DELEGATE(FOnReviveFinished);
	FOnReviveFinished OnReviveFinished;

private:
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealthAttribute);

	UFUNCTION()
	void OnRep_CurrentHealth(const FGameplayAttributeData& OldHealthAttribute);

	UFUNCTION()
	void OnRep_ReviveCounter(const FGameplayAttributeData& OldReviveCounterAttribute);

	UFUNCTION()
	void OnRep_ReviveDuration(const FGameplayAttributeData& OldReviveDurationAttribute);

protected:
	/*
	* Called before the current value of an attribute is changed, it doesn't have any additional context like other similar functions because
	* anything can trigger it.
	* 
	* Use case: Ideal place to clamp the value of an attribute.
	* 
	*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	/*
	* Similar to PreAttributeChange, but it's called after the current value of an attribute is changed.
	*/
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/*
	* Called right before applying an Instant GE. Contains contextual data about the GE, and lets you tweak the modifier's value before it's
	* applied to the attribute.
	* This may return false to throw out the entire GE modifier. If you choose to do so, then PostGameplayEffectExecute won't even be called.
	* 
	* Use case: Applies 'gamewide' rules (e.g., granting +3 health for every point of strength).
	* 
	*/
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	/*
	* Called right after modifying an attribute that was changed due to a GE. Contains contextual data about the GE, and lets you use the
	* final calculated modifier value to enforce gameplay rules.
	* 
	* Use case: Applies 'gamewide' rules, just like PreGameplayEffectExecute. In our case, we're using it to delegate changes to the damage
	* meta-attribute to other attributes (i.e., if the GE has modified the value for the damage attribute, we take the damage value and apply
	* it to health. If the character had a shield attribute, we could decrement it before health).
	* 
	*/
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

};
