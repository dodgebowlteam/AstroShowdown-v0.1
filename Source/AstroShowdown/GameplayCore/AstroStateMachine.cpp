/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroStateMachine.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilityTask_AstroDash.h"
#include "AstroCharacter.h"
#include "AstroController.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroTimeDilationSubsystem.h"
#include "GameplayAbility_AstroDash.h"
#include "SubsystemUtils.h"
#include "TimerManager.h"


void UAstroStateMachine::SwitchState(AAstroCharacter* InOwner, TSubclassOf<UAstroStateBase> NewStateClass)
{
	if (UAbilitySystemComponent* OwnerAbilitySystemComponent = InOwner->GetAbilitySystemComponent())
	{
		FGameplayAbilitySpec StateAbilitySpec { NewStateClass };
		FGameplayAbilitySpecHandle StateAbilitySpecHandle = OwnerAbilitySystemComponent->GiveAbilityAndActivateOnce(StateAbilitySpec);
	}
}


////////////////////////////////////////////////////////////////
// UAstroStateBase
////////////////////////////////////////////////////////////////

UAstroStateBase::UAstroStateBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Forces the state to auto-cancel overlapping states, thus calling their ExitState method
	const FGameplayTag& AstroStateGameplayTag = AstroGameplayTags::AstroState;
	AbilityTags.AddTag(AstroStateGameplayTag);
	CancelAbilitiesWithTag.AddTag(AstroStateGameplayTag);
}

void UAstroStateBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo->AvatarActor.IsValid())
	{
		EnterState(Cast<AAstroCharacter>(ActorInfo->AvatarActor));
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAstroStateBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo->AvatarActor.IsValid())
	{
		ExitState(Cast<AAstroCharacter>(ActorInfo->AvatarActor));			// We need to call this before Super, or it won't get executed
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAstroStateBase::EnterState_Implementation(AAstroCharacter* InOwner)
{
	Owner = InOwner;
	ensureMsgf(Owner.Get(), TEXT("::ERROR: (%s) Invalid owner"), ANSI_TO_TCHAR(__FUNCTION__));

	if (UAbilitySystemComponent* InOwnerAbilitySystemComponent = InOwner->GetAbilitySystemComponent())
	{
		// Listens for the generic data gameplay event, allowing AstroCharacter to pass data to AstroStates
		const FGameplayTag& GenericStateGameplayEventTag = AstroGameplayTags::GameplayEvent_GenericStateDataEvent;
		FGameplayEventMulticastDelegate& GenericStateDataDelegate = InOwnerAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(GenericStateGameplayEventTag);
		GenericStateDataDelegate.AddUObject(this, &UAstroStateBase::OnGameplayEventReceivedInternal);

		// Listens for input tag events, allowing AstroStates to handle them
		const FGameplayTagContainer InputTagContainer { AstroGameplayTags::InputTag };
		InputEventDelegateHandle = InOwnerAbilitySystemComponent->AddGameplayEventTagContainerDelegate(InputTagContainer, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UAstroStateBase::OnInputEventReceivedInternal));
	}
}

void UAstroStateBase::ExitState_Implementation(AAstroCharacter* InOwner)
{
	// Stops listening for the generic data gameplay event
	if (UAbilitySystemComponent* InOwnerAbilitySystemComponent = InOwner->GetAbilitySystemComponent())
	{
		const FGameplayTag& GenericStateGameplayEventTag = AstroGameplayTags::GameplayEvent_GenericStateDataEvent;
		FGameplayEventMulticastDelegate& GenericStateDataDelegate = InOwnerAbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(GenericStateGameplayEventTag);
		GenericStateDataDelegate.RemoveAll(this);

		if (InputEventDelegateHandle.IsValid())
		{
			const FGameplayTagContainer InputTagContainer { AstroGameplayTags::InputTag };
			InOwnerAbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(InputTagContainer, InputEventDelegateHandle);
		}
	}
}

void UAstroStateBase::OnGameplayEventReceivedInternal(const FGameplayEventData* Payload)
{
	if (Payload)
	{
		OnGameplayEventReceived(*Payload);
	}
}

void UAstroStateBase::OnInputEventReceivedInternal(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	if (Payload)
	{
		OnInputEventReceived(MatchingTag, *Payload);
	}
}


////////////////////////////////////////////////////////////////
// UAstroState_Idle
////////////////////////////////////////////////////////////////

