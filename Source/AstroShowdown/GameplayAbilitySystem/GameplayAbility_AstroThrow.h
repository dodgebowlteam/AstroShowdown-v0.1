/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_AstroThrow.generated.h"


/** Dashes towards a fixed target location. */
UCLASS()
class UGameplayAbility_AstroThrow : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_AstroThrow();

public:
	/**
	* Called when the ball is thrown.
	*/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAstroThrowCompleted);
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FOnAstroThrowCompleted OnAstroThrowCompleted;

	/**
	* Called when a throw is canceled and the ball isn't thrown.
	* NOTE: If the throw is canceled after the ball is thrown, we call the complete delegate instead.
	*/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAstroThrowCanceled);
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FOnAstroThrowCanceled OnAstroThrowCanceled;

};