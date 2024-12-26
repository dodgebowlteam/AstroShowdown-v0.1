/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/PawnComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "AstroDashBlockComponent.generated.h"

class USkeletalMesh;

UCLASS()
class UAstroDashBlockComponent : public UPawnComponent
{
	GENERATED_BODY()

#pragma region UPawnComponent
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
#pragma endregion

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USkeletalMesh> CharacterMeshOverride = nullptr;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> OriginalCharacterMesh = nullptr;

	uint8 bIsDashLocked : 1 = false;

private:
	FGameplayMessageListenerHandle RoomEnterMessageHandle;

public:
	void LockDash();

	UFUNCTION(BlueprintCallable)
	void UnlockDash();

protected:
	UFUNCTION()
	void OnRoomEntered(const FGameplayTag GameplayTag, const FAstroRoomGenericMessage& EnterRoomPayload);

};
