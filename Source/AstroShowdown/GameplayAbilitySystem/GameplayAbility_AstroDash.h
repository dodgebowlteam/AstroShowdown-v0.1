/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_AstroDash.generated.h"

UENUM(BlueprintType)
enum class EAstroDashResult : uint8
{
	None,
	HitNothing,
	HitObstacle,
	HitTarget,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstroDashFinished, EAstroDashResult, DashResult);

/** Dashes towards a fixed target location. */
UCLASS()
class ASTROSHOWDOWN_API UGameplayAbility_AstroDash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_AstroDash();

public:
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FOnAstroDashFinished OnAstroDashFinished;

};
