/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's Context Effects System, though we modified it to suit our needs.
*/

#include "Modules/ModuleManager.h"

/**
 * Implements the FAstroContextEffectsModule module.
 */
class FAstroContextEffectsModule : public IModuleInterface
{
public:
	FAstroContextEffectsModule();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


FAstroContextEffectsModule::FAstroContextEffectsModule()
{
}

void FAstroContextEffectsModule::StartupModule()
{
}

void FAstroContextEffectsModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FAstroContextEffectsModule, AstroContextEffects);
