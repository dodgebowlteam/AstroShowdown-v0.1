/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "GameplayAbility_BulletTime.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AstroGameplayTags.h"
#include "AstroTimeDilationSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "SubsystemUtils.h"

namespace BulletTimeVars
{
	static float ResumeAfterMovingIncrementStep = 6.5f;
	static FAutoConsoleVariableRef CVarIsFriendlyFireSupported(
		TEXT("AstroBulletTime.ResumeAfterMovingIncrementStep"),
		ResumeAfterMovingIncrementStep,
		TEXT("Time dilation smoothing increment step for when time dilation is changed after the player stops moving."),
		ECVF_Default);
}

UGameplayAbility_BulletTime::UGameplayAbility_BulletTime()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGameplayAbility_BulletTime::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		constexpr bool bReplicateEndAbility = true;
		constexpr bool bWasCancelled = true;
		EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}

	// Slows time down
	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(ActorInfo->AvatarActor.Get()))
	{
		// bSmooth will smooth the time dilation change over a few frames, preventing big physics tick deltas (i.e., velocity explosions)
		// NOTE: From a game design perspective, it's important that this time dilation change is as close to instant as possible
		constexpr bool bSmooth = true;
		TimeDilationSubsystem->SetGlobalTimeDilation(BulletTimeCustomTimeDilation, bSmooth);
	}

	if (UAbilitySystemComponent* InOwnerAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActorInfo->AvatarActor.Get()))
	{
		// Listens for input tag events sent to the owning avatar actor
		const FGameplayTagContainer InputTagContainer { AstroGameplayTags::InputTag };
		InputEventDelegateHandle = InOwnerAbilitySystemComponent->AddGameplayEventTagContainerDelegate(InputTagContainer, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGameplayAbility_BulletTime::OnInputEventReceived));
	}
}


void UGameplayAbility_BulletTime::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// Clears all resume requests
	ResumeRequests.Empty();

	// Stops listening for input and gameplay events
	if (UAbilitySystemComponent* InOwnerAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActorInfo->AvatarActor.Get()))
	{
		if (InputEventDelegateHandle.IsValid())
		{
			const FGameplayTagContainer InputTagContainer { AstroGameplayTags::InputTag };
			InOwnerAbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(InputTagContainer, InputEventDelegateHandle);
			InputEventDelegateHandle.Reset();
		}
	}

	// Restores time back to normal
	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(ActorInfo->AvatarActor.Get()))
	{
		TimeDilationSubsystem->SetGlobalTimeDilation(1.f);
	}
}

void UGameplayAbility_BulletTime::AddResumeTimeRequest(const FTransientResumeTimeRequest& NewResumeTimeRequest)
{
	// Prevents from adding the same resume request and changing time dilation for it multiple times
	if (!ResumeRequests.Contains(NewResumeTimeRequest))
	{
		ResumeRequests.Add(NewResumeTimeRequest);
		ResumeRequests.Sort([](const FTransientResumeTimeRequest& RequestA, const FTransientResumeTimeRequest& RequestB)
		{
			return RequestA.ResumeTimeDilationValue >= RequestB.ResumeTimeDilationValue;
		});		// Sorts by bigger time dilation value first, to make it easier to update
		UpdateTimeDilation();
	}
}

void UGameplayAbility_BulletTime::RemoveResumeTimeRequest(const FTransientResumeTimeRequest& RemovedResumeTimeRequest)
{
	if (ResumeRequests.Contains(RemovedResumeTimeRequest))
	{
		ResumeRequests.Remove(RemovedResumeTimeRequest);
		UpdateTimeDilation();
	}
}

void UGameplayAbility_BulletTime::UpdateTimeDilation()
{
	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(GetActorInfo().AvatarActor.Get()))
	{
		// bSmooth: Avoids big physics tick deltas.
		// TimeDilationIncrementStepOverride: Makes smooth change sharper when leaving the resumed state, as we don't want it to last too long.
		// NOTE: Assumes ResumeRequests are sorted
		constexpr bool bSmooth = true;
		const bool bIsResumingTime = ResumeRequests.Num() > 0;
		const float NewTimeDilationValue = bIsResumingTime ? ResumeRequests[0].ResumeTimeDilationValue : BulletTimeCustomTimeDilation;
		const float TimeDilationIncrementStepOverride = bIsResumingTime ? -1.f : BulletTimeVars::ResumeAfterMovingIncrementStep;
		TimeDilationSubsystem->SetGlobalTimeDilation(NewTimeDilationValue, bSmooth, TimeDilationIncrementStepOverride);
	}
}

void UGameplayAbility_BulletTime::OnInputEventReceived(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	// Prevents input events from coming through after we stop listening to them. This might happen in cases where the delegate was cached before the event listener was removed.
	if (!InputEventDelegateHandle.IsValid())
	{
		return;
	}

	// Restores time back to normal while player is moving
	if (bShouldResumeTimeWhileMoving && MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_Move))
	{
		AddResumeTimeRequest({ AstroGameplayTags::InputTag_Move, BulletTimeCustomTimeDilationWhileMoving });
	}

	// Slows time down again once the player stops moving
	if (bShouldResumeTimeWhileMoving && MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_Move_Released))
	{
		RemoveResumeTimeRequest({ AstroGameplayTags::InputTag_Move });
	}

	// Restores time back to normal while player is throwing
	if (bShouldResumeTimeWhileThrowing && MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_AstroThrow))
	{
		AddResumeTimeRequest({ AstroGameplayTags::InputTag_AstroThrow, BulletTimeCustomTimeDilationWhileThrowing });
	}

	// Slows time down again once the player stops throwing
	if (bShouldResumeTimeWhileThrowing && MatchingTag.MatchesTagExact(AstroGameplayTags::InputTag_AstroThrow_Release))
	{
		RemoveResumeTimeRequest({ AstroGameplayTags::InputTag_AstroThrow });
	}
}
