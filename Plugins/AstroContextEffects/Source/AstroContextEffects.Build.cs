/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's Context Effects System, though we modified it to suit our needs.
*/

using UnrealBuildTool;

public class AstroContextEffects : ModuleRules
{
	public AstroContextEffects(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;				
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Chaos",
				"Core",
				"CoreUObject",
            	"DeveloperSettings",
				"Engine",
				"GameplayTags",
				"Niagara",
				"NiagaraCore",
                "PhysicsCore",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
