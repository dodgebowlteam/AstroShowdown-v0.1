/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/DataAsset.h"
#include "Engine/SoftWorldReference.h"
#include "AstroRoomData.generated.h"

class UAstroInterstitialWidget;
class UGameFeatureAction;

UCLASS(BlueprintType, Const)
class UAstroRoomData : public UPrimaryDataAsset
{
	GENERATED_BODY()

#pragma region UPrimaryDataAsset
public:
	UAstroRoomData();

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif
#pragma endregion


public:
	/** Auto-generated unique ID that can be used to identify each room. */
	UPROPERTY(VisibleAnywhere, DuplicateTransient)
	FGuid RoomId;

	/** Player-facing room name. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText RoomDisplayName;

	/** World that represents the level. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FSoftWorldReference RoomLevel;

	/** Plays when the level is visited for the first time. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = UI)
	TSoftClassPtr<UAstroInterstitialWidget> IntroInterstitialScreen;

	/** List of actions to perform as this room is loaded/unloaded */
	UPROPERTY(EditAnywhere, Instanced, Category = Gameplay)
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	/** When enabled, the dash will be locked for the duration of this level. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Gameplay)
	uint8 bShouldLockDash : 1 = false;

	/** When enabled, the AstroDome will be lit up by default for this level. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Gameplay)
	uint8 bIsDomeLitUp : 1 = true;
};