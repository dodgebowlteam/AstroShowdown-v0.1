/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "AsyncAction_RealTimeDelay.generated.h"

class UWorld;

/**
 * Just like a delay node, but ignores time dilation.
 */
UCLASS(BlueprintType)
class UAsyncAction_RealTimeDelay : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRealTimeDelayDelegate);
	UPROPERTY(BlueprintAssignable)
	FRealTimeDelayDelegate OnComplete;

	DECLARE_MULTICAST_DELEGATE(FRealTimeDelayStaticDelegate);
	FRealTimeDelayStaticDelegate OnCompleteDelegate;

private:
	TWeakObjectPtr<UWorld> World = nullptr;
	FTimerHandle DelayTimerHandle;

	float DelayDuration = 0.f;
	float CurrentDelayCounter = 0.f;

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (WorldContext = "InWorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_RealTimeDelay* WaitForRealTime(UObject* InWorldContextObject, float InDuration);

	virtual void Activate() override;
	virtual void Cancel() override;

private:
	void OnDelayTick();

};
