/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroMissionComponent_DefeatEnemies.h"
#include "AstroAbilitySystem.h"
#include "AstroCharacter.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroUtilitiesBlueprintLibrary.h"
#include "AstroWorldManagerSubsystem.h"
#include "ControlFlowManager.h"
#include "Kismet/GameplayStatics.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"

UAstroMissionComponent_DefeatEnemies::UAstroMissionComponent_DefeatEnemies(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroMissionComponent_DefeatEnemies::BeginPlay()
{
	Super::BeginPlay();

	FGameplayMessageListenerParams<FAstroGameInitializeRequestMessage> InitializeRequestMessageListenerParams;
	InitializeRequestMessageListenerParams.SetMessageReceivedCallback(this, &UAstroMissionComponent_DefeatEnemies::OnInitializeRequestReceived);
	InitializeRequestMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_Game_RequestInitialize, InitializeRequestMessageListenerParams);
}

void UAstroMissionComponent_DefeatEnemies::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UGameplayMessageSubsystem::Get(this).UnregisterListener(InitializeRequestMessageHandle);

	if (UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManager->OnAliveEnemyCountChanged.RemoveAll(this);
		AstroWorldManager->OnInitializedAllEnemies.RemoveDynamic(this, &UAstroMissionComponent_DefeatEnemies::OnInitializedAllEnemies);
	}

	if (AAstroCharacter* LocalCharacter = UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(this))
	{
		LocalCharacter->OnAstroCharacterDied.RemoveDynamic(this, &UAstroMissionComponent_DefeatEnemies::OnPlayerDied);
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->OnAstroGameActivatedStatic.RemoveAll(this);
		GameStateOwner->OnAstroGameResolvedStatic.RemoveAll(this);
	}

	// Unregister the dash input event, if it's registered
	if (DashInputEventDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this))
		{
			const FGameplayTagContainer DashInputTagContainer = FGameplayTagContainer(AstroGameplayTags::InputTag_AstroDash);
			LocalPlayerASC->RemoveGameplayEventTagContainerDelegate(DashInputTagContainer, DashInputEventDelegateHandle);
			DashInputEventDelegateHandle.Reset();
		}
	}
}

TSharedPtr<FControlFlow> UAstroMissionComponent_DefeatEnemies::CreateMissionControlFlow()
{
	// Creates the mission's control flow
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("DefeatEnemiesFlow"));

	if (bIsTutorialEnabled)
	{
		Flow.QueueStep(TEXT("Activation Onboarding"), this, &UAstroMissionComponent_DefeatEnemies::FlowStep_MissionActivationOnboarding);
	}

	Flow.QueueStep(TEXT("Defeat Enemies Mission"), this, &UAstroMissionComponent_DefeatEnemies::FlowStep_DefeatEnemiesMission);

	return Flow.AsShared();
}

void UAstroMissionComponent_DefeatEnemies::FlowStep_MissionActivationOnboarding(FControlFlowNodeRef SubFlow)
{
	if (!ensureMsgf(ActivationTotemClass.IsValid(), TEXT("Invalid ActivationTotemClass")))
	{
		SubFlow->ContinueFlow();
		return;
	}

	TWeakObjectPtr<AActor> ActivationTotem = UGameplayStatics::GetActorOfClass(this, ActivationTotemClass.Get());
	if (!ensureMsgf(ActivationTotem.IsValid(), TEXT("Could not find ActivationTotem")))
	{
		SubFlow->ContinueFlow();
		return;
	}

	if (!GameStateOwner.IsValid())
	{
		SubFlow->ContinueFlow();
		return;
	}

	// Shows indicator on top of the activation totem
	RegisterActorIndicator(ActivationTotem.Get());
	
	// Waits for the level to be initialized, then goes to the next step
	GameStateOwner->OnAstroGameInitializedStatic.AddUObject(this, &UAstroMissionComponent_DefeatEnemies::OnGameInitialized, SubFlow.ToWeakPtr(), ActivationTotem);
}

void UAstroMissionComponent_DefeatEnemies::FlowStep_DefeatEnemiesMission(FControlFlowNodeRef SubFlow)
{
	UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this);
	check(AstroWorldManager);
	if (AstroWorldManager)
	{
		AstroWorldManager->OnAliveEnemyCountChanged.AddUObject(this, &UAstroMissionComponent_DefeatEnemies::OnEnemyCountChanged);
		AstroWorldManager->OnInitializedAllEnemies.AddUniqueDynamic(this, &UAstroMissionComponent_DefeatEnemies::OnInitializedAllEnemies);
	}

	if (AAstroCharacter* LocalCharacter = UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(this))
	{
		LocalCharacter->OnAstroCharacterDied.AddUniqueDynamic(this, &UAstroMissionComponent_DefeatEnemies::OnPlayerDied);
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->OnAstroGameResolvedStatic.AddUObject(this, &UAstroMissionComponent_DefeatEnemies::OnGameResolved, SubFlow.ToWeakPtr());
		GameStateOwner->OnAstroGameActivatedStatic.AddUObject(this, &UAstroMissionComponent_DefeatEnemies::OnGameActivated, SubFlow.ToWeakPtr());
	}
}

