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
#include "AstroMissionComponent_InteractWithItem.generated.h"

class IAstroInteractableInterface;
class UAstroInteractionComponent;

UCLASS()
class UAstroMissionComponent_InteractWithItem : public UAstroMissionComponent
{
	GENERATED_BODY()

#pragma region UAstroMissionComponent
public:
	UAstroMissionComponent_InteractWithItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual TSharedPtr<FControlFlow> CreateMissionControlFlow() override;
#pragma endregion

protected:
	UPROPERTY(EditDefaultsOnly, Category = Settings)
	TSubclassOf<AActor> InteractableActorClass;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftClassPtr<UAstroMissionDialogWidget> SuccessDialog;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAstroInteractionComponent> CachedInteractionComponent = nullptr;

private:
	void FlowStep_InteractWithObjectMission(FControlFlowNodeRef SubFlow);
	void FlowStep_ResolveMission(FControlFlowNodeRef SubFlow);

private:
	void OnInteract(TScriptInterface<IAstroInteractableInterface> InteractObject, TSharedPtr<FControlFlowNode> SubFlowPtr);

};