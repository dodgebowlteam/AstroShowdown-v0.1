/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "GameFramework/SaveGame.h"
#include "AstroCampaignSaveGame.generated.h"

UCLASS()
class UAstroCampaignSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAstroCampaignSaveGame();

public:
	UPROPERTY()
	TArray<FGuid> UnlockedRooms;

	UPROPERTY()
	TArray<FGuid> VisitedRooms;

	UPROPERTY()
	TArray<FGuid> CompletedRooms;

	UPROPERTY()
	FGuid LastVisitedRoomId;

	UPROPERTY()
	TArray<uint8> DisplayedHints;

};
