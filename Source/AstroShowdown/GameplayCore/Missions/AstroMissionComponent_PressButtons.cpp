/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroMissionComponent_PressButtons.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AstroAbilitySystem.h"
#include "AstroBall.h"
#include "AstroCharacter.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroMissionDialogWidget.h"
#include "AstroUtilitiesBlueprintLibrary.h"
#include "AstroWorldManagerSubsystem.h"
#include "ControlFlowManager.h"
#include "ControlFlowConditionalLoop.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"
#include "TimerManager.h"


UAstroMissionComponent_PressButtons::UAstroMissionComponent_PressButtons(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroMissionComponent_PressButtons::BeginPlay()
{
	Super::BeginPlay();
}

void UAstroMissionComponent_PressButtons::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Stops listening for enemy count changes
	if (UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManager->OnAliveEnemyCountChanged.RemoveAll(this);
		AstroWorldManager->OnEnemyRegistered.RemoveAll(this);
		AstroWorldManager->OnEnemyUnregistered.RemoveAll(this);
	}
}

TSharedPtr<FControlFlow> UAstroMissionComponent_PressButtons::CreateMissionControlFlow()
{
	// Allocates the state for the BallOnboardingLoop and keeps it alive through lambda closures
	// NOTE: This avoids the need to create this state on the component itself, thus keeping the scope tighter
	TSharedPtr<bool> BallOnboardingLoopState = MakeShared<bool>(false);

	// Creates the mission's control flow
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("ButtonMissionFlow"));

	if (bIsTutorialEnabled)
	{
		Flow.QueueDelay(4.f, TEXT("Wait"))		// Waits a bit, to ensure that the loading screen has finished loading
			.QueueStep(TEXT("Movement Onboarding"), this, &UAstroMissionComponent_PressButtons::FlowStep_MovementOnboarding)
			.Loop([this, BallOnboardingLoopState](TSharedRef<FConditionalLoop> BallOnboardingLoop)
			{
				BallOnboardingLoop->RunLoopFirst()
					.QueueStep("Grab Onboarding", this, &UAstroMissionComponent_PressButtons::FlowStep_GrabBallOnboarding)
					.QueueStep("Throw Onboarding", this, &UAstroMissionComponent_PressButtons::FlowStep_ThrowBallOnboarding, BallOnboardingLoopState)
					.QueueStep("Throw Onboarding", this, &UAstroMissionComponent_PressButtons::FlowStep_ThrowBallOnboardingCleanup);

				return BallOnboardingLoopState && *BallOnboardingLoopState ? EConditionalLoopResult::LoopFinished : EConditionalLoopResult::RunLoop;
			});
	}

	Flow.QueueStep(TEXT("Press Buttons Mission"), this, &UAstroMissionComponent_PressButtons::FlowStep_OpenDoorMission);

	return Flow.AsShared();
}

void UAstroMissionComponent_PressButtons::FlowStep_MovementOnboarding(FControlFlowNodeRef SubFlow)
{
	// 0. Shows the movement onboarding dialog
	StartMissionDialog(MovementOnboardingDialog);

	// 1. Waits for the player to move
	if (TWeakObjectPtr<UAbilitySystemComponent> LocalPlayerASC = UAstroAbilitySystemBlueprintLibrary::FindLocalPlayerAbilitySystemComponent(this); LocalPlayerASC.IsValid())
	{
		// NOTE: Stores the DelegateHandle as a SharedPtr, so we can unregister it from the lambda
		// while avoiding a state variable in the component to store it, as it's specific to this flow step.
		TSharedPtr<FDelegateHandle> TransientMoveInputDelegateHandle = MakeShared<FDelegateHandle>();
		const TSoftClassPtr<UAstroMissionDialogWidget> LocalMovementOnboardingDialog = MovementOnboardingDialog;
		auto HideDialogOnMovement = [SubFlowPtr = SubFlow.ToSharedPtr(), WeakThis = TWeakObjectPtr<UAstroMissionComponent_PressButtons>(this), TransientMoveInputDelegateHandle, LocalPlayerASC, LocalMovementOnboardingDialog](FGameplayTag MatchingTag, const FGameplayEventData* Payload)
		{
			// 2. Once the player moves, broadcasts the success event
			if (WeakThis.IsValid())
			{
				constexpr bool bSuccess = true;
				WeakThis->ResolveMissionDialog(LocalMovementOnboardingDialog, bSuccess);
			}

			// 3. Unregisters the movement listener
			if (ensure(TransientMoveInputDelegateHandle.IsValid()))
			{
				if (LocalPlayerASC.IsValid())
				{
					const FGameplayTagContainer InputTagContainer{ AstroGameplayTags::InputTag_Move };
					const FDelegateHandle LocalMoveInputDelegateHandle = *TransientMoveInputDelegateHandle;
					LocalPlayerASC->RemoveGameplayEventTagContainerDelegate(InputTagContainer, LocalMoveInputDelegateHandle);
				}
			}

			// 4. Moves to the next step
			if (SubFlowPtr.IsValid())
			{
				SubFlowPtr->ContinueFlow();
			}
		};

		const FGameplayTagContainer InputTagContainer { AstroGameplayTags::InputTag_Move };
		*TransientMoveInputDelegateHandle = LocalPlayerASC->AddGameplayEventTagContainerDelegate(InputTagContainer, FGameplayEventTagMulticastDelegate::FDelegate::CreateWeakLambda(this, HideDialogOnMovement));
	}
}

