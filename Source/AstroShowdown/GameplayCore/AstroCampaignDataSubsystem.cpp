/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroCampaignDataSubsystem.h"
#include "AstroCampaignData.h"
#include "AstroCampaignSaveGame.h"
#include "AstroRoomData.h"
#include "AstroSectionData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroCampaignData, Log, All);
DEFINE_LOG_CATEGORY(LogAstroCampaignData);

void UAstroCampaignDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

UAstroCampaignDataSubsystem* UAstroCampaignDataSubsystem::Get(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject->GetWorld())
	{
		return UGameInstance::GetSubsystem<UAstroCampaignDataSubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

UAstroRoomData* UAstroCampaignDataSubsystem::GetRoomDataById(const FGuid& InRoomId) const
{
	const UAstroCampaignData* CampaignData = UAstroCampaignDataSubsystem::GetCampaignData(this);
	if (!ensureMsgf(CampaignData, TEXT("Invalid CampaignData")))
	{
		return nullptr;
	}

	for (UAstroSectionData* SectionData : CampaignData->Sections)
	{
		for (int32 RoomIndex = 0; RoomIndex < SectionData->Rooms.Num(); RoomIndex++)
		{
			UAstroRoomData* RoomData = SectionData->Rooms[RoomIndex];
			if (const bool IsSameRoom = RoomData->RoomId == InRoomId)
			{
				return RoomData;
			}
		}
	}

	return nullptr;
}

UAstroRoomData* UAstroCampaignDataSubsystem::GetRoomDataByWorld(const FSoftWorldReference& InRoomWorld) const
{
	const UAstroCampaignData* CampaignData = UAstroCampaignDataSubsystem::GetCampaignData(this);
	if (!ensureMsgf(CampaignData, TEXT("Invalid CampaignData")))
	{
		return nullptr;
	}

	for (UAstroSectionData* SectionData : CampaignData->Sections)
	{
		for (int32 RoomIndex = 0; RoomIndex < SectionData->Rooms.Num(); RoomIndex++)
		{
			UAstroRoomData* RoomData = SectionData->Rooms[RoomIndex];
			if (const bool bHasSameRoomWorld = RoomData->RoomLevel.WorldAsset == InRoomWorld.WorldAsset)
			{
				return RoomData;
			}
		}
	}

	return nullptr;
}

UAstroSectionData* UAstroCampaignDataSubsystem::GetSectionDataByRoomId(const FGuid& InRoomId) const
{
	const UAstroCampaignData* CampaignData = UAstroCampaignDataSubsystem::GetCampaignData(this);
	if (!ensureMsgf(CampaignData, TEXT("Invalid CampaignData")))
	{
		return nullptr;
	}

	for (UAstroSectionData* SectionData : CampaignData->Sections)
	{
		for (int32 RoomIndex = 0; RoomIndex < SectionData->Rooms.Num(); RoomIndex++)
		{
			UAstroRoomData* RoomData = SectionData->Rooms[RoomIndex];
			if (const bool IsSameRoom = RoomData->RoomId == InRoomId)
			{
				return SectionData;
			}
		}
	}

	return nullptr;
}

const UAstroCampaignData* UAstroCampaignDataSubsystem::GetCampaignData(const UObject* WorldContextObject)
{
	return UAstroCampaignData::Get();
}
