/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroMissionComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "AstroMissionComponent_DefeatEnemies.generated.h"

struct FGameplayEventData;
class UCommonActivatableWidget;
struct FAstroGameInitializeRequestMessage;

UCLASS()
class UAstroMissionComponent_DefeatEnemies : public UAstroMissionComponent
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroMissionComponent_DefeatEnemies(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual TSharedPtr<FControlFlow> CreateMissionControlFlow() override;
#pragma endregion

public:
	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> VictoryScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|UI", meta = (EditConditionHides, EditCondition = "bIsTutorialEnabled"))
	TSoftClassPtr<UAstroMissionDialogWidget> DefeatEnemiesOnboardingDialog;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|UI", meta = (EditConditionHides, EditCondition = "bShowDashTutorial"))
	TSoftClassPtr<UAstroMissionDialogWidget> DashOnboardingDialog;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Settings", meta = (EditConditionHides, EditCondition = "bIsTutorialEnabled"))
	TSoftClassPtr<AActor> ActivationTotemClass;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Settings")
	uint8 bIsTutorialEnabled : 1 = false;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial|Settings")
	uint8 bShowDashTutorial : 1 = false;

private:
	FDelegateHandle DashInputEventDelegateHandle;
	FGameplayMessageListenerHandle InitializeRequestMessageHandle;

private:
	void FlowStep_MissionActivationOnboarding(FControlFlowNodeRef SubFlow);
	void FlowStep_DefeatEnemiesMission(FControlFlowNodeRef SubFlow);

public:
	/** Alerts world entities that the level is being initialized. */
	UFUNCTION(BlueprintCallable)
	void InitializeGame();

	/** Alerts world entities that the level has begun. */
	UFUNCTION(BlueprintCallable)
	void BroadcastBeginLevelCue();

private:
	void OnInitializeRequestReceived(const FGameplayTag ChannelTag, const FAstroGameInitializeRequestMessage& Message);
	void OnGameInitialized(TWeakPtr<FControlFlowNode> SubFlow, TWeakObjectPtr<AActor> WeakActivationTotem);
	void OnGameActivated(TWeakPtr<FControlFlowNode> SubFlow);
	void OnGameResolved(TWeakPtr<FControlFlowNode> SubFlow);

	UFUNCTION()
	void OnPlayerDied();
	void OnEnemyCountChanged(const int32 OldCount, const int32 NewCount);

	UFUNCTION()
	void OnInitializedAllEnemies();

	void OnDashInputEventReceived(const FGameplayTag MatchingTag, const FGameplayEventData* Payload);
};