void UAstroState_Idle::EnterState_Implementation(AAstroCharacter* InOwner)
{
	Super::EnterState_Implementation(InOwner);

	ensure(FatiguedGE && StaminaRechargeGE);

	AAstroController* AstroController = InOwner ? InOwner->GetController<AAstroController>() : nullptr;
	if (!AstroController)
	{
		return;
	}

	// Always grants movement input
	AstroController->GrantMoveInput();
	Owner->EnableInteraction();

	// Listens for the begin level cue, and activates focus when it's received
	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameBeginLevelCue.AddDynamic(this, &UAstroState_Idle::OnAstroGameBeginLevelCue);
	}

	// If the player has no stamina and has not won yet, it becomes fatigued
	if (UAbilitySystemComponent* AbilitySystemComponent = InOwner->GetAbilitySystemComponent())
	{
		const FGameplayTag& WinningGameplayTag = AstroGameplayTags::AstroState_Win;
		if (AbilitySystemComponent->HasMatchingGameplayTag(WinningGameplayTag))
		{
			// Do nothing, player has already won, so it's not fatigued and shouldn't be able to focus
		}
		if (const bool bIsPlayerFatigued = InOwner->GetMaxStamina() > 0.f && InOwner->GetCurrentStamina() == 0.f)
		{
			// If stamina is depleted, applies fatigue to player
			AddFatigue();
		}
	}
}

void UAstroState_Idle::ExitState_Implementation(AAstroCharacter* InOwner)
{
	Super::ExitState_Implementation(InOwner);

	// Stops listening for the begin level cue
	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameBeginLevelCue.RemoveDynamic(this, &UAstroState_Idle::OnAstroGameBeginLevelCue);
	}

	// Removes all inputs related to this state
	if (AAstroController* AstroController = InOwner ? InOwner->GetController<AAstroController>() : nullptr)
	{
		AstroController->UnGrantInput(AstroGameplayTags::InputTag_Move);
		AstroController->UnGrantInput(AstroGameplayTags::InputTag_Focus);
		InOwner->DisableInteraction();
	}

	RemoveFatigue();
}

void UAstroState_Idle::OnInputEventReceived_Implementation(FGameplayTag MatchingTag, const FGameplayEventData& Payload)
{
	Super::OnInputEventReceived_Implementation(MatchingTag, Payload);

	if (MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_Focus))
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = Owner->GetAbilitySystemComponent())
		{
			const FGameplayTag& WinningGameplayTag = AstroGameplayTags::AstroState_Win;
			if (const bool HasWon = AbilitySystemComponent->HasMatchingGameplayTag(WinningGameplayTag))
			{
				// Do nothing
			}
			else if (AbilitySystemComponent->HasMatchingGameplayTag(AstroGameplayTags::AstroState_Fatigued))
			{
				// Do nothing
			}
			else
			{
				Owner->SwitchState(Owner->FocusStateAbilityClass);
			}
		}
	}
}

void UAstroState_Idle::OnStaminaChanged(float OldValue, float NewValue)
{
	if (const bool bIsStaminaFullyRecharged = Owner->GetCurrentStamina() == Owner->GetMaxStamina())
	{
		if (AAstroController* AstroController = Owner->GetController<AAstroController>())
		{
			AstroController->GrantFocusInput();
		}

		RemoveFatigue();
	}
}

void UAstroState_Idle::OnAstroGameBeginLevelCue()
{
	if (Owner.IsValid())
	{
		Owner->SwitchState(Owner->FocusStateAbilityClass);
	}
}

void UAstroState_Idle::AddFatigue()
{
	UAbilitySystemComponent* AbilitySystemComponent = Owner.IsValid() ? Owner->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Applies fatigue debuff
	if (const FGameplayEffectSpecHandle FatiguedSpecHandle = MakeOutgoingGameplayEffectSpec(FatiguedGE, 0.f); FatiguedSpecHandle.IsValid())
	{
		ActiveFatigueEffect = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*FatiguedSpecHandle.Data.Get());
	}

	// Applies stamina recharge buff
	if (const FGameplayEffectSpecHandle StaminaRechargeSpecHandle = MakeOutgoingGameplayEffectSpec(StaminaRechargeGE, 0.f); StaminaRechargeSpecHandle.IsValid())
	{
		ActiveStaminaRechargeEffect = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*StaminaRechargeSpecHandle.Data.Get());
	}

	// Starts listening to stamina changes. Removes fatigue and recharge once stamina is fully recharged.
	Owner->OnStaminaChangedDelegate.AddDynamic(this, &UAstroState_Idle::OnStaminaChanged);
	Owner->OnStaminaRechargeCounterChangedDelegate.AddDynamic(this, &UAstroState_Idle::OnStaminaRechargeChanged_BP);
}

