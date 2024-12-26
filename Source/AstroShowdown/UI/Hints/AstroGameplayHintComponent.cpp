/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroGameplayHintComponent.h"
#include "AstroCharacter.h"
#include "AstroGameplayHintSubsystem.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroGenericGameplayMessageTypes.h"
#include "AstroMissionComponent_PracticeMode.h"
#include "AstroTimeDilationSubsystem.h"
#include "LoadingScreenManager.h"
#include "SubsystemUtils.h"

UAstroGameplayHintComponent::UAstroGameplayHintComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroGameplayHintComponent::BeginPlay()
{
	Super::BeginPlay();

	// Caches the gameplay hint subsystem
	UAstroGameplayHintSubsystem* GameplayHintSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroGameplayHintSubsystem>(this);
	CachedGameplayHintSubsystem = GameplayHintSubsystem;
	check(CachedGameplayHintSubsystem.IsValid());
	if (!CachedGameplayHintSubsystem.IsValid())
	{
		return;
	}

	// Starts listening for gameplay-related events, so that we can display their hints
	const UWorld* World = GetWorld();
	const APlayerController* LocalPlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (AAstroCharacter* LocalAstroCharacter = LocalPlayerController ? CastChecked<AAstroCharacter>(LocalPlayerController->GetPawn()) : nullptr)
	{
		LocalAstroCharacter->OnAstroFocusStarted.AddUniqueDynamic(this, &UAstroGameplayHintComponent::OnPlayerFocusStarted);
		LocalAstroCharacter->OnAstroCharacterDied.AddUniqueDynamic(this, &UAstroGameplayHintComponent::OnPlayerDied);
		LocalAstroCharacter->OnAstroCharacterFatigued.AddUObject(this, &UAstroGameplayHintComponent::OnPlayerFatigued);
	}

	// Starts listening to the practice mode start message
	if (CachedGameplayHintSubsystem->CanDisplayHint(EAstroGameplayHintType::PracticeMode))
	{
		FGameplayMessageListenerParams<FPracticeModeGenericMessage> PracticeModeMessageListenerParams;
		PracticeModeMessageListenerParams.SetMessageReceivedCallback(this, &UAstroGameplayHintComponent::OnPracticeModeStart);
		PracticeModeStartMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_PracticeMode_Start, PracticeModeMessageListenerParams);
	}

	// Starts listening to the NPC revive message
	if (CachedGameplayHintSubsystem->CanDisplayHint(EAstroGameplayHintType::NPCRevive))
	{
		FGameplayMessageListenerParams<FAstroGenericNPCReviveMessage> NPCReviveMessageListenerParams;
		NPCReviveMessageListenerParams.SetMessageReceivedCallback(this, &UAstroGameplayHintComponent::OnNPCRevive);
		NPCReviveMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_NPC_Revive, NPCReviveMessageListenerParams);
	}

	// Starts listening for the loading screen visibility events, so that we can lock/unlock hints depending on the loading state
	if (ULoadingScreenManager* LoadingScreenManager = SubsystemUtils::GetGameInstanceSubsystem<ULoadingScreenManager>(this))
	{
		LoadingScreenManager->OnLoadingScreenVisibilityChangedDelegate().AddUObject(this, &UAstroGameplayHintComponent::OnLoadingScreenVisibilityChanged);
		OnLoadingScreenVisibilityChanged(LoadingScreenManager->GetLoadingScreenDisplayStatus());
	}
}

void UAstroGameplayHintComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	const UWorld* World = GetWorld();
	const APlayerController* LocalPlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (AAstroCharacter* LocalAstroCharacter = LocalPlayerController ? CastChecked<AAstroCharacter>(LocalPlayerController->GetPawn()) : nullptr)
	{
		LocalAstroCharacter->OnAstroFocusStarted.RemoveDynamic(this, &UAstroGameplayHintComponent::OnPlayerFocusStarted);
		LocalAstroCharacter->OnAstroCharacterDied.RemoveDynamic(this, &UAstroGameplayHintComponent::OnPlayerDied);
		LocalAstroCharacter->OnAstroCharacterFatigued.RemoveAll(this);
	}

	UGameplayMessageSubsystem::Get(this).UnregisterListener(PracticeModeStartMessageHandle);
	UGameplayMessageSubsystem::Get(this).UnregisterListener(NPCReviveMessageHandle);

	if (ULoadingScreenManager* LoadingScreenManager = SubsystemUtils::GetGameInstanceSubsystem<ULoadingScreenManager>(this))
	{
		LoadingScreenManager->OnLoadingScreenVisibilityChangedDelegate().RemoveAll(this);
	}
}

void UAstroGameplayHintComponent::OnPlayerFocusStarted()
{
	if (UAstroGameplayHintSubsystem* GameplayHintSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroGameplayHintSubsystem>(this))
	{
		GameplayHintSubsystem->QueueHint(EAstroGameplayHintType::PlayerFocus);
	}
}

void UAstroGameplayHintComponent::OnPlayerFatigued()
{
	if (UAstroGameplayHintSubsystem* GameplayHintSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroGameplayHintSubsystem>(this))
	{
		GameplayHintSubsystem->QueueHint(EAstroGameplayHintType::PlayerFatigue);
	}
}

void UAstroGameplayHintComponent::OnPlayerDied()
{
	if (UAstroGameplayHintSubsystem* GameplayHintSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroGameplayHintSubsystem>(this))
	{
		constexpr bool bLocked = true;
		GameplayHintSubsystem->AddLockInstigator(this);
		GameplayHintSubsystem->QueueHint(EAstroGameplayHintType::PlayerDeath);
	}
}

void UAstroGameplayHintComponent::OnLoadingScreenVisibilityChanged(const bool bIsLoadingScreenVisible)
{
	if (!CachedGameplayHintSubsystem.IsValid())
	{
		return;
	}

	ULoadingScreenManager* LoadingScreenManager = SubsystemUtils::GetGameInstanceSubsystem<ULoadingScreenManager>(this);
	if (!LoadingScreenManager)
	{
		return;
	}

	if (bIsLoadingScreenVisible)
	{
		CachedGameplayHintSubsystem->AddLockInstigator(LoadingScreenManager);
	}
	else
	{
		CachedGameplayHintSubsystem->RemoveLockInstigator(LoadingScreenManager);
	}
}

void UAstroGameplayHintComponent::OnPracticeModeStart(const FGameplayTag ChannelTag, const FPracticeModeGenericMessage& Message)
{
	if (CachedGameplayHintSubsystem.IsValid())
	{
		CachedGameplayHintSubsystem->QueueHint(EAstroGameplayHintType::PracticeMode);
	}
}

void UAstroGameplayHintComponent::OnNPCRevive(const FGameplayTag ChannelTag, const FAstroGenericNPCReviveMessage& Message)
{
	// Prevents from adding the revive hint if it's already been added during this game
	const UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this);
	const float CurrentRealTimeSeconds = TimeDilationSubsystem ? TimeDilationSubsystem->GetRealTimeSeconds() : 0.f;
	const float LastNPCHintDeltaTime = CurrentRealTimeSeconds - LastAddedReviveHintTimestamp;
	constexpr float ReviveHintDelay = 30.f;		// How long it takes to display each revive hint
	if (LastNPCHintDeltaTime < ReviveHintDelay)
	{
		return;
	}

	// Prevents this hint from spamming during practice mode
	const AAstroGameState* GameStateOwner = GetGameStateChecked<AAstroGameState>();
	if (GameStateOwner && GameStateOwner->IsInPracticeMode())
	{
		return;
	}

	if (CachedGameplayHintSubsystem.IsValid())
	{
		CachedGameplayHintSubsystem->QueueHint(EAstroGameplayHintType::NPCRevive);
	}

	LastAddedReviveHintTimestamp = CurrentRealTimeSeconds;
}
