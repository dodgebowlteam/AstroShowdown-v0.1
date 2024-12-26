/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AsyncAction_RealTimeDelay.h"
#include "AstroTimeDilationSubsystem.h"
#include "Engine/Engine.h"
#include "SubsystemUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_RealTimeDelay)


UAsyncAction_RealTimeDelay* UAsyncAction_RealTimeDelay::WaitForRealTime(UObject* InWorldContextObject, float InDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UAsyncAction_RealTimeDelay* Action = NewObject<UAsyncAction_RealTimeDelay>();
	Action->DelayDuration = InDuration;
	Action->World = World;
	Action->RegisterWithGameInstance(World);

	return Action;
}

void UAsyncAction_RealTimeDelay::Activate()
{
	Super::Activate();

	if (DelayDuration <= 0.f)
	{
		OnComplete.Broadcast();
		OnCompleteDelegate.Broadcast();
		SetReadyToDestroy();
	}
	else if (World.IsValid())
	{
		DelayTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UAsyncAction_RealTimeDelay::OnDelayTick);
	}
	else
	{
		ensureMsgf(false, TEXT("Failed to initialize AsyncAction"));
		Cancel();
	}
}

void UAsyncAction_RealTimeDelay::Cancel()
{
	Super::Cancel();

	if (DelayTimerHandle.IsValid() && World.IsValid())
	{
		World->GetTimerManager().ClearTimer(DelayTimerHandle);
	}
}

void UAsyncAction_RealTimeDelay::OnDelayTick()
{
	UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(World.Get());
	const float DeltaTime = (TimeDilationSubsystem ? TimeDilationSubsystem->GetRealTimeDeltaSeconds() : 0.f);
	const float DeltaTimeClamped = FMath::Min(DeltaTime, 1.f / 30.f);		// We need this because RealTimeDelta is broken and jumps when the game is unpaused
	CurrentDelayCounter += DeltaTimeClamped;
	if (CurrentDelayCounter >= DelayDuration)
	{
		OnComplete.Broadcast();
		OnCompleteDelegate.Broadcast();
		SetReadyToDestroy();
	}
	else if (World.IsValid())
	{
		DelayTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UAsyncAction_RealTimeDelay::OnDelayTick);
	}
}