void UAstroMissionComponent_PressButtons::FlowStep_GrabBallOnboarding(FControlFlowNodeRef SubFlow)
{
	// 0. Shows the grab onboarding dialog
	StartMissionDialog(GrabOnboardingDialog);

	// 1. Waits for the player to grab a ball
	if (TWeakObjectPtr<AAstroCharacter> LocalAstroCharacter = UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(this); LocalAstroCharacter.IsValid())
	{
		const TSoftClassPtr<UAstroMissionDialogWidget> LocalGrabOnboardingDialog = GrabOnboardingDialog;

		auto OnBallPickedUp = [WeakThis = TWeakObjectPtr<UAstroMissionComponent_PressButtons>(this), SubFlowPtr = SubFlow.ToSharedPtr(), LocalAstroCharacter, LocalGrabOnboardingDialog]()
		{
			if (!ensure(WeakThis.IsValid()))
			{
				return;
			}

			// 2. Once the player grabs the ball, sends the success event to the dialog
			constexpr bool bSuccess = true;
			WeakThis->ResolveMissionDialog(LocalGrabOnboardingDialog, bSuccess);

			// 3. Moves to the next step
			if (SubFlowPtr.IsValid())
			{
				SubFlowPtr->ContinueFlow();
			}
			
			// 4. Stops listening to the ball pickup event
			// NOTE: We have to do this at the end of the callback, to avoid cleaning up captured shared ptrs.
			if (LocalAstroCharacter.IsValid())
			{
				LocalAstroCharacter->OnBallPickedUp.RemoveAll(WeakThis.Get());
			}
		};
		LocalAstroCharacter->OnBallPickedUp.AddWeakLambda(this, OnBallPickedUp);
	}
}

void UAstroMissionComponent_PressButtons::FlowStep_ThrowBallOnboarding(FControlFlowNodeRef SubFlow, TSharedPtr<bool> SuccessPtr)
{
	// Resolves the step by either failing or success-ing it
	TWeakObjectPtr<UAstroMissionComponent_PressButtons> WeakThis = this;
	auto ResolveStep = [WeakThis, SuccessPtr, SubFlowPtr = SubFlow.ToSharedPtr()](const bool bSuccess, const bool bResolveDialog = false)
	{
		if (ensureMsgf(SuccessPtr.IsValid(), TEXT("Success state is invalid. Something went awfully wrong.")))
		{
			*SuccessPtr = bSuccess;
		}

		if (WeakThis.IsValid() && bResolveDialog)
		{
			WeakThis->ResolveMissionDialog(WeakThis->ThrowOnboardingDialog, bSuccess);
		}

		if (SubFlowPtr.IsValid())
		{
			SubFlowPtr->ContinueFlow();
		}
	};

	TWeakObjectPtr<AAstroCharacter> LocalAstroCharacter = UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(this);
	if (!LocalAstroCharacter.IsValid())
	{
		constexpr bool bSuccess = false;
		ResolveStep(bSuccess);
		return;
	}

	// NOTE: This step assumes the player is holding a ball. If it's not, then we'll forcibly fail it.
	if (!LocalAstroCharacter->IsHoldingBall())
	{
		constexpr bool bSuccess = false;
		ResolveStep(bSuccess);
		return;
	}

	// 0. Shows the throw onboarding dialog
	StartMissionDialog(ThrowOnboardingDialog);

	// 1. Listens to ball drop and throw events, which may fail or pass this onboarding step, respectively.
	if (TWeakObjectPtr<AAstroBall> BallPickup = LocalAstroCharacter->GetBallPickup(); BallPickup.IsValid())
	{
		ensureMsgf(!LastBallPickup.IsValid(), TEXT("We expect this to be cleared out after this step."));
		LastBallPickup = BallPickup;

		BallPickup->OnAstroBallDropped.AddWeakLambda(this, [ResolveStep]()
		{
			// 2a. Player dropped the ball, so we'll fail the step and clear the events
			constexpr bool bSuccess = false;
			constexpr bool bResolveDialog = true;
			ResolveStep(bSuccess, bResolveDialog);
		});

		BallPickup->OnAstroBallThrown.AddWeakLambda(this, [ResolveStep]()
		{
			// 2b. Player threw the ball, so we'll pass the step and clear the events
			constexpr bool bSuccess = true;
			constexpr bool bResolveDialog = true;
			ResolveStep(bSuccess, bResolveDialog);
		});
	}
}