void UAstroState_Idle::RemoveFatigue()
{
	if (ActiveFatigueEffect.IsValid())
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = Owner.IsValid() ? Owner->GetAbilitySystemComponent() : nullptr)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveFatigueEffect);
			AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaRechargeEffect);
			ActiveFatigueEffect.Invalidate();
			ActiveStaminaRechargeEffect.Invalidate();
		}

		if (Owner.IsValid())
		{
			Owner->OnStaminaChangedDelegate.RemoveDynamic(this, &UAstroState_Idle::OnStaminaChanged);
			Owner->OnStaminaRechargeCounterChangedDelegate.RemoveDynamic(this, &UAstroState_Idle::OnStaminaRechargeChanged_BP);
		}

		OnFatigueRemoved_BP();
	}
}


////////////////////////////////////////////////////////////////
// UAstroState_Focus
////////////////////////////////////////////////////////////////

void UAstroState_Focus::EnterState_Implementation(AAstroCharacter* InOwner)
{
	Super::EnterState_Implementation(InOwner);

	if (InOwner)
	{
		InOwner->ActivateBulletTimeAbility();
		InOwner->BroadcastFocusStateEntered();
	}

	GrantInputs();

	// AstroCharacter should ignore time dilation, effectively making it 'immune' to bullet time.
	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		TimeDilationSubsystem->RegisterIgnoreTimeDilation(InOwner);

		// Cancels inputs for one frame, to interrupt whatever the player was doing before entering focus.
		// NOTE: Prevents a bug where if the player started moving before entering BT, it would get stuck in time resume
		BlockInputs();
		InputInterruptTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UAstroState_Focus::UnblockInputs);
	}
}

void UAstroState_Focus::ExitState_Implementation(AAstroCharacter* InOwner)
{
	Super::ExitState_Implementation(InOwner);

	if (InOwner)
	{
		InOwner->CancelBulletTimeAbility();
		InOwner->BroadcastFocusStateEnded();
	}

	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		TimeDilationSubsystem->UnregisterIgnoreTimeDilation(InOwner);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InputInterruptTimerHandle);
	}
	
	UnblockInputs();
	UngrantInputs();

	if (DashTask.IsValid())
	{
		DashTask->OnFinished.RemoveAll(this);
		if (DashTask->IsActive())
		{
			DashTask->ExternalCancel();
		}
	}
}

void UAstroState_Focus::OnInputEventReceived_Implementation(FGameplayTag MatchingTag, const FGameplayEventData& Payload)
{
	Super::OnInputEventReceived_Implementation(MatchingTag, Payload);

	if (MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_CancelFocus))
	{
		Owner->SwitchState(Owner->IdleStateAbilityClass);
	}

	// Decreases stamina while the player is moving
	else if (MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_Move))
	{
		if (UAstroTimeDilationSubsystem* AstroTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
		{
			const float RealWorldDeltaSeconds = AstroTimeDilationSubsystem->GetRealTimeDeltaSeconds();
			ConsumeStamina(RealWorldDeltaSeconds * MovementCostPerSecond.GetValue());
		}
	}

	// Executes the dash task
	else if (MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_AstroDash))
	{
		DashTask = UAbilityTask_AstroDash::ExecuteAstroDash(this, Owner.Get(), Owner->AstroDashAbilityClass, Payload);
		DashTask->OnFinished.AddDynamic(this, &UAstroState_Focus::OnAstroDashFinished);
		DashTask->ReadyForActivation();

		// Consumes stamina immediately after the dash is called, to give the player instant feedback.
		// bShouldCheckStaminaDepletion prevents the player from going back to idle when the stamina is consumed,
		// to avoid fatigue during the dash. We'll perform the check right after the dash is completed, though.
		constexpr bool bShouldCheckStaminaDepletion = false;
		ConsumeStamina(Owner->AstroDashCost.GetValue(), bShouldCheckStaminaDepletion);

		// Deactivates aiming and inputs, as we don't need them while dashing.
		// NOTE: This may issue additional input gameplay events (e.g., move released), which could modify the Payload reference,
		// so we're doing it after everything has been processed to guarantee that we're processing the correct Payload.
		BlockInputs();
	}
}

void UAstroState_Focus::GrantInputs()
{
	if (AAstroController* AstroController = Owner.IsValid() ? Owner->GetController<AAstroController>() : nullptr)
	{
		AstroController->GrantMoveInput();
		AstroController->GrantDashInput();
		AstroController->GrantExitPracticeModeInput();
		Owner->EnableInteraction();
	}
}

