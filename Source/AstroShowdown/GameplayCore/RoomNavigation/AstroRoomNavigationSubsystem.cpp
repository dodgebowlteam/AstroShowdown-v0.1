/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroRoomNavigationSubsystem.h"
#include "AstroCampaignDataSubsystem.h"
#include "AstroCampaignData.h"
#include "AstroGameState.h"
#include "AstroRoomData.h"
#include "AstroRoomNavigationComponent.h"
#include "AstroSectionData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "SubsystemUtils.h"


void UAstroRoomNavigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency(UAstroCampaignDataSubsystem::StaticClass());

	ensureMsgf(UAstroCampaignDataSubsystem::GetCampaignData(this), TEXT("Invalid campaign data!"));
}

UAstroRoomNavigationSubsystem* UAstroRoomNavigationSubsystem::Get(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject->GetWorld())
	{
		return UGameInstance::GetSubsystem<UAstroRoomNavigationSubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData UAstroRoomNavigationSubsystem::GetRoomRuntimeNavigationData(const FSoftWorldReference& InRoomWorldAsset) const
{
	if (InRoomWorldAsset.WorldAsset.GetLongPackageName().IsEmpty())
	{
		ensureMsgf(false, TEXT("Invalid WorldAsset"));
		return FAstroRoomRuntimeNavigationData();
	}

	const UAstroCampaignData* CampaignData = UAstroCampaignDataSubsystem::GetCampaignData(this);
	if (!ensureMsgf(CampaignData, TEXT("Invalid CampaignData")))
	{
		return FAstroRoomRuntimeNavigationData();
	}

	// Right now, we can assume that all rooms are disposed linearly and consecutively, so a room may only have one entry and one exit,
	// through the south and north, respectively.
	UAstroRoomData* PreviousRoom = nullptr;
	for (int32 SectionIndex = 0; SectionIndex < CampaignData->Sections.Num(); SectionIndex++)
	{
		const UAstroSectionData* CurrentSection = CampaignData->Sections[SectionIndex];
		for (int32 RoomIndex = 0; RoomIndex < CurrentSection->Rooms.Num(); RoomIndex++)
		{
			UAstroRoomData* RoomData = CurrentSection->Rooms[RoomIndex];
			if (const bool bIsAnother = RoomData->RoomLevel.WorldAsset != InRoomWorldAsset.WorldAsset)
			{
				PreviousRoom = RoomData;
				continue;
			}

			const int32 NextRoomIndex = RoomIndex + 1;
			const int32 NextSectionIndex = SectionIndex + 1;
			FAstroRoomRuntimeNavigationData RoomRuntimeNavigationData;
			RoomRuntimeNavigationData.Room = RoomData;
			RoomRuntimeNavigationData.SouthNeighbor = PreviousRoom;

			// NorthNeighbor is either the next room in this section, or the first room in the next section
			if (CurrentSection->Rooms.IsValidIndex(NextRoomIndex))
			{
				RoomRuntimeNavigationData.NorthNeighbor = CurrentSection->Rooms[NextRoomIndex];
			}
			else if (CampaignData->Sections.IsValidIndex(NextSectionIndex) && CampaignData->Sections[NextSectionIndex]->Rooms.IsValidIndex(0))
			{
				RoomRuntimeNavigationData.NorthNeighbor = CampaignData->Sections[NextSectionIndex]->Rooms[0];
			}

			return RoomRuntimeNavigationData;
		}
	}

	return FAstroRoomRuntimeNavigationData();
}

UAstroRoomData* UAstroRoomNavigationSubsystem::GetCurrentRoom() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const AAstroGameState* GameState = World->GetGameStateChecked<AAstroGameState>();
	const UAstroRoomNavigationComponent* RoomNavigationComponent = GameState ? GameState->FindComponentByClass<UAstroRoomNavigationComponent>() : nullptr;
	if (!RoomNavigationComponent)
	{
		return nullptr;
	}

	const FSoftWorldReference CurrentWorldAsset = RoomNavigationComponent->GetCurrentRoomWorldAsset();
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroCampaignDataSubsystem>(this);
	return CampaignDataSubsystem ? CampaignDataSubsystem->GetRoomDataByWorld(CurrentWorldAsset) : nullptr;
}
