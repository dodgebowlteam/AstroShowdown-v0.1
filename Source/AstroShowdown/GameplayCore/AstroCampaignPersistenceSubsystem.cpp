/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroCampaignPersistenceSubsystem.h"
#include "AstroCampaignData.h"
#include "AstroCampaignDataSubsystem.h"
#include "AstroCampaignSaveGame.h"
#include "AstroRoomData.h"
#include "AstroSectionData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroCampaignPersistence, Log, All);
DEFINE_LOG_CATEGORY(LogAstroCampaignPersistence);

namespace AstroCampaignPersistenceStatics
{
	const int32 CampaignSaveGameSlotIndex = 0;
	static const FString CampaignSaveGameSlotName = "AstroCampaign";
}

namespace AstroCVars
{
	static int32 RoomLockCheatState = 0;
	static FAutoConsoleVariableRef CVarRoomLockCheatState(
		TEXT("AstroCampaign.RoomLockCheatState"),
		RoomLockCheatState,
		TEXT("0 = Disabled")
		TEXT("1 = Unlock All")
		TEXT("2 = Lock All."));
}

void UAstroCampaignPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadCampaignSaveGame();
}

UAstroCampaignPersistenceSubsystem* UAstroCampaignPersistenceSubsystem::Get(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject->GetWorld())
	{
		return UGameInstance::GetSubsystem<UAstroCampaignPersistenceSubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

void UAstroCampaignPersistenceSubsystem::SaveCampaignUnlockedRoom(const FGuid& RoomId)
{
	if (!RoomId.IsValid())
	{
		UE_LOG(LogAstroCampaignPersistence, Warning, TEXT("[%hs] Invalid room"), __FUNCTION__);
		return;
	}

	check(CachedCampaignSaveGame);
	if (CachedCampaignSaveGame->UnlockedRooms.Contains(RoomId))
	{
		return;
	}

	CachedCampaignSaveGame->UnlockedRooms.AddUnique(RoomId);
	DirtyCampaignSaveGame();
}

void UAstroCampaignPersistenceSubsystem::SaveCampaignVisitedRoom(const FGuid& RoomId)
{
	if (!RoomId.IsValid())
	{
		UE_LOG(LogAstroCampaignPersistence, Warning, TEXT("[%hs] Invalid room"), __FUNCTION__);
		return;
	}

	check(CachedCampaignSaveGame);
	if (CachedCampaignSaveGame->VisitedRooms.Contains(RoomId))
	{
		return;
	}

	CachedCampaignSaveGame->VisitedRooms.AddUnique(RoomId);
	DirtyCampaignSaveGame();
}

void UAstroCampaignPersistenceSubsystem::SaveCampaignLastVisitedRoom(const FGuid& RoomId)
{
	check(CachedCampaignSaveGame);
	if (CachedCampaignSaveGame->LastVisitedRoomId == RoomId)
	{
		return;
	}

	CachedCampaignSaveGame->LastVisitedRoomId = RoomId;
	DirtyCampaignSaveGame();
}

void UAstroCampaignPersistenceSubsystem::SaveCampaignCompletedRoom(const FGuid& RoomId)
{
	if (!RoomId.IsValid())
	{
		UE_LOG(LogAstroCampaignPersistence, Warning, TEXT("[%hs] Invalid room"), __FUNCTION__);
		return;
	}

	check(CachedCampaignSaveGame);
	if (CachedCampaignSaveGame->CompletedRooms.Contains(RoomId))
	{
		return;
	}

	CachedCampaignSaveGame->CompletedRooms.AddUnique(RoomId);
	DirtyCampaignSaveGame();
}

void UAstroCampaignPersistenceSubsystem::SaveDisplayedHint(const uint8 Hint)
{
	check(CachedCampaignSaveGame);
	CachedCampaignSaveGame->DisplayedHints.AddUnique(Hint);
}

void UAstroCampaignPersistenceSubsystem::LoadCampaignSaveGame()
{
	if (CachedCampaignSaveGame)
	{
		UE_LOG(LogAstroCampaignPersistence, Warning, TEXT("[%s] Campaign save game already exists."), __FUNCTION__);
		return;
	}

	USaveGame* CampaignSaveGame = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(AstroCampaignPersistenceStatics::CampaignSaveGameSlotName, AstroCampaignPersistenceStatics::CampaignSaveGameSlotIndex))
	{
		CampaignSaveGame = UGameplayStatics::LoadGameFromSlot(AstroCampaignPersistenceStatics::CampaignSaveGameSlotName, AstroCampaignPersistenceStatics::CampaignSaveGameSlotIndex);
	}
	else
	{
		CampaignSaveGame = UGameplayStatics::CreateSaveGameObject(UAstroCampaignSaveGame::StaticClass());
		UGameplayStatics::SaveGameToSlot(CampaignSaveGame, AstroCampaignPersistenceStatics::CampaignSaveGameSlotName, AstroCampaignPersistenceStatics::CampaignSaveGameSlotIndex);
	}

	CachedCampaignSaveGame = Cast<UAstroCampaignSaveGame>(CampaignSaveGame);
	checkf(CachedCampaignSaveGame, TEXT("Invalid campaign save game. Something went awfully wrong."));
	RecalculateCampaignSaveGameDerivedData();
}

