/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"
#include "AstroBlockInputGameplayEffectComponent.generated.h"

USTRUCT()
struct FAstroBlockInputEffectData
{
	GENERATED_BODY()

	/** GameplayTag for the input we want to block. */
	UPROPERTY(EditDefaultsOnly, Category = None, meta = (DisplayName = "Block Input w/ Tag"))
	FGameplayTag InputTag;

	/** When set to false, this will unblock the input instead. */
	UPROPERTY(EditDefaultsOnly, Category = None)
	bool bShouldBlockInput = true;
};

/** Blocks player inputs based on tags. NOTE: This should be paired with a GE that unblocks the input. */
UCLASS()
class ASTROSHOWDOWN_API UAstroBlockInputGameplayEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

public:
	/** Determines which inputs to block/unblock. */
	UPROPERTY(EditDefaultsOnly, Category = None, meta = (DisplayName = "Block Input w/ Tag"))
	TArray<FAstroBlockInputEffectData> ModifiedInputs;

};
