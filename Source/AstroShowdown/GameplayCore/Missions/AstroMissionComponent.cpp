/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroMissionComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AstroCharacter.h"
#include "AstroGameContextManagerComponent.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroIndicatorWidgetManagerComponent.h"
#include "AstroMissionDialogWidget.h"
#include "ControlFlowManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"


UAstroMissionComponent::UAstroMissionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// NOTE: Currently the MissionComponents are being destroyed by an external Actor, so this needs to be enabled
	bAllowAnyoneToDestroyMe = true;
}

void UAstroMissionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Resets the game state before beginning.
	// NOTE: Assumes that the mission starts from an Inactive game state and ends in a Resolved state.
	GameStateOwner = GetGameStateChecked<AAstroGameState>();
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->ResetGameState();
	}

	// Listen for the game context load to complete
	// NOTE: This delegate is on a component with the same lifetime as this one, so no need to unhook it in
	UAstroGameContextManagerComponent* GameContextComponent = GameStateOwner->FindComponentByClass<UAstroGameContextManagerComponent>();
	check(GameContextComponent);
	GameContextComponent->CallOrRegister_OnGameContextLoaded_HighPriority(FOnAstroGameContextLoaded::FDelegate::CreateUObject(this, &UAstroMissionComponent::OnGameContextLoaded));

	// Listens to PreLoadMap. Prevents MissionDialogWidget leaks on restarts.
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UAstroMissionComponent::OnPreLoadMap);
}

void UAstroMissionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Stops executing the MissionControlFlow
	if (MissionControlFlow.IsValid() && MissionControlFlow->IsRunning())
	{
		MissionControlFlow->CancelFlow();
	}

	// Hides all MissionDialogs that could have been spawned by the MissionControlFlow
	// NOTE: This should take place after cancelling, to ensure that the widget deactivation callbacks won't continue the MissionControlFlow
	StopMissionDialogs();

	// Stops listening to PreLoadMap
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UAstroMissionComponent::OnPreLoadMap);
}

void UAstroMissionComponent::StartMissionDialog(TSoftClassPtr<UAstroMissionDialogWidget> MissionDialogWidgetClass)
{
	FAstroMissionPayload MissionPayload;
	MissionPayload.MissionWidgetClass = MissionDialogWidgetClass;
	MissionPayload.Status = EAstroMissionStatus::InProgress;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Mission_Start, MissionPayload);
}

void UAstroMissionComponent::ResolveMissionDialog(TSoftClassPtr<UAstroMissionDialogWidget> MissionDialogWidgetClass, const bool bSuccess)
{
	FAstroMissionPayload MissionPayload;
	MissionPayload.MissionWidgetClass = MissionDialogWidgetClass;
	MissionPayload.Status = EAstroMissionStatus::Success;

	const FGameplayTag StatusTag = bSuccess ? AstroGameplayTags::Gameplay_Mission_Success : AstroGameplayTags::Gameplay_Mission_Fail;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(StatusTag, MissionPayload);
}

void UAstroMissionComponent::StopMissionDialogs()
{
	FAstroMissionPayload MissionPayload;
	MissionPayload.Status = EAstroMissionStatus::Fail;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Mission_Stop, MissionPayload);
}

void UAstroMissionComponent::RegisterActorIndicator(AActor* Actor)
{
	if (UAstroIndicatorWidgetManagerComponent* IndicatorManager = GetCachedIndicatorWidgetManagerComponent())
	{
		FAstroIndicatorWidgetSettings IndicatorSettings;
		IndicatorSettings.Owner = Actor;
		IndicatorSettings.SocketName = "IndicatorSocket";
		IndicatorSettings.bOffscreenActorOnly = false;		// Displays indicator while the actor is on screen
		IndicatorSettings.bShouldCheckAliveState = false;	// Does not update the indicator's dead/alive status
		IndicatorManager->RegisterActorIndicator(IndicatorSettings);
	}
}

void UAstroMissionComponent::UnregisterActorIndicator(AActor* Actor)
{
	if (UAstroIndicatorWidgetManagerComponent* IndicatorManager = GetCachedIndicatorWidgetManagerComponent())
	{
		IndicatorManager->UnregisterActorIndicator(Actor);
	}
}

void UAstroMissionComponent::OnGameContextLoaded(const UAstroGameContextData* GameContext)
{
	MissionControlFlow = CreateMissionControlFlow();
	MissionControlFlow->ExecuteFlow();
}

void UAstroMissionComponent::OnPreLoadMap(const FString& MapName)
{
	// Prevents the flow from executing during map loads (i.e., restarting)
	// NOTE: Prevents leaking Widgets that for whatever reason can't be removed properly from their layer during map loads
	if (MissionControlFlow.IsValid())
	{
		MissionControlFlow->CancelFlow();
	}

	// Hides all MissionDialogs that could have been spawned by the MissionControlFlow
	// NOTE: This should take place after cancelling, to ensure that the widget deactivation callbacks won't continue the MissionControlFlow
	StopMissionDialogs();
}

UAstroIndicatorWidgetManagerComponent* UAstroMissionComponent::GetCachedIndicatorWidgetManagerComponent() const
{
	if (!CachedIndicatorComponent.IsValid())
	{
		CachedIndicatorComponent = GameStateOwner->GetComponentByClass<UAstroIndicatorWidgetManagerComponent>();
	}

	return CachedIndicatorComponent.Get();
}

void UAstroMissionBlueprintUtilities::MergeMissionPayload(const FAstroMissionPayload& InMission, UPARAM(ref)TArray<FAstroMissionPayload>& InMissions)
{
	// If there's a mission with the same widget, replaces with the new one
	for (int32 Index = 0; Index < InMissions.Num(); Index++)
	{
		if (InMissions[Index].MissionWidgetClass == InMission.MissionWidgetClass)
		{
			InMissions[Index] = InMission;
			return;
		}
	}

	// Otherwise, adds it
	InMissions.Add(InMission);
}
