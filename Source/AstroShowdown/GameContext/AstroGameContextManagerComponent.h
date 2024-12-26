/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: This file was modified from Epic Games' Lyra project. They name those as "Experiences", but
* we called them "GameContexts" and added our own customizations.
*/

#pragma once

#include "Components/GameStateComponent.h"
#include "LoadingProcessInterface.h"

#include "AstroGameContextManagerComponent.generated.h"

namespace UE::GameFeatures { struct FResult; }

class UAstroGameContextData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAstroGameContextLoaded, const UAstroGameContextData* /*GameContext*/);

enum class EAstroGameContextLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	LoadingChaosTestingDelay,
	ExecutingActions,
	Loaded,
	Deactivating
};

/** Handles loading and managing the current game context. */
UCLASS()
class UAstroGameContextManagerComponent final : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:

	UAstroGameContextManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface

	// Tries to set the current GameContext, either a UI or gameplay one
	void SetCurrentGameContext(FPrimaryAssetId GameContextId);

	// Ensures the delegate is called once the GameContext has been loaded,
	// before others are called.
	// However, if the GameContext has already loaded, calls the delegate immediately.
	void CallOrRegister_OnGameContextLoaded_HighPriority(FOnAstroGameContextLoaded::FDelegate&& Delegate);

	// Ensures the delegate is called once the GameContext has been loaded
	// If the GameContext has already loaded, calls the delegate immediately
	void CallOrRegister_OnGameContextLoaded(FOnAstroGameContextLoaded::FDelegate&& Delegate);

	// Ensures the delegate is called once the GameContext has been loaded
	// If the GameContext has already loaded, calls the delegate immediately
	void CallOrRegister_OnGameContextLoaded_LowPriority(FOnAstroGameContextLoaded::FDelegate&& Delegate);

	// This returns the current GameContext if it is fully loaded, asserting otherwise
	// (i.e., if you called it too soon)
	const UAstroGameContextData* GetCurrentGameContextChecked() const;

	// Returns true if the GameContext is fully loaded
	bool IsGameContextLoaded() const;

private:
	UFUNCTION()
	void OnRep_CurrentGameContext();

	void StartGameContextLoad();
	void OnGameContextLoadComplete();
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	void OnGameContextFullLoadCompleted();

	void OnActionDeactivationCompleted();
	void OnAllActionsDeactivated();

private:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentGameContext)
	TObjectPtr<const UAstroGameContextData> CurrentGameContext;

	EAstroGameContextLoadState LoadState = EAstroGameContextLoadState::Unloaded;

	int32 NumGameFeaturePluginsLoading = 0;
	TArray<FString> GameFeaturePluginURLs;

	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;

	/**
	 * Delegate called when the GameContext has finished loading just before others
	 * (e.g., subsystems that set up for regular gameplay)
	 */
	FOnAstroGameContextLoaded OnGameContextLoaded_HighPriority;

	/** Delegate called when the GameContext has finished loading */
	FOnAstroGameContextLoaded OnGameContextLoaded;

	/** Delegate called when the GameContext has finished loading */
	FOnAstroGameContextLoaded OnGameContextLoaded_LowPriority;
};
