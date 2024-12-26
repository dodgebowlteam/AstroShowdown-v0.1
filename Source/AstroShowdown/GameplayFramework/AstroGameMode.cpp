/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroGameMode.h"
#include "AstroController.h"
#include "AstroCharacter.h"
#include "AstroGameContextManagerComponent.h"
#include "AstroGameState.h"
#include "AstroWorldSettings.h"
#include "AstroAssetManager.h"
#include "TimerManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroGameMode, Log, All);
DEFINE_LOG_CATEGORY(LogAstroGameMode);

AAstroGameMode::AAstroGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerControllerClass = AAstroController::StaticClass();
	DefaultPawnClass = AAstroCharacter::StaticClass();
	GameStateClass = AAstroGameState::StaticClass();
}

void AAstroGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Wait for the next frame to give time to initialize startup settings
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AAstroGameMode::InitializeGameContext);
}

void AAstroGameMode::InitializeGameContext()
{
	FPrimaryAssetId GameContextDataId;
	FString GameContextDataIdSource;

	// Sees if AstroWorldSettings has a default GameContextData
	if (AAstroWorldSettings* TypedWorldSettings = Cast<AAstroWorldSettings>(GetWorldSettings()))
	{
		GameContextDataId = TypedWorldSettings->GetWorldGameContext();
		GameContextDataIdSource = TEXT("WorldSettings");
	}

	// Uses default game context as a fallback, in case WorldSettings hasn't specified one
	if (!GameContextDataId.IsValid())
	{
		GameContextDataId = UAstroAssetManager::Get().GetPrimaryAssetIdForPath(DefaultGameContext.ToSoftObjectPath());
		GameContextDataIdSource = TEXT("AstroGameMode");
	}

	ensureMsgf(GameContextDataId.IsValid(), TEXT("Failed to find Game Context"));
	OnGameContextFound(GameContextDataId, GameContextDataIdSource);
}

void AAstroGameMode::OnGameContextFound(FPrimaryAssetId GameContextId, const FString& GameContextIdSource)
{
	if (GameContextId.IsValid())
	{
		UE_LOG(LogAstroGameMode, Log, TEXT("Identified game context %s (Source: %s)"), *GameContextId.ToString(), *GameContextIdSource);

		UAstroGameContextManagerComponent* GameContextComponent = GameState->FindComponentByClass<UAstroGameContextManagerComponent>();
		check(GameContextComponent);
		GameContextComponent->SetCurrentGameContext(GameContextId);
	}
	else
	{
		UE_LOG(LogAstroGameMode, Error, TEXT("Failed to identify game context, loading screen will stay up forever"));
	}
}