void UAstroState_Focus::UngrantInputs()
{
	if (AAstroController* AstroController = Owner.IsValid() ? Owner->GetController<AAstroController>() : nullptr)
	{
		AstroController->UnGrantInput(AstroGameplayTags::InputTag_Move);
		AstroController->UnGrantInput(AstroGameplayTags::InputTag_AstroDash);
		AstroController->UnGrantInput(AstroGameplayTags::InputTag_ExitPracticeMode);
		Owner->DisableInteraction();
	}
}

void UAstroState_Focus::BlockInputs()
{
	if (bHasBlockedInputs)
	{
		return;
	}

	if (AAstroController* AstroController = Owner.IsValid() ? Owner->GetController<AAstroController>() : nullptr)
	{
		AstroController->RegisterInputBlock(AstroGameplayTags::InputTag_Move);
		AstroController->RegisterInputBlock(AstroGameplayTags::InputTag_AstroDash);
		AstroController->RegisterInputBlock(AstroGameplayTags::InputTag_AstroThrow);
		AstroController->RegisterInputBlock(AstroGameplayTags::InputTag_ExitPracticeMode);
		Owner->DisableInteraction();
		bHasBlockedInputs = true;
	}
}

void UAstroState_Focus::UnblockInputs()
{
	if (!bHasBlockedInputs)
	{
		return;
	}

	if (AAstroController* AstroController = Owner.IsValid() ? Owner->GetController<AAstroController>() : nullptr)
	{
		AstroController->UnregisterInputBlock(AstroGameplayTags::InputTag_Move);
		AstroController->UnregisterInputBlock(AstroGameplayTags::InputTag_AstroDash);
		AstroController->UnregisterInputBlock(AstroGameplayTags::InputTag_AstroThrow);
		AstroController->UnregisterInputBlock(AstroGameplayTags::InputTag_ExitPracticeMode);
		Owner->EnableInteraction();
		bHasBlockedInputs = false;
	}
}

void UAstroState_Focus::ConsumeStamina(float Value, const bool bShouldCheckStaminaDepletion/* = true*/)
{
	if (!Owner.IsValid())
	{
		return;
	}

	// Throttles stamina consumption, to avoid sending too many BP events and applying too many GEs
	constexpr float StaminaConsumptionThreshold = 0.01f;
	AccumulatedSpentStamina += Value;
	if (const bool bSpentEnoughStamina = AccumulatedSpentStamina < StaminaConsumptionThreshold)
	{
		return;
	}

	// Consumes accumulated depleted stamina using the stamina cost GE
	const float OldStamina = Owner->GetCurrentStamina();
	ensureMsgf(StaminaCostGE, TEXT("StaminaCostGE is invalid."));
	if (UAbilitySystemComponent* AbilitySystemComponent = Owner->GetAbilitySystemComponent())
	{
		const FGameplayEffectSpecHandle StaminaDamageEffectHandle = MakeOutgoingGameplayEffectSpec(StaminaCostGE, 0.f);
		if (StaminaDamageEffectHandle.IsValid())
		{
			if (FGameplayEffectSpec* StaminaDamageSpec = StaminaDamageEffectHandle.Data.Get())
			{
				// Values sent to this method are positive, so we revert them to subtract from stamina
				StaminaDamageSpec->SetSetByCallerMagnitude(AstroGameplayTags::SetByCaller_StaminaCost, -AccumulatedSpentStamina);
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*StaminaDamageSpec);
				AccumulatedSpentStamina = 0.f;
			}
		}
	}

	// Checks if stamina was depleted with this decrement, and if so, switches state back to idle.
	if (bShouldCheckStaminaDepletion)
	{
		const float CurrentStamina = Owner->GetCurrentStamina();
		if (const bool bbIsStaminaDepleted = CurrentStamina == 0.f)
		{
			Owner->SwitchState(Owner->IdleStateAbilityClass);
		}
	}
}

void UAstroState_Focus::CheckStaminaDepletion()
{
	// Checks if stamina was depleted with this decrement, and if so, switches state back to idle.
	const float CurrentStamina = Owner->GetCurrentStamina();
	if (const bool bbIsStaminaDepleted = CurrentStamina == 0.f)
	{
		Owner->SwitchState(Owner->IdleStateAbilityClass);
	}
}

bool UAstroState_Focus::HasStamina() const
{
	return Owner.IsValid() && Owner->GetCurrentStamina() > 0.f;
}

void UAstroState_Focus::OnAstroDashFinished_Implementation(EAstroDashResult DashResult)
{
	UnblockInputs();

	// Paired with a ConsumeStamina call when the dash is started
	CheckStaminaDepletion();
}