void UAstroMissionComponent_PressButtons::FlowStep_ThrowBallOnboardingCleanup(FControlFlowNodeRef SubFlow)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		SubFlow->CancelFlow();
		return;
	}

	// Clears events that are set on the player's ball pickup during the ThrowBallOnboarding step
	// NOTE: We're delaying it by one tick to avoid ContinueFlow-order shenanigans
	TWeakObjectPtr<UAstroMissionComponent_PressButtons> WeakThis = this;
	World->GetTimerManager().SetTimerForNextTick([WeakThis, SubFlowPtr = SubFlow.ToSharedPtr()]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->ClearLastBallPickup();
		}
		if (SubFlowPtr.IsValid())
		{
			SubFlowPtr->ContinueFlow();
		}
	});

}

void UAstroMissionComponent_PressButtons::FlowStep_OpenDoorMission(FControlFlowNodeRef SubFlow)
{
	UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this);
	if (!AstroWorldManager || !GameStateOwner.IsValid())
	{
		SubFlow->CancelFlow();
		return;
	}

	// 0. Shows the mission's dialog and indicators
	if (bIsTutorialEnabled)
	{
		StartMissionDialog(MissionOnboardingDialog);

		// Adds indicators to all the existing buttons
		for (TWeakObjectPtr<AActor> Enemy : AstroWorldManager->GetActiveEnemies())
		{
			RegisterActorIndicator(Enemy.Get());
		}

		// Adds indicator on button register, and removes on unregister
		AstroWorldManager->OnEnemyRegistered.AddUObject(this, &UAstroMissionComponent_PressButtons::OnEnemyRegistered);
		AstroWorldManager->OnEnemyUnregistered.AddUObject(this, &UAstroMissionComponent_PressButtons::OnEnemyUnregistered);
	}

	// 1. Resolves the level once all buttons are pressed
	AstroWorldManager->OnAliveEnemyCountChanged.AddUObject(this, &UAstroMissionComponent_PressButtons::OnEnemyAliveCountChanged, SubFlow.ToSharedPtr());
}

void UAstroMissionComponent_PressButtons::ClearLastBallPickup()
{
	// Clears the last ball pickup reference and stops listening to its events
	if (LastBallPickup.IsValid())
	{
		LastBallPickup->OnAstroBallDropped.RemoveAll(this);
		LastBallPickup->OnAstroBallThrown.RemoveAll(this);
		LastBallPickup = nullptr;
	}
}

void UAstroMissionComponent_PressButtons::OnEnemyAliveCountChanged(const int32 OldCount, const int32 NewCount, FControlFlowNodePtr SubFlowPtr)
{
	if (NewCount == 0)
	{
		if (AAstroGameState* AstroGameState = GetGameStateChecked<AAstroGameState>())
		{
			AstroGameState->ResolveGame();
		}

		if (UAstroWorldManagerSubsystem* AstroWorldManager = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
		{
			AstroWorldManager->OnAliveEnemyCountChanged.RemoveAll(this);
		}

		constexpr bool bSuccess = true;
		ResolveMissionDialog(MissionOnboardingDialog, bSuccess);

		if (SubFlowPtr.IsValid())
		{
			SubFlowPtr->ContinueFlow();
		}
	}
}

void UAstroMissionComponent_PressButtons::OnEnemyRegistered(AActor* RegisteredEnemy)
{
	RegisterActorIndicator(RegisteredEnemy);
}

void UAstroMissionComponent_PressButtons::OnEnemyUnregistered(AActor* UnregisteredEnemy)
{
	UnregisterActorIndicator(UnregisteredEnemy);
}
