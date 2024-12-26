/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AbilityTask_AstroDash.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "AstroAbilitySystem.h"
#include "AstroCharacter.h"
#include "AstroGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameplayAbility_AstroDash.h"
#include "TimerManager.h"

UAbilityTask_AstroDash::UAbilityTask_AstroDash(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UAbilityTask_AstroDash* UAbilityTask_AstroDash::ExecuteAstroDash(UGameplayAbility* OwningAbility, AAstroCharacter* InDashOwner, TSubclassOf<UGameplayAbility_AstroDash> InAstroDashAbilityClass, FGameplayEventData InAstroDashGameplayEvent, FName TaskInstanceName)
{
	UAbilityTask_AstroDash* MyObj = NewAbilityTask<UAbilityTask_AstroDash>(OwningAbility, TaskInstanceName);
	MyObj->DashOwner = InDashOwner;
	MyObj->AstroDashAbilityClass = InAstroDashAbilityClass;
	MyObj->CachedAstroDashGameplayEvent = InAstroDashGameplayEvent;
	return MyObj;
}

void UAbilityTask_AstroDash::Activate()
{
	if (!Ability)
	{
		return;
	}

	ensure(!DashAbilitySpecHandle.IsValid());

	if (AbilitySystemComponent.IsValid())
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();

		// Build and validates the ability spec
		FGameplayAbilitySpec AbilitySpec = AbilitySystemComponent->BuildAbilitySpecFromClass(AstroDashAbilityClass, 0, -1);
		if (!IsValid(AbilitySpec.Ability))
		{
			ensureMsgf(false, TEXT("TryActivateAbilityWithCallback called with an invalid Ability Class."));
			return;
		}

		// Activates the ability and binds the completion delegates
		AbilitySystemComponent->AbilityActivatedCallbacks.AddUObject(this, &UAbilityTask_AstroDash::OnGameplayAbilityActivated);
		AbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &UAbilityTask_AstroDash::OnGameplayAbilityFailed);
		DashAbilitySpecHandle = AbilitySystemComponent->GiveAbilityAndActivateOnce(AbilitySpec);

		if (!ensure(DashAbilitySpecHandle.IsValid()))
		{
			ABILITY_LOG(Warning, TEXT("[%s] Failed to activate dash"), ANSI_TO_TCHAR(__FUNCTION__));
			EndTask();
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("[%s] Called on invalid AbilitySystemComponent"), ANSI_TO_TCHAR(__FUNCTION__));
		EndTask();
	}
}

void UAbilityTask_AstroDash::ExternalCancel()
{
	if (AbilitySystemComponent.IsValid() && IsDashing())
	{
		FGameplayAbilitySpec* AstroDashAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(AstroDashAbilityClass);
		if (AstroDashAbility && AstroDashAbilitySpec && AstroDashAbility->IsActive())
		{
			AbilitySystemComponent->CancelAbility(AstroDashAbilitySpec->Ability);
		}
	}

	Super::ExternalCancel();
}

void UAbilityTask_AstroDash::OnDestroy(bool AbilityEnded)
{
	if (AstroDashAbility)
	{
		AstroDashAbility->OnAstroDashFinished.RemoveAll(this);
	}

	UnregisterAbilityActivationDelegates();

	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_AstroDash::OnAstroDashFinished(EAstroDashResult DashResult)
{
	bIsDashing = false;

	OnFinished.Broadcast(DashResult);

	EndTask();
}

void UAbilityTask_AstroDash::OnGameplayAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (UGameplayAbility_AstroDash* ActivatedDash = Cast<UGameplayAbility_AstroDash>(ActivatedAbility))
	{
		bIsDashing = true;

		if (!ShouldBroadcastAbilityTaskDelegates())
		{
			ABILITY_LOG(Display, TEXT("[%s] Suppressing ability task delegates"), ANSI_TO_TCHAR(__FUNCTION__));
			EndTask();
			return;
		}

		AstroDashAbility = ActivatedDash;
		AstroDashAbility->OnAstroDashFinished.AddDynamic(this, &UAbilityTask_AstroDash::OnAstroDashFinished);

		// Sends the dash payload data through a gameplay event.
		// NOTE: OnGameplayAbilityActivated is called during UGameplayAbility::PreActivate, so we'll delay this
		// by one frame to ensure that the dash called UGameplayAbility::Activate, and is waiting for the event.
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
			{
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(DashOwner.Get(), AstroGameplayTags::GameplayEvent_AstroDashPayload, CachedAstroDashGameplayEvent);
			}));
		}
		
		OnStarted.Broadcast();
		
		UnregisterAbilityActivationDelegates();
	}
}

void UAbilityTask_AstroDash::OnGameplayAbilityFailed(const UGameplayAbility* FailedAbility, const FGameplayTagContainer& FailTags)
{
	if (const UGameplayAbility_AstroDash* ActivatedDash = Cast<UGameplayAbility_AstroDash>(FailedAbility))
	{
		ensure(false);
		EndTask();
	}
}

void UAbilityTask_AstroDash::UnregisterAbilityActivationDelegates()
{
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->AbilityActivatedCallbacks.RemoveAll(this);
		AbilitySystemComponent->AbilityFailedCallbacks.RemoveAll(this);
	}
}
