/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroMissionComponent.h"
#include "Components/GameStateComponent.h"
#include "ControlFlowNode.h"
#include "AstroMissionComponent_PressButtons.generated.h"

class AAstroBall;

UCLASS()
class UAstroMissionComponent_PressButtons : public UAstroMissionComponent
{
	GENERATED_BODY()

#pragma region UAstroMissionComponent
public:
	UAstroMissionComponent_PressButtons(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual TSharedPtr<FControlFlow> CreateMissionControlFlow() override;
#pragma endregion

public:
	UPROPERTY(EditDefaultsOnly, Category = UI, meta=(EditCondition = "bIsTutorialEnabled"))
	TSoftClassPtr<UAstroMissionDialogWidget> MovementOnboardingDialog;

	UPROPERTY(EditDefaultsOnly, Category = UI, meta = (EditCondition = "bIsTutorialEnabled"))
	TSoftClassPtr<UAstroMissionDialogWidget> GrabOnboardingDialog;

	UPROPERTY(EditDefaultsOnly, Category = UI, meta = (EditCondition = "bIsTutorialEnabled"))
	TSoftClassPtr<UAstroMissionDialogWidget> ThrowOnboardingDialog;

	UPROPERTY(EditDefaultsOnly, Category = UI, meta = (EditCondition = "bIsTutorialEnabled"))
	TSoftClassPtr<UAstroMissionDialogWidget> MissionOnboardingDialog;

	UPROPERTY(EditDefaultsOnly, Category = Settings)
	uint8 bIsTutorialEnabled : 1 = false;

private:
	UPROPERTY()
	TWeakObjectPtr<AAstroBall> LastBallPickup = nullptr;

private:
	void FlowStep_MovementOnboarding(FControlFlowNodeRef SubFlow);
	void FlowStep_GrabBallOnboarding(FControlFlowNodeRef SubFlow);
	void FlowStep_ThrowBallOnboarding(FControlFlowNodeRef SubFlow, TSharedPtr<bool> SuccessPtr);
	void FlowStep_ThrowBallOnboardingCleanup(FControlFlowNodeRef SubFlow);
	void FlowStep_OpenDoorMission(FControlFlowNodeRef SubFlow);

	/** Clears the ball pickup events that we need during the onboarding throw/grab missions. */
	void ClearLastBallPickup();

private:
	void OnEnemyAliveCountChanged(const int32 OldCount, const int32 NewCount, FControlFlowNodePtr SubFlowPtr);

	UFUNCTION()
	void OnEnemyRegistered(AActor* RegisteredEnemy);
	UFUNCTION()
	void OnEnemyUnregistered(AActor* UnregisteredEnemy);

};
