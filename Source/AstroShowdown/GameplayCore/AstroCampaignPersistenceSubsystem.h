/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "AstroCampaignPersistenceSubsystem.generated.h"

struct FSoftWorldReference;
class UAstroCampaignSaveGame;
class UAstroSectionData;

UCLASS()
class UAstroCampaignPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region GameInstanceSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/** Static wrapper for getting this subsystem. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "Get Campaign Persistence Subsystem"))
	static UAstroCampaignPersistenceSubsystem* Get(UObject* WorldContextObject);
#pragma endregion

private:
	/** Cached AstroCampaignSaveGame object. Avoids having to load it every time we want to manipulate the campaign's persistence. */
	UPROPERTY(Transient)
	mutable TObjectPtr<UAstroCampaignSaveGame> CachedCampaignSaveGame = nullptr;

	/** Data that is not persisted, but derived from the save game instead. */
	struct FAstroCampaignSaveGameDerivedData
	{
		TWeakObjectPtr<UAstroRoomData> LastVisitedRoomData = nullptr;
		TArray<TWeakObjectPtr<UAstroSectionData>> UnlockedSectionsData;
	}
	CampaignSaveGameDerivedData;

private:
	/** Defers saving the game for a single frame, to account for multiple data changes in a single frame. */
	FTimerHandle DirtySaveGameTimerHandle;

public:
	void SaveCampaignUnlockedRoom(const FGuid& RoomId);
	void SaveCampaignVisitedRoom(const FGuid& RoomId);
	void SaveCampaignLastVisitedRoom(const FGuid& RoomId);
	void SaveCampaignCompletedRoom(const FGuid& RoomId);
	void SaveDisplayedHint(const uint8 Hint);

	UFUNCTION(BlueprintCallable)
	void ResetCampaignSaveGame();

private:
	void LoadCampaignSaveGame();
	UFUNCTION()
	void SaveCampaignSaveGame();
	void DirtyCampaignSaveGame();
	void RecalculateCampaignSaveGameDerivedData();

public:
	/** @return true if the current campaign save game exists and has relevant data. */
	UFUNCTION(BlueprintPure)
	bool IsCampaignSaveGameValid() const;

	/** @return true if room was already unlocked by the player. */
	UFUNCTION(BlueprintPure)
	bool IsRoomUnlocked(const UAstroRoomData* InRoomData) const;

	/** @return true if room was already completed by the player. */
	bool IsRoomCompleted(const UAstroRoomData* InRoomData) const;

	/** @return true if room was already visited by the player. */
	bool WasRoomVisited(const UAstroRoomData* InRoomData) const;

	/** @return true if section was already unlocked by the player. */
	UFUNCTION(BlueprintPure)
	bool IsSectionUnlocked(UAstroSectionData* InSectionData) const;

	/** @return How many sections the player has unlocked. */
	int32 NumSectionsUnlocked() const;

	/** @return true if a given hint was already displayed. */
	bool WasHintDisplayed(const uint8 Hint) const;

	/** @return DataAsset for the room that was visited last by the player. */
	UFUNCTION(BlueprintPure)
	UAstroRoomData* GetLastVisitedRoomData();

};
