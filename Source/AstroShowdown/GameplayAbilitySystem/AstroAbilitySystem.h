/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstroAbilitySystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGameplayAbilityEndedCallback, UGameplayAbility*, GameplayAbility);

UCLASS()
class ASTROSHOWDOWN_API UAstroAbilitySystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gameplay Ability System")
	static void CancelAllAbilities(UAbilitySystemComponent* AbilitySystemComponent);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Ability System")
	static UGameplayAbility* TryActivateAbilityWithCallback(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UGameplayAbility> InAbilityToActivate, FOnGameplayAbilityEndedCallback OnGameplayAbilityEnded);

	// Based on UAbilitySystemBlueprintLibrary::GetGameplayAbilityFromSpecHandle
	static UGameplayAbility* GetGameplayAbilityFromClass(UAbilitySystemComponent* AbilitySystem, TSubclassOf<UGameplayAbility> InAbilityClass, bool& bIsInstance);
	
	static UAbilitySystemComponent* FindLocalPlayerAbilitySystemComponent(UObject* WorldContextObject);

};