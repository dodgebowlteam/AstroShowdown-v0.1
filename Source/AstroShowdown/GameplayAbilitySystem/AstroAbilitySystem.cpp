/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroAbilitySystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AstroUtilitiesBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AstroCharacter.h"

void UAstroAbilitySystemBlueprintLibrary::CancelAllAbilities(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}
}

UGameplayAbility* UAstroAbilitySystemBlueprintLibrary::TryActivateAbilityWithCallback(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UGameplayAbility> InAbilityToActivate, FOnGameplayAbilityEndedCallback OnGameplayAbilityEnded)
{
	if (ensure(AbilitySystemComponent))
	{
		// Build and validates the ability spec
		FGameplayAbilitySpec AbilitySpec = AbilitySystemComponent->BuildAbilitySpecFromClass(InAbilityToActivate, 0, -1);
		if (!IsValid(AbilitySpec.Ability))
		{
			ensureMsgf(false, TEXT("TryActivateAbilityWithCallback called with an invalid Ability Class."));
			return nullptr;
		}

		// Activates the ability
		AbilitySystemComponent->GiveAbilityAndActivateOnce(AbilitySpec);

		// Binds the end callback to the ability
		bool bIsInstance = false;
		UGameplayAbility* AbilityInstance = UAstroAbilitySystemBlueprintLibrary::GetGameplayAbilityFromClass(AbilitySystemComponent, InAbilityToActivate, bIsInstance);
		if (ensure(bIsInstance && OnGameplayAbilityEnded.IsBound()))
		{
			if (AbilityInstance->IsActive())
			{
				AbilityInstance->OnGameplayAbilityEnded.AddUFunction(OnGameplayAbilityEnded.GetUObject(), OnGameplayAbilityEnded.GetFunctionName());
			}
			else
			{
				OnGameplayAbilityEnded.Execute(AbilityInstance);
			}
		}

		return AbilityInstance;
	}

	ensure(false);
	return nullptr;
}

UGameplayAbility* UAstroAbilitySystemBlueprintLibrary::GetGameplayAbilityFromClass(UAbilitySystemComponent* AbilitySystem, TSubclassOf<UGameplayAbility> InAbilityClass, bool& bIsInstance)
{
	if (!AbilitySystem)
	{
		UE_LOG(LogTemp, Error, TEXT("GetGameplayAbilityFromSpecHandle() called with an invalid Ability System Component"));

		bIsInstance = false;
		return nullptr;
	}

	FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromClass(InAbilityClass);
	if (!AbilitySpec)
	{
		UE_LOG(LogTemp, Error, TEXT("GetGameplayAbilityFromSpecHandle() Ability Spec not found on passed Ability System Component"));

		bIsInstance = false;
		return nullptr;
	}

	UGameplayAbility* AbilityInstance = AbilitySpec->GetPrimaryInstance();
	bIsInstance = true;

	if (!AbilityInstance)
	{
		AbilityInstance = AbilitySpec->Ability;
		bIsInstance = false;
	}

	return AbilityInstance;
}

UAbilitySystemComponent* UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(UObject* WorldContextObject)
{
	const TWeakObjectPtr<AAstroCharacter> LocalAstroCharacter = UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(WorldContextObject);
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(LocalAstroCharacter.Get());
}
