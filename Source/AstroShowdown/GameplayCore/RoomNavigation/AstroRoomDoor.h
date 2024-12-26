/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/SoftWorldReference.h"
#include "GameFramework/Actor.h"
#include "AstroRoomDoor.generated.h"


UENUM(BlueprintType)
enum class EAstroRoomDoorDirection : uint8
{
	North,
	South,
	//West,
	//East,
};

UCLASS()
class AAstroRoomDoor : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	EAstroRoomDoorDirection DoorDirection;

	UPROPERTY(BlueprintReadOnly)
	FSoftWorldReference NeighborRoomWorld;

public:
	void SetDoorDirection(EAstroRoomDoorDirection InDirection);
	void SetNeighborRoomWorld(const FSoftWorldReference& InNeighborRoomWorld);

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnEnterRoom(ACharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnExitRoom(ACharacter* Character);

	UFUNCTION(BlueprintImplementableEvent)
	void OnNeighborRoomWorldChanged(const FSoftWorldReference& NewNeighborRoomWorld);

};