/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroCampaignPersistenceComponent.h"
#include "AstroCampaignDataSubsystem.h"
#include "AstroCampaignPersistenceSubsystem.h"
#include "AstroGameState.h"
#include "AstroRoomNavigationComponent.h"
#include "AstroRoomNavigationSubsystem.h"
#include "AstroRoomData.h"

UAstroCampaignPersistenceComponent::UAstroCampaignPersistenceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroCampaignPersistenceComponent::BeginPlay()
{
	Super::BeginPlay();

	AAstroGameState* GameState = GetGameStateChecked<AAstroGameState>();
	if (GameState)
	{
		GameState->OnAstroGameResolved.AddDynamic(this, &UAstroCampaignPersistenceComponent::OnAstroGameResolved);
	}

	if (UAstroRoomNavigationComponent* RoomNavigationComponent = GameState ? GameState->FindComponentByClass<UAstroRoomNavigationComponent>() : nullptr)
	{
		RoomNavigationComponent->OnRoomLoaded.AddUObject(this, &UAstroCampaignPersistenceComponent::OnRoomLoaded);
	}
}

void UAstroCampaignPersistenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AAstroGameState* GameState = GetGameStateChecked<AAstroGameState>();
	if (GameState)
	{
		GameState->OnAstroGameResolved.RemoveDynamic(this, &UAstroCampaignPersistenceComponent::OnAstroGameResolved);
	}

	if (UAstroRoomNavigationComponent* RoomNavigationComponent = GameState ? GameState->FindComponentByClass<UAstroRoomNavigationComponent>() : nullptr)
	{
		RoomNavigationComponent->OnRoomLoaded.RemoveAll(this);
	}
}

void UAstroCampaignPersistenceComponent::OnAstroGameResolved()
{
	// Gets current room and saves it to completed rooms
	const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	const UAstroRoomNavigationComponent* RoomNavigationComponent = GameState ? GameState->FindComponentByClass<UAstroRoomNavigationComponent>() : nullptr;
	const FSoftWorldReference CurrentRoomWorld = RoomNavigationComponent ? RoomNavigationComponent->GetCurrentRoomWorldAsset() : FSoftWorldReference();

	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = UAstroCampaignDataSubsystem::Get(this);
	const UAstroRoomData* CurrentRoom = CampaignDataSubsystem ? CampaignDataSubsystem->GetRoomDataByWorld(CurrentRoomWorld) : nullptr;

	const UAstroRoomNavigationSubsystem* RoomNavigationSubsystem = UAstroRoomNavigationSubsystem::Get(this);
	const UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData RoomNavigationData = RoomNavigationSubsystem->GetRoomRuntimeNavigationData(CurrentRoomWorld);

	if (UAstroCampaignPersistenceSubsystem* CampaignPersistenceSubsystem = UAstroCampaignPersistenceSubsystem::Get(this))
	{
		if (CurrentRoom)
		{
			CampaignPersistenceSubsystem->SaveCampaignCompletedRoom(CurrentRoom->RoomId);
		}
		if (RoomNavigationData.NorthNeighbor)
		{
			CampaignPersistenceSubsystem->SaveCampaignUnlockedRoom(RoomNavigationData.NorthNeighbor->RoomId);
		}
		if (RoomNavigationData.SouthNeighbor)
		{
			CampaignPersistenceSubsystem->SaveCampaignUnlockedRoom(RoomNavigationData.SouthNeighbor->RoomId);
		}
	}
}

void UAstroCampaignPersistenceComponent::OnRoomLoaded(const FSoftWorldReference& RoomWorld)
{
	// Saves current room to visited, last visited and unlocked
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = UAstroCampaignDataSubsystem::Get(this);
	const UAstroRoomData* CurrentRoom = CampaignDataSubsystem ? CampaignDataSubsystem->GetRoomDataByWorld(RoomWorld) : nullptr;

	UAstroCampaignPersistenceSubsystem* CampaignPersistenceSubsystem = UAstroCampaignPersistenceSubsystem::Get(this);
	if (CurrentRoom && CampaignPersistenceSubsystem)
	{
		CampaignPersistenceSubsystem->SaveCampaignVisitedRoom(CurrentRoom->RoomId);
		CampaignPersistenceSubsystem->SaveCampaignLastVisitedRoom(CurrentRoom->RoomId);
		CampaignPersistenceSubsystem->SaveCampaignUnlockedRoom(CurrentRoom->RoomId);
	}
}
