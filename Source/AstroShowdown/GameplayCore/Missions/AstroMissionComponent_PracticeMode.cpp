/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroMissionComponent_PracticeMode.h"
#include "AstroAbilitySystem.h"
#include "AstroCampaignPersistenceSubsystem.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroRoomNavigationSubsystem.h"
#include "AstroWorldManagerSubsystem.h"
#include "ControlFlowConditionalLoop.h"
#include "ControlFlowManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"
#include "TimerManager.h"

UAstroMissionComponent_PracticeMode::UAstroMissionComponent_PracticeMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroMissionComponent_PracticeMode::BeginPlay()
{
	Super::BeginPlay();

	FGameplayMessageListenerParams<FAstroGameInitializeRequestMessage> InitializeRequestMessageListenerParams;
	InitializeRequestMessageListenerParams.SetMessageReceivedCallback(this, &UAstroMissionComponent_PracticeMode::OnInitializeRequestReceived);
	InitializeRequestMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_Game_RequestInitialize, InitializeRequestMessageListenerParams);
}

void UAstroMissionComponent_PracticeMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UGameplayMessageSubsystem::Get(this).UnregisterListener(InitializeRequestMessageHandle);

	if (UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManager->OnAliveEnemyCountChanged.RemoveAll(this);
		AstroWorldManager->OnInitializedAllEnemies.RemoveDynamic(this, &UAstroMissionComponent_PracticeMode::OnInitializedAllEnemies);
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->OnAstroGameActivatedStatic.RemoveAll(this);
		GameStateOwner->OnAstroGameResolvedStatic.RemoveAll(this);
	}

	RemovePracticeModeGameplayEffect();
	UnregisterExitPracticeModeInputEvent();
}

TSharedPtr<FControlFlow> UAstroMissionComponent_PracticeMode::CreateMissionControlFlow()
{
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("PracticeModeFlow"))
	.QueueStep(TEXT("Practice Mode Unlock"), this, &UAstroMissionComponent_PracticeMode::FlowStep_PracticeModeUnlocked)
	.Loop([this](TSharedRef<FConditionalLoop> PracticeModeLoop)
	{
		PracticeModeLoop->RunLoopFirst()
			.QueueDelay(0.5f, TEXT("Practice Mode Mission Delay"))		// Prevents resetting the state too quick after the game is resolved
			.QueueStep(TEXT("Practice Mode Mission"), this, &UAstroMissionComponent_PracticeMode::FlowStep_PracticeModeMission);
		return EConditionalLoopResult::RunLoop;
	});
	return Flow.AsShared();
}

void UAstroMissionComponent_PracticeMode::FlowStep_PracticeModeUnlocked(FControlFlowNodeRef SubFlow)
{
	// If the player is visiting the room for the first time, show the unlock dialog after a few seconds.
	const UWorld* World = GetWorld();
	const UAstroRoomNavigationSubsystem* RoomNavigationSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroRoomNavigationSubsystem>(this);
	const UAstroCampaignPersistenceSubsystem* CampaignPersistenceSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroCampaignPersistenceSubsystem>(this);
	const UAstroRoomData* CurrentRoom = RoomNavigationSubsystem ? RoomNavigationSubsystem->GetCurrentRoom() : nullptr;
	if (World && CampaignPersistenceSubsystem && CurrentRoom && !CampaignPersistenceSubsystem->IsRoomCompleted(CurrentRoom))
	{
		auto ShowUnlockDialog = [WeakThis = TWeakObjectPtr<UAstroMissionComponent_PracticeMode>(this)]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->StartMissionDialog(WeakThis->PracticeModeUnlockedDialog);
			}
		};

		FTimerHandle UnlockMessageTimerHandle;
		World->GetTimerManager().SetTimer(OUT UnlockMessageTimerHandle, ShowUnlockDialog, 3.0f /*InRate*/, false /*InbLoop*/);
	}

	SubFlow->ContinueFlow();
}

void UAstroMissionComponent_PracticeMode::FlowStep_PracticeModeMission(FControlFlowNodeRef SubFlow)
{
	UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this);
	check(AstroWorldManager);
	if (AstroWorldManager)
	{
		AstroWorldManager->OnInitializedAllEnemies.AddUniqueDynamic(this, &UAstroMissionComponent_PracticeMode::OnInitializedAllEnemies);
	}

	// Rebinds all game state delegates
	// NOTE: We're removing and adding because this FlowStep is inside a loop, so the SubFlow ptr will be GC'd in the next iteration
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->OnAstroGameResolvedStatic.RemoveAll(this);
		GameStateOwner->OnAstroGameActivatedStatic.RemoveAll(this);

		GameStateOwner->OnAstroGameResolvedStatic.AddUObject(this, &UAstroMissionComponent_PracticeMode::OnGameResolved, SubFlow.ToWeakPtr());
		GameStateOwner->OnAstroGameActivatedStatic.AddUObject(this, &UAstroMissionComponent_PracticeMode::OnGameActivated, SubFlow.ToWeakPtr());
	}
}

