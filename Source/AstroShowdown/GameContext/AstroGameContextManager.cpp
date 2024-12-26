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

#include "AstroGameContextManager.h"
#include "AstroGameContextManager.h"
#include "Engine/Engine.h"
#include "Subsystems/SubsystemCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroGameContextManager)

#if WITH_EDITOR

void UAstroGameContextManager::OnPlayInEditorBegun()
{
	ensure(GameFeaturePluginRequestCountMap.IsEmpty());
	GameFeaturePluginRequestCountMap.Empty();
}

void UAstroGameContextManager::NotifyOfPluginActivation(const FString PluginURL)
{
	if (GIsEditor)
	{
		UAstroGameContextManager* GameContextManagerSubsystem = GEngine->GetEngineSubsystem<UAstroGameContextManager>();
		check(GameContextManagerSubsystem);

		// Track the number of requesters who activate this plugin. Multiple load/activation requests are always allowed because concurrent requests are handled.
		int32& Count = GameContextManagerSubsystem->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		++Count;
	}
}

bool UAstroGameContextManager::RequestToDeactivatePlugin(const FString PluginURL)
{
	if (GIsEditor)
	{
		UAstroGameContextManager* GameContextManagerSubsystem = GEngine->GetEngineSubsystem<UAstroGameContextManager>();
		check(GameContextManagerSubsystem);

		// Only let the last requester to get this far deactivate the plugin
		int32& Count = GameContextManagerSubsystem->GameFeaturePluginRequestCountMap.FindChecked(PluginURL);
		--Count;

		if (Count == 0)
		{
			GameContextManagerSubsystem->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}

		return false;
	}

	return true;
}

#endif
