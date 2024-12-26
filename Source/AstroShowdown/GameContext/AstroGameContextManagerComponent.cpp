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

#include "AstroGameContextManagerComponent.h"

#include "AstroGameContextData.h"
#include "AstroGameContextActionSet.h"
#include "AstroGameContextManager.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/AssetManager.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystemSettings.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroGameContextManagerComponent)

DECLARE_LOG_CATEGORY_EXTERN(LogAstroGameContextManager, Log, All);
DEFINE_LOG_CATEGORY(LogAstroGameContextManager);

namespace AstroConsoleVariables
{
	static float GameContextLoadRandomDelayMin = 0.0f;
	static FAutoConsoleVariableRef CVarGameContextLoadRandomDelayMin(
		TEXT("Astro.chaos.GameContextDelayLoad.MinSecs"),
		GameContextLoadRandomDelayMin,
		TEXT("This value (in seconds) will be added as a delay of load completion of the GameContext (along with the random value Astro.chaos.GameContextDelayLoad.RandomSecs)"),
		ECVF_Default);

	static float GameContextLoadRandomDelayRange = 0.0f;
	static FAutoConsoleVariableRef CVarGameContextLoadRandomDelayRange(
		TEXT("Astro.chaos.GameContextDelayLoad.RandomSecs"),
		GameContextLoadRandomDelayRange,
		TEXT("A random amount of time between 0 and this value (in seconds) will be added as a delay of load completion of the GameContext (along with the fixed value Astro.chaos.GameContextDelayLoad.MinSecs)"),
		ECVF_Default);

	float GetGameContextLoadDelayDuration()
	{
		return FMath::Max(0.0f, GameContextLoadRandomDelayMin + FMath::FRand() * GameContextLoadRandomDelayRange);
	}
}

UAstroGameContextManagerComponent::UAstroGameContextManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UAstroGameContextManagerComponent::SetCurrentGameContext(FPrimaryAssetId GameContextId)
{
	UAssetManager& AssetManager = UAssetManager::Get();
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(GameContextId);
	const UAstroGameContextData* GameContextData = Cast<UAstroGameContextData>(AssetPath.TryLoad());
	check(GameContextData);
	check(CurrentGameContext == nullptr);

	CurrentGameContext = GameContextData;
	StartGameContextLoad();
}

