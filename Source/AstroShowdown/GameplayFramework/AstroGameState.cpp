/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroGameState.h"
#include "AstroCoreDelegates.h"
#include "AstroGameContextManagerComponent.h"
#include "AstroMissionComponent_PracticeMode.h"

AAstroGameState::AAstroGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GameContextManagerComponent = CreateDefaultSubobject<UAstroGameContextManagerComponent>(TEXT("GameContextManagerComponent"));
}

void AAstroGameState::BeginPlay()
{
	Super::BeginPlay();

	FAstroCoreDelegates::OnPreRestartCurrentLevel.AddUObject(this, &AAstroGameState::OnPreRestartCurrentLevel);
}

void AAstroGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	FAstroCoreDelegates::OnPreRestartCurrentLevel.RemoveAll(this);
}

void AAstroGameState::InitializeGame()
{
	if (ensure(CurrentGameState == EAstroGameState::Inactive))
	{
		CurrentGameState = EAstroGameState::Initializing;
		OnAstroGameInitialized.Broadcast();
		OnAstroGameInitializedStatic.Broadcast();
	}
}

void AAstroGameState::ActivateGame()
{
	if (ensure(CurrentGameState == EAstroGameState::Initializing))
	{
		CurrentGameState = EAstroGameState::Active;
		OnAstroGameActivated.Broadcast();
		OnAstroGameActivatedStatic.Broadcast();
	}
}

void AAstroGameState::ResolveGame()
{
	CurrentGameState = EAstroGameState::Resolved;
	OnAstroGameResolved.Broadcast();
	OnAstroGameResolvedStatic.Broadcast();
	OnPostAstroGameResolved.Broadcast();
}

void AAstroGameState::BroadcastBeginLevelCue()
{
	if (ensure(CurrentGameState == EAstroGameState::Active))
	{
		OnAstroGameBeginLevelCue.Broadcast();
	}
}

void AAstroGameState::ResetGameState()
{
	CurrentGameState = EAstroGameState::Inactive;
	OnAstroGameReset.Broadcast();
}

void AAstroGameState::OnPreRestartCurrentLevel(bool& bShouldDeferRestart)
{
	OnPreAstroWorldRestart.Broadcast();
}

bool AAstroGameState::IsInPracticeMode() const
{
	return FindComponentByClass<UAstroMissionComponent_PracticeMode>() != nullptr;
}
