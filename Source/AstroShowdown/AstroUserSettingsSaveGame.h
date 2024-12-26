/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "GameFramework/SaveGame.h"
#include "AstroUserSettingsSaveGame.generated.h"

UENUM(BlueprintType)
enum class EAstroResolutionMode : uint8
{
	Mid,		// Uses the lowest screen percentage
	High,		// Uses 100% screen percentage	(1080p)
	Ultra,		// Uses 133% screen percentage	(1440p)
};

UCLASS()
class UAstroUserSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAstroUserSettingsSaveGame();

public:
	UPROPERTY()
	float GeneralVolume = 1.f;

	UPROPERTY()
	float MusicVolume = 1.f;

	UPROPERTY()
	EAstroResolutionMode ResolutionMode = EAstroResolutionMode::Mid;

};
