/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
* 
* NOTE: Based on Epic Games Lyra's custom AssetManager. We've modified it a lot, but still they deserve the credit.
*/

#pragma once

#include "Engine/AssetManager.h"
#include "Templates/SubclassOf.h"
#include "AstroAssetManager.generated.h"

class UPrimaryDataAsset;
class UAstroCampaignData;

/** This class is used by setting 'AssetManagerClassName' in DefaultEngine.ini. */
UCLASS(Config = Game)
class UAstroAssetManager : public UAssetManager
{
	GENERATED_BODY()

#pragma region UAssetManager interface
public:
	UAstroAssetManager();

	/** Returns the AssetManager singleton object. */
	static UAstroAssetManager& Get();

	virtual void StartInitialLoading() override;

#if WITH_EDITOR
	virtual void PreBeginPIE(bool bStartSimulate) override;
#endif
#pragma endregion

protected:
	/** Global campaign data asset to use. */
	UPROPERTY(Config)
	TSoftObjectPtr<UAstroCampaignData> CampaignDataPath;

private:
	/** Loaded version of the game data. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	/** Assets loaded and tracked by the asset manager. */
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> DebugLoadedAssets;

	/** Used for a scope lock when modifying the list of load assets. */
	FCriticalSection DebugLoadedAssetsCritical;

public:
	/** Returns the asset referenced by a TSoftObjectPtr.  This will synchronously load the asset if it's not already loaded. */
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	/** Returns the subclass referenced by a TSoftClassPtr.  This will synchronously load the asset if it's not already loaded. */
	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	/** Asynchronously loads all assets of the given class. */
	template <typename T>
	void LoadAllAssetsOfClass()
	{
		LoadPrimaryAssetsWithType(T::StaticClass()->GetFName());
	}

	/** Returns the campaign data used by the game. */
	const UAstroCampaignData* GetCampaignData();

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const* ResultPtr = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*ResultPtr);
		}

		// Does a blocking load if needed
		return *CastChecked<const GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

	UPrimaryDataAsset* LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);

	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	static bool ShouldLogAssetLoads();

	// Thread safe way of adding a loaded asset to keep in memory.
	void AddDebugLoadedAsset(const UObject* Asset);

private:
	/** Called periodically during loads, could be used to feed the status to a loading screen. */
	void UpdateInitialGameContentLoadPercent(float GameContentPercent);

public:
	/** Logs all assets currently loaded and tracked by the asset manager. */
	static void DumpLoadedAssets();
};


template<typename AssetType>
AssetType* UAstroAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddDebugLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template<typename AssetType>
TSubclassOf<AssetType> UAstroAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddDebugLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}
