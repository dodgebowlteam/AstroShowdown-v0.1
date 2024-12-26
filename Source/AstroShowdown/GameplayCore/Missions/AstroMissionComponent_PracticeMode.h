/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "AstroMissionComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "AstroMissionComponent_PracticeMode.generated.h"

struct FGameplayEventData;
class UCommonActivatableWidget;
class UGameplayEffect;
struct FAstroGameInitializeRequestMessage;

/** Generic message used by the PracticeMode. */
USTRUCT()
struct FPracticeModeGenericMessage
{
	GENERATED_BODY()
};

UCLASS()
class UAstroMissionComponent_PracticeMode : public UAstroMissionComponent
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroMissionComponent_PracticeMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual TSharedPtr<FControlFlow> CreateMissionControlFlow() override;
#pragma endregion

public:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	TSubclassOf<UGameplayEffect> PracticeModeGE;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UAstroMissionDialogWidget> PracticeModeUnlockedDialog;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UAstroMissionDialogWidget> PracticeModeExitDialog;

private:
	UPROPERTY()
	FActiveGameplayEffectHandle ActivePracticeModeEffect;
	FDelegateHandle ExitPracticeModeInputEventHandle;
	FGameplayMessageListenerHandle InitializeRequestMessageHandle;

private:
	void FlowStep_PracticeModeUnlocked(FControlFlowNodeRef SubFlow);
	void FlowStep_PracticeModeMission(FControlFlowNodeRef SubFlow);

public:
	/** Alerts world entities that the level is being initialized. */
	UFUNCTION(BlueprintCallable)
	void InitializeGame();

	/** Alerts world entities that the level has begun. */
	UFUNCTION(BlueprintCallable)
	void BroadcastBeginLevelCue();

private:
	void RegisterExitPracticeModeInputEvent();
	void UnregisterExitPracticeModeInputEvent();

	void AddPracticeModeGameplayEffect();
	void RemovePracticeModeGameplayEffect();

private:
	void OnInitializeRequestReceived(const FGameplayTag ChannelTag, const FAstroGameInitializeRequestMessage& Message);
	void OnGameInitialized(TWeakPtr<FControlFlowNode> SubFlow);
	void OnGameActivated(TWeakPtr<FControlFlowNode> SubFlow);
	void OnGameResolved(TWeakPtr<FControlFlowNode> SubFlow);

	UFUNCTION()
	void OnInitializedAllEnemies();

	void OnExitPracticeModeInputEventReceived(const FGameplayTag MatchingTag, const FGameplayEventData* Payload);

};
