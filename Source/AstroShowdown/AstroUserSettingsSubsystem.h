/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FMODStudio/Classes/FMODBus.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "AstroUserSettingsSubsystem.generated.h"

class UAstroUserSettingsSaveGame;
class UMaterialParameterCollection;
enum class EAstroResolutionMode : uint8;


UCLASS(config = Game, defaultconfig, meta = (DisplayName = "AstroUserSettings"))
class UAstroUserSettingsConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UFMODBus> GeneralVolumeBus = nullptr;

	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UFMODBus> MusicVolumeBus = nullptr;

	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UMaterialParameterCollection> PostProcessMPC = nullptr;

};


UCLASS()
class UAstroUserSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region GameInstanceSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/** Static wrapper for getting this subsystem. */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "Get User Settings Subsystem"))
	static UAstroUserSettingsSubsystem* Get(UObject* WorldContextObject);
#pragma endregion


private:
	/** Cached the AstroUserSettingsSaveGame object. Avoids having to load it every time we want to manipulate the user's settings. */
	UPROPERTY(Transient)
	mutable TObjectPtr<UAstroUserSettingsSaveGame> CachedSettingsSaveGame = nullptr;

	UPROPERTY()
	TObjectPtr<UFMODBus> CachedGeneralVolumeBus = nullptr;

	UPROPERTY()
	TObjectPtr<UFMODBus> CachedMusicVolumeBus = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialParameterCollection> CachedPostProcessMPC = nullptr;

	/** Dirty bit for the user settings SaveGame object. */
	uint8 bDirty : 1 = false;

public:
	UFUNCTION(BlueprintCallable)
	void SaveUserSettings();

private:
	void ApplyUserAudioSettings();
	void ApplyUserResolutionSettings();
	void LoadUserSettingsSaveGame();

public:
	UFUNCTION(BlueprintCallable)
	void SetGeneralVolume(const float NewVolume);
	
	UFUNCTION(BlueprintCallable)
	void SetMusicVolume(const float NewVolume);

	UFUNCTION(BlueprintCallable)
	void SetResolutionMode(const EAstroResolutionMode NewResolutionMode);

	UFUNCTION(BlueprintPure)
	float GetGeneralVolume() const;

	UFUNCTION(BlueprintPure)
	float GetMusicVolume() const;

	UFUNCTION(BlueprintPure)
	EAstroResolutionMode GetResolutionMode() const;

};