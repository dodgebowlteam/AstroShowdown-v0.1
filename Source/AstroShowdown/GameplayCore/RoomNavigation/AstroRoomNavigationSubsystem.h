/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/SoftWorldReference.h"
#include "AstroRoomNavigationSubsystem.generated.h"


class UAstroRoomData;
class UAstroCampaignData;
class AAstroRoomDoor;
class APlayerStart;

UCLASS(config = Game)
class UAstroRoomNavigationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region GameInstanceSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/** Static wrapper for getting this subsystem. */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static UAstroRoomNavigationSubsystem* Get(UObject* WorldContextObject);
#pragma endregion


private:
	/**
	* Used by the UAstroRoomNavigationComponent to override the RootLevel on startup.
	*
	* NOTE: The sole reason why this is an UGameInstanceSubsystem is because we need it to persist RootLevelOverride.
	*/
	UPROPERTY()
	FSoftWorldReference RootLevelOverride;

public:
	struct FAstroRoomRuntimeNavigationData
	{
		UAstroRoomData* Room = nullptr;
		UAstroRoomData* SouthNeighbor = nullptr;
		UAstroRoomData* NorthNeighbor = nullptr;

		FAstroRoomRuntimeNavigationData() = default;
	};
	FAstroRoomRuntimeNavigationData GetRoomRuntimeNavigationData(const FSoftWorldReference& InRoomWorldAsset) const;

	/** @return UAstroRoomData for the current room. */
	UAstroRoomData* GetCurrentRoom() const;

	UFUNCTION(BlueprintCallable)
	void SetRootLevelOverride(const FSoftWorldReference& InRootLevelOverride) { RootLevelOverride = InRootLevelOverride; }
	FSoftWorldReference GetRootLevelOverride() const { return RootLevelOverride; }

};
