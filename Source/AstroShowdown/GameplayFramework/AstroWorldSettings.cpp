/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroWorldSettings.h"
#include "AstroAssetManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroWorldSettings)

DECLARE_LOG_CATEGORY_EXTERN(LogAstroWorldSettings, Log, All);
DEFINE_LOG_CATEGORY(LogAstroWorldSettings);

AAstroWorldSettings::AAstroWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FPrimaryAssetId AAstroWorldSettings::GetWorldGameContext() const
{
	FPrimaryAssetId Result = UAstroAssetManager::Get().GetPrimaryAssetIdForPath(WorldGameContext.ToSoftObjectPath());
	if (!Result.IsValid())
	{
		UE_LOG(LogAstroWorldSettings, Error, TEXT("%s.WorldGameContext is %s but that failed to resolve into an asset ID (you might need to add a path to the Asset Rules in your game feature plugin or project settings"),
			*GetPathNameSafe(this), *WorldGameContext.ToString());
	}
	return Result;
}