void UAstroMissionComponent_DefeatEnemies::InitializeGame()
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->InitializeGame();
	}
}

void UAstroMissionComponent_DefeatEnemies::BroadcastBeginLevelCue()
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->BroadcastBeginLevelCue();
	}
}

void UAstroMissionComponent_DefeatEnemies::OnInitializeRequestReceived(const FGameplayTag ChannelTag, const FAstroGameInitializeRequestMessage& Message)
{
	if (!ChannelTag.MatchesTagExact(AstroGameplayTags::Gameplay_Message_Game_RequestInitialize))
	{
		return;
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->InitializeGame();
	}
}

void UAstroMissionComponent_DefeatEnemies::OnGameInitialized(TWeakPtr<FControlFlowNode> SubFlow, TWeakObjectPtr<AActor> WeakActivationTotem)
{
	UnregisterActorIndicator(WeakActivationTotem.Get());

	if (ensure(SubFlow.IsValid()))
	{
		SubFlow.Pin()->ContinueFlow();
	}

	if (GameStateOwner.IsValid())
	{
		GameStateOwner->OnAstroGameInitializedStatic.RemoveAll(this);
	}
}

void UAstroMissionComponent_DefeatEnemies::OnGameActivated(TWeakPtr<FControlFlowNode> SubFlow)
{
	if (bIsTutorialEnabled)
	{
		StartMissionDialog(DefeatEnemiesOnboardingDialog);
	}

	if (bShowDashTutorial)
	{
		// Starts listening to the dash input event
		UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this);
		if (ensureMsgf(LocalPlayerASC, TEXT("Failed to find an AbilitySystemComponent for the local player")))
		{
			const FGameplayTagContainer DashInputTagContainer = FGameplayTagContainer(AstroGameplayTags::InputTag_AstroDash);
			DashInputEventDelegateHandle = LocalPlayerASC->AddGameplayEventTagContainerDelegate(DashInputTagContainer, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UAstroMissionComponent_DefeatEnemies::OnDashInputEventReceived));
		}

		// Shows the dash onboarding mission dialog
		StartMissionDialog(DashOnboardingDialog);
	}
}

void UAstroMissionComponent_DefeatEnemies::OnGameResolved(TWeakPtr<FControlFlowNode> SubFlow)
{
	if (bIsTutorialEnabled)
	{
		ResolveMissionDialog(DefeatEnemiesOnboardingDialog, true);
	}

	// Shows the victory screen
	if (ensure(!VictoryScreenClass.GetAssetName().IsEmpty()))
	{
		if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
		{
			constexpr bool bSuspendInputUntilComplete = false;
			RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Menu, bSuspendInputUntilComplete, VictoryScreenClass);
		}
	}

	if (SubFlow.IsValid())
	{
		SubFlow.Pin()->ContinueFlow();
	}
}

void UAstroMissionComponent_DefeatEnemies::OnPlayerDied()
{
	// Stops listening to enemy count changes, thus making it impossible to clear the level while the player is dead
	if (UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManager->OnAliveEnemyCountChanged.RemoveAll(this);
	}
}

void UAstroMissionComponent_DefeatEnemies::OnEnemyCountChanged(const int32 OldCount, const int32 NewCount)
{
	if (NewCount != 0)
	{
		return;
	}

	// If the game is active AND the enemy count reached 0, we'll try to resolve it
	if (GameStateOwner.IsValid() && GameStateOwner->IsGameActive())
	{
		GameStateOwner->ResolveGame();
	}
}

void UAstroMissionComponent_DefeatEnemies::OnInitializedAllEnemies()
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->ActivateGame();
	}
}

void UAstroMissionComponent_DefeatEnemies::OnDashInputEventReceived(const FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	if (!MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_AstroDash))
	{
		return;
	}

	// Finishes the dash tutorial with success
	constexpr bool bSuccess = true;
	ResolveMissionDialog(DashOnboardingDialog, bSuccess);

	// Unregister the dash input event
	if (UAbilitySystemComponent* LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this))
	{
		if (DashInputEventDelegateHandle.IsValid())
		{
			const FGameplayTagContainer DashInputTagContainer = FGameplayTagContainer(AstroGameplayTags::InputTag_AstroDash);
			LocalPlayerASC->RemoveGameplayEventTagContainerDelegate(DashInputTagContainer, DashInputEventDelegateHandle);
			DashInputEventDelegateHandle.Reset();
		}
	}
}