void UAstroGameContextManagerComponent::CallOrRegister_OnGameContextLoaded_HighPriority(FOnAstroGameContextLoaded::FDelegate&& Delegate)
{
	if (IsGameContextLoaded())
	{
		Delegate.Execute(CurrentGameContext);
	}
	else
	{
		OnGameContextLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}

void UAstroGameContextManagerComponent::CallOrRegister_OnGameContextLoaded(FOnAstroGameContextLoaded::FDelegate&& Delegate)
{
	if (IsGameContextLoaded())
	{
		Delegate.Execute(CurrentGameContext);
	}
	else
	{
		OnGameContextLoaded.Add(MoveTemp(Delegate));
	}
}

void UAstroGameContextManagerComponent::CallOrRegister_OnGameContextLoaded_LowPriority(FOnAstroGameContextLoaded::FDelegate&& Delegate)
{
	if (IsGameContextLoaded())
	{
		Delegate.Execute(CurrentGameContext);
	}
	else
	{
		OnGameContextLoaded_LowPriority.Add(MoveTemp(Delegate));
	}
}

const UAstroGameContextData* UAstroGameContextManagerComponent::GetCurrentGameContextChecked() const
{
	check(LoadState == EAstroGameContextLoadState::Loaded);
	check(CurrentGameContext != nullptr);
	return CurrentGameContext;
}

bool UAstroGameContextManagerComponent::IsGameContextLoaded() const
{
	return (LoadState == EAstroGameContextLoadState::Loaded) && (CurrentGameContext != nullptr);
}

void UAstroGameContextManagerComponent::OnRep_CurrentGameContext()
{
	StartGameContextLoad();
}

void UAstroGameContextManagerComponent::StartGameContextLoad()
{
	check(CurrentGameContext != nullptr);
	check(LoadState == EAstroGameContextLoadState::Unloaded);

	UE_LOG(LogAstroGameContextManager, Log, TEXT("GameContext: StartGameContextLoad(CurrentGameContext = %s)"), *CurrentGameContext->GetPrimaryAssetId().ToString());

	LoadState = EAstroGameContextLoadState::Loading;

	UAssetManager& AssetManager = UAssetManager::Get();

	TSet<FPrimaryAssetId> BundleAssetList;
	TSet<FSoftObjectPath> RawAssetList;

	BundleAssetList.Add(CurrentGameContext->GetPrimaryAssetId());
	for (const TObjectPtr<UAstroGameContextActionSet>& ActionSet : CurrentGameContext->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
		}
	}

	// Load assets associated with the GameContext
	// Example: LyraAssetManager.cpp
	// More about bundles: https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine
	TArray<FName> BundlesToLoad;
	//BundlesToLoad.Add(FAstroBundles::BundleName);

	//@TODO: Centralize this client/server stuff into the AstroAssetManager
	const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
	const bool bLoadClient = GIsEditor || (OwnerNetMode != NM_DedicatedServer);
	const bool bLoadServer = GIsEditor || (OwnerNetMode != NM_Client);
	if (bLoadClient)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
	}
	if (bLoadServer)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
	}

	TSharedPtr<FStreamableHandle> BundleLoadHandle = nullptr;
	if (BundleAssetList.Num() > 0)
	{
		BundleLoadHandle = AssetManager.ChangeBundleStateForPrimaryAssets(BundleAssetList.Array(), BundlesToLoad, {}, false, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	}

	TSharedPtr<FStreamableHandle> RawLoadHandle = nullptr;
	if (RawAssetList.Num() > 0)
	{
		RawLoadHandle = AssetManager.LoadAssetList(RawAssetList.Array(), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, TEXT("StartGameContextLoad()"));
	}

	// If both async loads are running, combine them
	TSharedPtr<FStreamableHandle> Handle = nullptr;
	if (BundleLoadHandle.IsValid() && RawLoadHandle.IsValid())
	{
		Handle = AssetManager.GetStreamableManager().CreateCombinedHandle({ BundleLoadHandle, RawLoadHandle });
	}
	else
	{
		Handle = BundleLoadHandle.IsValid() ? BundleLoadHandle : RawLoadHandle;
	}

	FStreamableDelegate OnAssetsLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnGameContextLoadComplete);
	if (!Handle.IsValid() || Handle->HasLoadCompleted())
	{
		// Assets were already loaded, call the delegate now
		FStreamableHandle::ExecuteDelegate(OnAssetsLoadedDelegate);
	}
	else
	{
		Handle->BindCompleteDelegate(OnAssetsLoadedDelegate);

		Handle->BindCancelDelegate(FStreamableDelegate::CreateLambda([OnAssetsLoadedDelegate]()
			{
				OnAssetsLoadedDelegate.ExecuteIfBound();
			}));
	}

	// This set of assets gets preloaded, but we don't block the start of the GameContext based on it
	TSet<FPrimaryAssetId> PreloadAssetList;
	//@TODO: Determine assets to preload (but not blocking-ly)
	if (PreloadAssetList.Num() > 0)
	{
		AssetManager.ChangeBundleStateForPrimaryAssets(PreloadAssetList.Array(), BundlesToLoad, {});
	}
}

void UAstroGameContextManagerComponent::OnGameContextLoadComplete()
{
	check(LoadState == EAstroGameContextLoadState::Loading);
	check(CurrentGameContext != nullptr);

	UE_LOG(LogAstroGameContextManager, Log, TEXT("GameContext: OnGameContextLoadComplete(CurrentGameContext = %s)"), *CurrentGameContext->GetPrimaryAssetId().ToString());

	// find the URLs for our GameFeaturePlugins - filtering out dupes and ones that don't have a valid mapping
	GameFeaturePluginURLs.Reset();

	auto CollectGameFeaturePluginURLs = [This = this](const UPrimaryDataAsset* Context, const TArray<FString>& FeaturePluginList)
	{
		for (const FString& PluginName : FeaturePluginList)
		{
			FString PluginURL;
			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, /*out*/ PluginURL))
			{
				This->GameFeaturePluginURLs.AddUnique(PluginURL);
			}
			else
			{
				ensureMsgf(false, TEXT("OnGameContextLoadComplete failed to find plugin URL from PluginName %s for GameContext %s - fix data, ignoring for this run"), *PluginName, *Context->GetPrimaryAssetId().ToString());
			}
		}
	};

	CollectGameFeaturePluginURLs(CurrentGameContext, CurrentGameContext->GameFeaturesToEnable);
	for (const TObjectPtr<UAstroGameContextActionSet>& ActionSet : CurrentGameContext->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
		}
	}

	// Load and activate the features	
	NumGameFeaturePluginsLoading = GameFeaturePluginURLs.Num();
	if (NumGameFeaturePluginsLoading > 0)
	{
		LoadState = EAstroGameContextLoadState::LoadingGameFeatures;
		for (const FString& PluginURL : GameFeaturePluginURLs)
		{
			UAstroGameContextManager::NotifyOfPluginActivation(PluginURL);
			UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete::CreateUObject(this, &ThisClass::OnGameFeaturePluginLoadComplete));
		}
	}
	else
	{
		OnGameContextFullLoadCompleted();
	}
}

void UAstroGameContextManagerComponent::OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result)
{
	// decrement the number of plugins that are loading
	NumGameFeaturePluginsLoading--;

	if (NumGameFeaturePluginsLoading == 0)
	{
		OnGameContextFullLoadCompleted();
	}
}

