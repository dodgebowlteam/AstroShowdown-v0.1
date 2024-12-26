/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

using UnrealBuildTool;

public class AstroShowdownEditor : ModuleRules
{
	public AstroShowdownEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange( new string[] { "AstroShowdownEditor" } );

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "EditorFramework", "Engine", "InputCore", "UnrealEd" });

		PrivateDependencyModuleNames.AddRange(new string[] { "AstroShowdown", "Slate", "SlateCore" });
	}
}