void UAstroMissionComponent_PracticeMode::InitializeGame()
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->InitializeGame();
	}
}

void UAstroMissionComponent_PracticeMode::BroadcastBeginLevelCue()
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->BroadcastBeginLevelCue();
	}
}

void UAstroMissionComponent_PracticeMode::RegisterExitPracticeModeInputEvent()
{
	if (ExitPracticeModeInputEventHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this);
	if (ensureMsgf(LocalPlayerASC, TEXT("Failed to find an AbilitySystemComponent for the local player")))
	{
		const FGameplayTagContainer PracticeModeInputTagContainer = FGameplayTagContainer(AstroGameplayTags::InputTag_ExitPracticeMode);
		ExitPracticeModeInputEventHandle = LocalPlayerASC->AddGameplayEventTagContainerDelegate(PracticeModeInputTagContainer, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UAstroMissionComponent_PracticeMode::OnExitPracticeModeInputEventReceived));
	}
}

void UAstroMissionComponent_PracticeMode::UnregisterExitPracticeModeInputEvent()
{
	if (!ExitPracticeModeInputEventHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this))
	{
		const FGameplayTagContainer PracticeModeInputTagContainer = FGameplayTagContainer(AstroGameplayTags::InputTag_ExitPracticeMode);
		LocalPlayerASC->RemoveGameplayEventTagContainerDelegate(PracticeModeInputTagContainer, ExitPracticeModeInputEventHandle);
		ExitPracticeModeInputEventHandle.Reset();
	}
}

void UAstroMissionComponent_PracticeMode::AddPracticeModeGameplayEffect()
{
	if (ActivePracticeModeEffect.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this))
	{
		const FGameplayEffectSpecHandle PracticeModeEffectSpec = LocalPlayerASC->MakeOutgoingSpec(PracticeModeGE, 0.f, {});
		if (PracticeModeEffectSpec.Data.IsValid())
		{
			ActivePracticeModeEffect = LocalPlayerASC->ApplyGameplayEffectSpecToSelf(*PracticeModeEffectSpec.Data.Get());
		}
	}
}

void UAstroMissionComponent_PracticeMode::RemovePracticeModeGameplayEffect()
{
	if (!ActivePracticeModeEffect.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this))
	{
		LocalPlayerASC->RemoveActiveGameplayEffect(ActivePracticeModeEffect);
		ActivePracticeModeEffect.Invalidate();
	}
}

void UAstroMissionComponent_PracticeMode::OnInitializeRequestReceived(const FGameplayTag ChannelTag, const FAstroGameInitializeRequestMessage& Message)
{
	if (!ChannelTag.MatchesTagExact(AstroGameplayTags::Gameplay_Message_Game_RequestInitialize))
	{
		return;
	}

	if (GameStateOwner.IsValid())
	{
		if (GameStateOwner->IsGameResolved())
		{
			GameStateOwner->ResetGameState();
		}

		GameStateOwner->InitializeGame();
	}
}

void UAstroMissionComponent_PracticeMode::OnGameInitialized(TWeakPtr<FControlFlowNode> SubFlow)
{
	if (ensure(SubFlow.IsValid()))
	{
		SubFlow.Pin()->ContinueFlow();
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->OnAstroGameInitializedStatic.RemoveAll(this);
	}
}

void UAstroMissionComponent_PracticeMode::OnGameActivated(TWeakPtr<FControlFlowNode> SubFlow)
{
	AddPracticeModeGameplayEffect();
	RegisterExitPracticeModeInputEvent();

	// Broadcasts the Practice Mode start message
	FPracticeModeGenericMessage PracticeModeGenericMessage;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_PracticeMode_Start, PracticeModeGenericMessage);

	// Shows the practice mode exit dialog
	StartMissionDialog(PracticeModeExitDialog);
}

void UAstroMissionComponent_PracticeMode::OnGameResolved(TWeakPtr<FControlFlowNode> SubFlow)
{
	ResolveMissionDialog(PracticeModeExitDialog, true /*bSuccess*/);
	
	UnregisterExitPracticeModeInputEvent();
	RemovePracticeModeGameplayEffect();

	if (SubFlow.IsValid())
	{
		SubFlow.Pin()->ContinueFlow();
	}
}

void UAstroMissionComponent_PracticeMode::OnInitializedAllEnemies()
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->ActivateGame();
	}
}

void UAstroMissionComponent_PracticeMode::OnExitPracticeModeInputEventReceived(const FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	if (!MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_ExitPracticeMode))
	{
		return;
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->ResolveGame();
	}
}
