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

#include "Subsystems/EngineSubsystem.h"
#include "AstroGameContextManager.generated.h"

/**
 * Manager for game contexts - primarily for arbitration between multiple PIE sessions
 */
UCLASS(MinimalAPI)
class UAstroGameContextManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	ASTROSHOWDOWN_API void OnPlayInEditorBegun();

	static void NotifyOfPluginActivation(const FString PluginURL);
	static bool RequestToDeactivatePlugin(const FString PluginURL);
#else
	static void NotifyOfPluginActivation(const FString PluginURL) {}
	static bool RequestToDeactivatePlugin(const FString PluginURL) { return true; }
#endif

private:
	// The map of requests to active count for a given game feature plugin
	// (to allow first in, last out activation management during PIE)
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
};
