/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroPlayerState.h"

#include "AbilitySystemComponent.h"
#include "AstroPlayerAttributeSet.h"
#include "HealthAttributeSet.h"

AAstroPlayerState::AAstroPlayerState()
{
	// From GAS docs:
	// - Anything that does not need to respawn: Set the Owner and Avatar actor be the same thing (e.g., AI enemies, buildings, world props, etc);
	// - Anything that does respawn: Set the Owner and Avatar be different so that the Ability System Component does not need to be saved off or
	// recreated or restored after a respawn;
	// 
	// PlayerState is the logical choice as it is replicated to all clients, whereas PlayerController is not. The downside is PlayerStates are always
	// relevant, so you can run into problems in games with lots of players (See the notes in GAS docs to understand what Fortnite did to solve this).
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);

	// The default values for the attributes are initialized by the Character during ASC setup
	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>("HealthAttributeSet");
	PlayerAttributeSet = CreateDefaultSubobject<UAstroPlayerAttributeSet>("PlayerAttributeSet");
}

void AAstroPlayerState::BeginPlay()
{
	Super::BeginPlay();
}
