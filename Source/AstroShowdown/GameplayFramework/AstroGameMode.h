/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "ModularGameMode.h"
#include "AstroGameMode.generated.h"

class UAstroGameContextData;

/**
 * AAstroGameMode
 *
 *	The base game mode class used by this project.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base game mode class used by this project."))
class AAstroGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

#pragma region AGameModeBase
public:
	AAstroGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
#pragma endregion


protected:
	/** The default context to use if one is not specified anywhere else. */
	UPROPERTY(Config)
	TSoftObjectPtr<UAstroGameContextData> DefaultGameContext;

protected:
	void InitializeGameContext();
	void OnGameContextFound(FPrimaryAssetId GameContextId, const FString& GameContextIdSource);


};