void UAstroGameContextManagerComponent::OnGameContextFullLoadCompleted()
{
	check(LoadState != EAstroGameContextLoadState::Loaded);

	// Insert a random delay for testing (if configured)
	if (LoadState != EAstroGameContextLoadState::LoadingChaosTestingDelay)
	{
		const float DelaySecs = AstroConsoleVariables::GetGameContextLoadDelayDuration();
		if (DelaySecs > 0.0f)
		{
			FTimerHandle DummyHandle;

			LoadState = EAstroGameContextLoadState::LoadingChaosTestingDelay;
			GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &ThisClass::OnGameContextFullLoadCompleted, DelaySecs, /*bLooping=*/ false);

			return;
		}
	}

	LoadState = EAstroGameContextLoadState::ExecutingActions;

	// Execute the actions
	FGameFeatureActivatingContext Context;

	// Only apply to our specific world context if set
	const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
	if (ExistingWorldContext)
	{
		Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}

	auto ActivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
	{
		for (UGameFeatureAction* Action : ActionList)
		{
			if (Action != nullptr)
			{
				//@TODO: The fact that these don't take a world are potentially problematic in client-server PIE
				// The current behavior matches systems like gameplay tags where loading and registering apply to the entire process,
				// but actually applying the results to actors is restricted to a specific world
				Action->OnGameFeatureRegistering();
				Action->OnGameFeatureLoading();
				Action->OnGameFeatureActivating(Context);
			}
		}
	};

	ActivateListOfActions(CurrentGameContext->Actions);
	for (const TObjectPtr<UAstroGameContextActionSet>& ActionSet : CurrentGameContext->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			ActivateListOfActions(ActionSet->Actions);
		}
	}

	LoadState = EAstroGameContextLoadState::Loaded;

	OnGameContextLoaded_HighPriority.Broadcast(CurrentGameContext);
	OnGameContextLoaded_HighPriority.Clear();

	OnGameContextLoaded.Broadcast(CurrentGameContext);
	OnGameContextLoaded.Clear();

	OnGameContextLoaded_LowPriority.Broadcast(CurrentGameContext);
	OnGameContextLoaded_LowPriority.Clear();
}

void UAstroGameContextManagerComponent::OnActionDeactivationCompleted()
{
	check(IsInGameThread());
	++NumObservedPausers;

	if (NumObservedPausers == NumExpectedPausers)
	{
		OnAllActionsDeactivated();
	}
}

void UAstroGameContextManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentGameContext);
}

void UAstroGameContextManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// deactivate any features this GameContext loaded
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		if (UAstroGameContextManager::RequestToDeactivatePlugin(PluginURL))
		{
			UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
		}
	}

	//@TODO: Ensure proper handling of a partially-loaded state too
	if (LoadState == EAstroGameContextLoadState::Loaded)
	{
		LoadState = EAstroGameContextLoadState::Deactivating;

		// Make sure we won't complete the transition prematurely if someone registers as a pauser but fires immediately
		NumExpectedPausers = INDEX_NONE;
		NumObservedPausers = 0;

		// Deactivate and unload the actions
		FGameFeatureDeactivatingContext Context(TEXT(""), [this](FStringView) { this->OnActionDeactivationCompleted(); });

		const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (ExistingWorldContext)
		{
			Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
		}

		auto DeactivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
			{
				for (UGameFeatureAction* Action : ActionList)
				{
					if (Action)
					{
						Action->OnGameFeatureDeactivating(Context);
						Action->OnGameFeatureUnregistering();
					}
				}
			};

		DeactivateListOfActions(CurrentGameContext->Actions);
		for (const TObjectPtr<UAstroGameContextActionSet>& ActionSet : CurrentGameContext->ActionSets)
		{
			if (ActionSet != nullptr)
			{
				DeactivateListOfActions(ActionSet->Actions);
			}
		}

		NumExpectedPausers = Context.GetNumPausers();

		if (NumExpectedPausers > 0)
		{
			UE_LOG(LogAstroGameContextManager, Error, TEXT("Actions that have asynchronous deactivation aren't fully supported yet in Astro GameContexts"));
		}

		if (NumExpectedPausers == NumObservedPausers)
		{
			OnAllActionsDeactivated();
		}
	}
}

bool UAstroGameContextManagerComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (LoadState != EAstroGameContextLoadState::Loaded)
	{
		OutReason = TEXT("GameContext still loading");
		return true;
	}
	else
	{
		return false;
	}
}

void UAstroGameContextManagerComponent::OnAllActionsDeactivated()
{
	// NOTE: We actually only deactivated and didn't fully unload. We should consider unloading, for memory perf.
	LoadState = EAstroGameContextLoadState::Unloaded;
	CurrentGameContext = nullptr;
}