void UAstroCampaignPersistenceSubsystem::ResetCampaignSaveGame()
{
	// Deletes the existing save game and nulls its reference
	UGameplayStatics::DeleteGameInSlot(AstroCampaignPersistenceStatics::CampaignSaveGameSlotName, AstroCampaignPersistenceStatics::CampaignSaveGameSlotIndex);
	CachedCampaignSaveGame = nullptr;

	// Reloads the save game right after
	LoadCampaignSaveGame();
}

void UAstroCampaignPersistenceSubsystem::SaveCampaignSaveGame()
{
	// Saves the campaign's save game to the default slot
	UGameplayStatics::AsyncSaveGameToSlot(CachedCampaignSaveGame, AstroCampaignPersistenceStatics::CampaignSaveGameSlotName, AstroCampaignPersistenceStatics::CampaignSaveGameSlotIndex);

	// Recalculates all data that's derived from the SaveGame data
	RecalculateCampaignSaveGameDerivedData();

	// Clears dirty save timer, if there's any active
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DirtySaveGameTimerHandle);
		DirtySaveGameTimerHandle.Invalidate();
	}
}

void UAstroCampaignPersistenceSubsystem::DirtyCampaignSaveGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (const bool bIsAlreadyDirty = DirtySaveGameTimerHandle.IsValid())
	{
		return;
	}

	DirtySaveGameTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UAstroCampaignPersistenceSubsystem::SaveCampaignSaveGame);
}

void UAstroCampaignPersistenceSubsystem::RecalculateCampaignSaveGameDerivedData()
{
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = UAstroCampaignDataSubsystem::Get(this);
	if (!CampaignDataSubsystem || !CachedCampaignSaveGame)
	{
		return;
	}

	// Resets campaign derived data
	CampaignSaveGameDerivedData = FAstroCampaignSaveGameDerivedData();

	// Derives unlocked sections from the save game data
	for (const FGuid& RoomId : CachedCampaignSaveGame->UnlockedRooms)
	{
		if (UAstroSectionData* UnlockedSection = CampaignDataSubsystem->GetSectionDataByRoomId(RoomId))
		{
			CampaignSaveGameDerivedData.UnlockedSectionsData.AddUnique(UnlockedSection);
		}
	}

	// Finds last visited room data
	CampaignSaveGameDerivedData.LastVisitedRoomData = CampaignDataSubsystem->GetRoomDataById(CachedCampaignSaveGame->LastVisitedRoomId);
}

bool UAstroCampaignPersistenceSubsystem::IsCampaignSaveGameValid() const
{
	if (!CachedCampaignSaveGame)
	{
		return false;
	}

	return CachedCampaignSaveGame->LastVisitedRoomId.IsValid()
		|| !CachedCampaignSaveGame->UnlockedRooms.IsEmpty()
		|| !CachedCampaignSaveGame->CompletedRooms.IsEmpty();
}

bool UAstroCampaignPersistenceSubsystem::IsRoomUnlocked(const UAstroRoomData* InRoomData) const
{
	if (AstroCVars::RoomLockCheatState == 1)
	{
		return true;
	}
	if (AstroCVars::RoomLockCheatState == 2)
	{
		return false;
	}

	return (InRoomData && CachedCampaignSaveGame && CachedCampaignSaveGame->UnlockedRooms.Contains(InRoomData->RoomId));
}

bool UAstroCampaignPersistenceSubsystem::IsRoomCompleted(const UAstroRoomData* InRoomData) const
{
	if (AstroCVars::RoomLockCheatState == 1)
	{
		return true;
	}
	if (AstroCVars::RoomLockCheatState == 2)
	{
		return false;
	}

	return (InRoomData && CachedCampaignSaveGame && CachedCampaignSaveGame->CompletedRooms.Contains(InRoomData->RoomId));
}

bool UAstroCampaignPersistenceSubsystem::WasRoomVisited(const UAstroRoomData* InRoomData) const
{
	if (AstroCVars::RoomLockCheatState == 1)
	{
		return true;
	}
	if (AstroCVars::RoomLockCheatState == 2)
	{
		return false;
	}

	return (InRoomData && CachedCampaignSaveGame && CachedCampaignSaveGame->VisitedRooms.Contains(InRoomData->RoomId));
}

bool UAstroCampaignPersistenceSubsystem::IsSectionUnlocked(UAstroSectionData* InSectionData) const
{
	if (AstroCVars::RoomLockCheatState == 1)
	{
		return true;
	}
	if (AstroCVars::RoomLockCheatState == 2)
	{
		return false;
	}

	return (InSectionData && CampaignSaveGameDerivedData.UnlockedSectionsData.Contains(InSectionData));
}

int32 UAstroCampaignPersistenceSubsystem::NumSectionsUnlocked() const
{
	return CampaignSaveGameDerivedData.UnlockedSectionsData.Num();
}

bool UAstroCampaignPersistenceSubsystem::WasHintDisplayed(const uint8 Hint) const
{
	return CachedCampaignSaveGame && CachedCampaignSaveGame->DisplayedHints.Contains(Hint);
}

UAstroRoomData* UAstroCampaignPersistenceSubsystem::GetLastVisitedRoomData()
{
	return CampaignSaveGameDerivedData.LastVisitedRoomData.Get();
}
