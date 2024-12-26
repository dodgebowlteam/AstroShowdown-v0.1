/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

using UnrealBuildTool;

public class AstroShowdown : ModuleRules
{
	public AstroShowdown(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "AstroShowdown",
                "AstroShowdown/AsyncActions",
                "AstroShowdown/GameContext",
                "AstroShowdown/GameFeatures",
                "AstroShowdown/GameplayAbilitySystem",
                "AstroShowdown/GameplayCore",
                "AstroShowdown/GameplayCore/Missions",
                "AstroShowdown/GameplayCore/RoomNavigation",
                "AstroShowdown/GameplayFramework",
                "AstroShowdown/UI",
                "AstroShowdown/Utils",
            }
        );

        PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
        });

		PrivateDependencyModuleNames.AddRange(new string[] {
            "AIModule",
            "AnimToTexture",
			"CommonGame",
            "CommonInput",
			"CommonLoadingScreen",
            "CommonUI",
			"ControlFlows",
            "DeveloperSettings",
            "EnhancedInput",
			"FMODStudio",
			"GameFeatures",
            "GameplayAbilities",
            "GameplayMessageRuntime",
            "GameplayTags",
            "GameplayTasks",
            "ModularGameplay",
			"ModularGameplayActors",
            "NiagaraCore",
            "Niagara",
            "StructUtils",
            "UMG"
        });
	}
}
