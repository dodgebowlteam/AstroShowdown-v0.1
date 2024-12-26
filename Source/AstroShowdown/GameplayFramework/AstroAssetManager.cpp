/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
* 
* NOTE: Based on Epic Games Lyra's custom AssetManager. We've modified it a lot, but still they deserve the credit.
*/

#include "AstroAssetManager.h"
#include "AbilitySystemGlobals.h"
#include "AstroCampaignData.h"
#include "Engine/DataAsset.h"
#include "Engine/Engine.h"
#include "Misc/App.h"
#include "Misc/ScopedSlowTask.h"
#include "Stats/StatsMisc.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroAssetManager)

DECLARE_LOG_CATEGORY_EXTERN(LogAstroAssetManager, Log, All);
DEFINE_LOG_CATEGORY(LogAstroAssetManager);


namespace AstroAssetManagerStatics
{
	static bool bEnableAssetDumping = false;
	static FAutoConsoleVariableRef CVarEnableAssetDumping(
		TEXT("AstroAssets.EnableAssetDumping"),
		bEnableAssetDumping,
		TEXT("When enabled, the asset manager will store loaded assets, which can then be printed through the DumpLoadedAssets command."),
		ECVF_Default);

	static FAutoConsoleCommand CVarDumpLoadedAssets(
		TEXT("AstroAssets.DumpLoadedAssets"),
		TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
		FConsoleCommandDelegate::CreateStatic(UAstroAssetManager::DumpLoadedAssets));
}


UAstroAssetManager::UAstroAssetManager()
{
}

UAstroAssetManager& UAstroAssetManager::Get()
{
	check(GEngine);

	if (UAstroAssetManager* Singleton = Cast<UAstroAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogAstroAssetManager, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to AstroAssetManager!"));

	// Fatal error above prevents this from being called.
	return *NewObject<UAstroAssetManager>();
}

UObject* UAstroAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (ShouldLogAssetLoads())
		{
			LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
		}

		if (UAssetManager::IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

bool UAstroAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UAstroAssetManager::AddDebugLoadedAsset(const UObject* Asset)
{
	if (AstroAssetManagerStatics::bEnableAssetDumping)
	{
		if (ensureAlways(Asset))
		{
			FScopeLock DebugLoadedAssetsLock(&DebugLoadedAssetsCritical);
			DebugLoadedAssets.Add(Asset);
		}
	}
}

void UAstroAssetManager::DumpLoadedAssets()
{
	UE_LOG(LogAstroAssetManager, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	for (const UObject* LoadedAsset : Get().DebugLoadedAssets)
	{
		UE_LOG(LogAstroAssetManager, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogAstroAssetManager, Log, TEXT("... %d assets in loaded pool"), Get().DebugLoadedAssets.Num());
	UE_LOG(LogAstroAssetManager, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

void UAstroAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("UAstroAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

	{
		// Loads campaign data asset on startup
		GetCampaignData();
	}

	// NOTE: If we ever need any async startup jobs, they should be started here (check Lyra for a good example)
}

const UAstroCampaignData* UAstroAssetManager::GetCampaignData()
{
	return &GetOrLoadTypedGameData<UAstroCampaignData>(CampaignDataPath);
}

UPrimaryDataAsset* UAstroAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("AstroEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif
		UE_LOG(LogAstroAssetManager, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

		// This can be called recursively in the editor because it is called on demand from PostLoad so force a sync load for primary asset and async load the rest in that case
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				// This should always work
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		// It is not acceptable to fail to load any GameData asset. It will result in soft failures that are hard to diagnose.
		UE_LOG(LogAstroAssetManager, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return Asset;
}

void UAstroAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// Could route this to the early startup loading screen
}

#if WITH_EDITOR
void UAstroAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("AstroEditor", "BeginLoadingPIEData", "Loading PIE Data"));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

		const UAstroCampaignData* CampaignData = GetCampaignData();

		// Intentionally after GetGameData to avoid counting GameData time in this timer
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);

		// You could add preloading of anything else needed for the experience we'll be using here
		// (e.g., by grabbing the default experience from the world settings + the experience override in developer settings)
	}
}
#endif
