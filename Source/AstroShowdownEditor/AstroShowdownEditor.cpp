/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroShowdownEditor.h"

#include "Editor/UnrealEdEngine.h"
#include "Engine/GameInstance.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "AstroShowdownEditor"

DEFINE_LOG_CATEGORY(LogAstroShowdownEditor);

/**
 * FAstroShowdownEditorModule
 */
class FAstroShowdownEditorModule : public FDefaultGameModuleImpl
{
	typedef FAstroShowdownEditorModule ThisClass;

	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
	}

	virtual void ShutdownModule() override
	{
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_MODULE(FAstroShowdownEditorModule, AstroShowdownEditor);

#undef LOCTEXT_NAMESPACE
