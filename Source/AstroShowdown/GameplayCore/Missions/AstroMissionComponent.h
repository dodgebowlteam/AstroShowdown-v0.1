/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/GameStateComponent.h"
#include "ControlFlowNode.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstroMissionComponent.generated.h"

class UAstroGameContextData;
class AAstroGameState;
class UAstroIndicatorWidgetManagerComponent;
class UAstroMissionDialogWidget;

UENUM(BlueprintType)
enum class EAstroMissionStatus : uint8
{
	InProgress,
	Success,
	Fail,
};

USTRUCT(BlueprintType)
struct FAstroMissionPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSoftClassPtr<UAstroMissionDialogWidget> MissionWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	EAstroMissionStatus Status = EAstroMissionStatus::InProgress;
};


// ===========================================
// UAstroMissionComponent
// ===========================================

UCLASS()
class UAstroMissionComponent : public UGameStateComponent
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroMissionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual TSharedPtr<FControlFlow> CreateMissionControlFlow() PURE_VIRTUAL(UAstroMissionComponent::CreateMissionControlFlow, return nullptr;);
#pragma endregion

protected:
	/** Reference to the AAstroGameState object that owns this component. */
	UPROPERTY(Transient)
	TWeakObjectPtr<AAstroGameState> GameStateOwner = nullptr;

	/** Reference to the mission's control flow. This determines the execution order of the mission steps. */
	TSharedPtr<FControlFlow> MissionControlFlow = nullptr;

	/** Cached reference to the widget manager component owned by the GameState. NOTE: Should be accessed through its getter. */
	UPROPERTY(Transient)
	mutable TWeakObjectPtr<UAstroIndicatorWidgetManagerComponent> CachedIndicatorComponent = nullptr;

protected:
	void StartMissionDialog(TSoftClassPtr<UAstroMissionDialogWidget> MissionDialogWidgetClass);
	void ResolveMissionDialog(TSoftClassPtr<UAstroMissionDialogWidget> MissionDialogWidgetClass, const bool bSuccess);
	void StopMissionDialogs();

	void RegisterActorIndicator(AActor* Actor);
	void UnregisterActorIndicator(AActor* Actor);

private:
	UFUNCTION()
	void OnGameContextLoaded(const UAstroGameContextData* GameContext);

	UFUNCTION()
	void OnPreLoadMap(const FString& MapName);

protected:
	UAstroIndicatorWidgetManagerComponent* GetCachedIndicatorWidgetManagerComponent() const;

};


// ===========================================
// UAstroMissionBlueprintUtilities
// ===========================================

UCLASS()
class UAstroMissionBlueprintUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Adds a mission to the array, but replaces it if there's already one with the same widget. */
	UFUNCTION(BlueprintCallable)
	static void MergeMissionPayload(const FAstroMissionPayload& InMission, UPARAM(ref) TArray<FAstroMissionPayload>& InMissions);
};
