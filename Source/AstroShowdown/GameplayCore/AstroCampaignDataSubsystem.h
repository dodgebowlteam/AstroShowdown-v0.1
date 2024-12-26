/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AstroCampaignDataSubsystem.generated.h"

struct FSoftWorldReference;
class UAstroCampaignData;
class UAstroCampaignSaveGame;
class UAstroRoomData;
class UAstroSectionData;

UCLASS()
class UAstroCampaignDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region GameInstanceSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/** Static wrapper for getting this subsystem. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static UAstroCampaignDataSubsystem* Get(UObject* WorldContextObject);
#pragma endregion

public:
	UFUNCTION(BlueprintPure)
	UAstroRoomData* GetRoomDataByWorld(const FSoftWorldReference& InRoomWorld) const;
	UAstroRoomData* GetRoomDataById(const FGuid& InRoomId) const;
	UAstroSectionData* GetSectionDataByRoomId(const FGuid& InRoomId) const;

public:
	/** Static wrapper to get the campaign data. */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static const UAstroCampaignData* GetCampaignData(const UObject* WorldContextObject);

};
