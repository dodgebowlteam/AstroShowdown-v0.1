/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/GameStateComponent.h"
#include "AstroCampaignPersistenceComponent.generated.h"

UCLASS()
class UAstroCampaignPersistenceComponent : public UGameStateComponent
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroCampaignPersistenceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion


private:
	UFUNCTION()
	void OnAstroGameResolved();

	UFUNCTION()
	void OnRoomLoaded(const struct FSoftWorldReference& RoomWorld);
};
