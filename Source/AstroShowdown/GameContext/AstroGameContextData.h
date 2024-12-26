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

#include "Engine/DataAsset.h"
#include "AstroGameContextData.generated.h"

class UGameFeatureAction;
class UAstroGameContextActionSet;

/**
 * Contains data that defines a game context
 */
UCLASS(BlueprintType, Const)
class UAstroGameContextData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UAstroGameContextData();

	//~UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UPrimaryDataAsset interface
#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif
	//~End of UPrimaryDataAsset interface

public:
	// List of Game Feature Plugins this game context wants to have active
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	// List of actions to perform as this game context is loaded/activated/deactivated/unloaded
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	// List of additional action sets to compose into this game context
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<TObjectPtr<UAstroGameContextActionSet>> ActionSets;
};
