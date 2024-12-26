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
#include "Engine/SoftWorldReference.h"
#include "LoadingProcessInterface.h"
#include "AstroRoomNavigationComponent.generated.h"


class AAstroRoomDoor;
class ACharacter;
class APlayerStart;
class FControlFlow;
class ULevelStreamingDynamic;

enum class EAstroRoomDoorDirection : uint8;


UCLASS()
class UAstroRoomNavigationComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroRoomNavigationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion


#pragma region ILoadingProcessInterface
public:
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;

	void SetShowLoadingScreen(const bool bShow) { bShouldShowLoadingScreen = bShow; }
private:
	bool bShouldShowLoadingScreen = true;
#pragma endregion


public:
	/** Which level to load if the campaign map doesn't have any loaded. */
	UPROPERTY(EditDefaultsOnly)
	FSoftWorldReference RootLevel;

	/** How long it takes to start MoveTo in MoveToWithTransition. */
	UPROPERTY(EditDefaultsOnly, meta = (ForceUnits = s))
	float MoveToTransitionDuration = 1.5f;

private:
	TSharedPtr<FControlFlow> RoomWorldLoadFlow;

	FDelegateHandle WaitForMainLevelLoadDelegateHandle;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoomLoaded, const FSoftWorldReference&);
	FOnRoomLoaded OnRoomLoaded;

public:
	/** Plays a loading transition, and then moves the player to the TargetWorld map. */
	UFUNCTION(BlueprintCallable)
	void MoveTo(const FSoftWorldReference& TargetWorld, float InTransitionDurationOverride = -1.f);

	/** Plays the loading transition and then starts MoveTo. */
	UFUNCTION(BlueprintCallable)
	void MoveToDirection(const EAstroRoomDoorDirection RoomDoorDirection);

private:
	struct FRoomLoadFlowStepSharedState
	{
		TWeakObjectPtr<ACharacter> Instigator = nullptr;
		FSoftWorldReference PreviousRoomWorld;
		TWeakObjectPtr<ULevelStreamingDynamic> NextRoomWorldStreaming = nullptr;
	};
	void RoomLoadFlowStep_UnloadCurrentRoom(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState);
	void RoomLoadFlowStep_PlayInterstitialScreen(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState, const FSoftWorldReference TargetWorld);
	void RoomLoadFlowStep_StartLoadingRoom(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState, const FSoftWorldReference TargetWorld);
	void RoomLoadFlowStep_WaitForMainLevelLoad(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState);
	void RoomLoadFlowStep_ProcessMainLevel(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState);
	void RoomLoadFlowStep_FinishMainLevelLoad(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState);

private:
	ULevelStreamingDynamic* FindCurrentRoomWorldStreaming() const;

	void LoadStartingLevel();
	void UnloadCurrentLevel();
	bool FindLevelEntryPoints(const ULevel* InLevel, OUT TArray<AAstroRoomDoor*>& OutDoors, OUT TArray<APlayerStart*>& OutPlayerStarts);

	void ActivateSection();
	void ActivateRoom();
	void DeactivateRoom();


public:
	FSoftWorldReference GetCurrentRoomWorldAsset() const;

private:
	UFUNCTION()
	void OnPreRestartCurrentLevel(bool& bShouldDeferRestart);

};
