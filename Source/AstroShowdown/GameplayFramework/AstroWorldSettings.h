/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroGameContextData.h"
#include "GameFramework/WorldSettings.h"
#include "AstroWorldSettings.generated.h"

/**
 * Overrides AWorldSettings with additional project-specific per-map settings
 */
UCLASS()
class ASTROSHOWDOWN_API AAstroWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	AAstroWorldSettings(const FObjectInitializer& ObjectInitializer);

public:
	/** Returns the default game context to use when a server opens this map */
	FPrimaryAssetId GetWorldGameContext() const;

protected:
	/** The default context to use when a server opens this map */
	UPROPERTY(EditDefaultsOnly, Category = "Game Context")
	TSoftObjectPtr<UAstroGameContextData> WorldGameContext;

};
