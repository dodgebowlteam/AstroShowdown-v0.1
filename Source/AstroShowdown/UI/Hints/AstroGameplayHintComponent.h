/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/GameStateComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "AstroGameplayHintComponent.generated.h"

class UAstroGameplayHintSubsystem;
struct FPracticeModeGenericMessage;
struct FAstroGenericNPCReviveMessage;

UCLASS()
class UAstroGameplayHintComponent : public UGameStateComponent
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroGameplayHintComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion

private:
	UPROPERTY()
	TWeakObjectPtr<UAstroGameplayHintSubsystem> CachedGameplayHintSubsystem = nullptr;

	FGameplayMessageListenerHandle PracticeModeStartMessageHandle;
	FGameplayMessageListenerHandle NPCReviveMessageHandle;

private:
	/** Prevents from adding the revive hint multiple times during the same match. */
	float LastAddedReviveHintTimestamp = 0.f;

private:
	UFUNCTION()
	void OnPlayerFocusStarted();

	UFUNCTION()
	void OnPlayerFatigued();

	UFUNCTION()
	void OnPlayerDied();

	UFUNCTION()
	void OnLoadingScreenVisibilityChanged(const bool bIsLoadingScreenVisible);

	void OnPracticeModeStart(const FGameplayTag ChannelTag, const FPracticeModeGenericMessage& Message);

	void OnNPCRevive(const FGameplayTag ChannelTag, const FAstroGenericNPCReviveMessage& Message);
};