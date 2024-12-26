/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroUserSettingsSubsystem.h"
#include "AstroUserSettingsSaveGame.h"
#include "FMODStudio/Classes/FMODBlueprintStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "SubsystemUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroUserSettings, Log, All);
DEFINE_LOG_CATEGORY(LogAstroUserSettings);

namespace AstroStatics
{
	const int32 UserSettingsSaveGameSlotIndex = 0;
	static const FString UserSettingsSaveGameSlotName = "AstroUserSettings";
	static const FName PostProcessResolutionIndexParameterName = "ResolutionOptionIndex";
}

void UAstroUserSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Forcibly loads the volume buses
	if (const UAstroUserSettingsConfig* ProjectSettings = GetDefault<UAstroUserSettingsConfig>())
	{
		CachedGeneralVolumeBus = ProjectSettings->GeneralVolumeBus.LoadSynchronous();
		CachedMusicVolumeBus = ProjectSettings->MusicVolumeBus.LoadSynchronous();
		CachedPostProcessMPC = ProjectSettings->PostProcessMPC.LoadSynchronous();

		UE_CLOG(!CachedGeneralVolumeBus.Get(), LogAstroUserSettings, Error, TEXT("Invalid GeneralVolumeBus"));
		UE_CLOG(!CachedMusicVolumeBus.Get(), LogAstroUserSettings, Error, TEXT("Invalid MusicVolumeBus"));
		UE_CLOG(!CachedPostProcessMPC.Get(), LogAstroUserSettings, Error, TEXT("Invalid PostProcessMPC"));
	}

	// Loads the user settings file from the disk
	LoadUserSettingsSaveGame();
}

UAstroUserSettingsSubsystem* UAstroUserSettingsSubsystem::Get(UObject* WorldContextObject)
{
	return SubsystemUtils::GetGameInstanceSubsystem<UAstroUserSettingsSubsystem>(WorldContextObject);
}

void UAstroUserSettingsSubsystem::SaveUserSettings()
{
	if (bDirty)
	{
		UGameplayStatics::AsyncSaveGameToSlot(CachedSettingsSaveGame, AstroStatics::UserSettingsSaveGameSlotName, AstroStatics::UserSettingsSaveGameSlotIndex);
		bDirty = false;
	}
}

void UAstroUserSettingsSubsystem::ApplyUserAudioSettings()
{
	if (!ensure(CachedSettingsSaveGame))
	{
		UE_LOG(LogAstroUserSettings, Warning, TEXT("[%s] Couldn't find user settings."), __FUNCTION__);
		return;
	}

	if (ensureMsgf(CachedGeneralVolumeBus.Get(), TEXT("Invalid GeneralVolumeBus")))
	{
		UFMODBlueprintStatics::BusSetVolume(CachedGeneralVolumeBus.Get(), CachedSettingsSaveGame->GeneralVolume);
	}

	if (ensureMsgf(CachedMusicVolumeBus.Get(), TEXT("Invalid MusicVolumeBus")))
	{
		UFMODBlueprintStatics::BusSetVolume(CachedMusicVolumeBus.Get(), CachedSettingsSaveGame->MusicVolume);
	}
}

void UAstroUserSettingsSubsystem::ApplyUserResolutionSettings()
{
	if (!ensure(CachedSettingsSaveGame))
	{
		UE_LOG(LogAstroUserSettings, Warning, TEXT("[%s] Couldn't find user settings."), __FUNCTION__);
		return;
	}

	switch (CachedSettingsSaveGame->ResolutionMode)
	{
	case EAstroResolutionMode::Mid:
		UKismetSystemLibrary::ExecuteConsoleCommand(this, "r.ScreenPercentage.Default 83");					// 83% resolution
		break;
	case EAstroResolutionMode::High:
		UKismetSystemLibrary::ExecuteConsoleCommand(this, "r.ScreenPercentage.Default 100");				// 100% resolution (1080p)
		break;
	case EAstroResolutionMode::Ultra:
		UKismetSystemLibrary::ExecuteConsoleCommand(this, "r.ScreenPercentage.Default 133");				// 133% resolution (1440p)
		break;
	}

	if (CachedPostProcessMPC)
	{
		const uint32 ResolutionModeInt = static_cast<uint32>(CachedSettingsSaveGame->ResolutionMode);
		UKismetMaterialLibrary::SetScalarParameterValue(this, CachedPostProcessMPC, AstroStatics::PostProcessResolutionIndexParameterName, FMath::AsFloat(ResolutionModeInt));
	}
}

void UAstroUserSettingsSubsystem::LoadUserSettingsSaveGame()
{
	if (CachedSettingsSaveGame)
	{
		UE_LOG(LogAstroUserSettings, Warning, TEXT("[%s] User settings save game already exists."), __FUNCTION__);
		return;
	}

	USaveGame* UserSettingsSaveGame = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(AstroStatics::UserSettingsSaveGameSlotName, AstroStatics::UserSettingsSaveGameSlotIndex))
	{
		UserSettingsSaveGame = UGameplayStatics::LoadGameFromSlot(AstroStatics::UserSettingsSaveGameSlotName, AstroStatics::UserSettingsSaveGameSlotIndex);
	}
	else
	{
		UserSettingsSaveGame = UGameplayStatics::CreateSaveGameObject(UAstroUserSettingsSaveGame::StaticClass());
		UGameplayStatics::SaveGameToSlot(UserSettingsSaveGame, AstroStatics::UserSettingsSaveGameSlotName, AstroStatics::UserSettingsSaveGameSlotIndex);
	}

	CachedSettingsSaveGame = CastChecked<UAstroUserSettingsSaveGame>(UserSettingsSaveGame);

	ApplyUserAudioSettings();
	ApplyUserResolutionSettings();
}

void UAstroUserSettingsSubsystem::SetGeneralVolume(const float NewVolume)
{
	if (CachedSettingsSaveGame)
	{
		CachedSettingsSaveGame->GeneralVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	}

	bDirty = true;

	ApplyUserAudioSettings();
}

void UAstroUserSettingsSubsystem::SetMusicVolume(const float NewVolume)
{
	if (CachedSettingsSaveGame)
	{
		CachedSettingsSaveGame->MusicVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	}

	bDirty = true;

	ApplyUserAudioSettings();
}

void UAstroUserSettingsSubsystem::SetResolutionMode(const EAstroResolutionMode NewResolutionMode)
{
	if (CachedSettingsSaveGame)
	{
		CachedSettingsSaveGame->ResolutionMode = NewResolutionMode;
	}

	bDirty = true;

	ApplyUserResolutionSettings();
}

float UAstroUserSettingsSubsystem::GetGeneralVolume() const
{
	return CachedSettingsSaveGame ? CachedSettingsSaveGame->GeneralVolume : 0.0f;
}

float UAstroUserSettingsSubsystem::GetMusicVolume() const
{
	return CachedSettingsSaveGame ? CachedSettingsSaveGame->MusicVolume : 0.0f;
}

EAstroResolutionMode UAstroUserSettingsSubsystem::GetResolutionMode() const
{
	return CachedSettingsSaveGame ? CachedSettingsSaveGame->ResolutionMode : EAstroResolutionMode::Mid;
}
