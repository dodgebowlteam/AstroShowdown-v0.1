/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "UObject/SoftObjectPtr.h"
#include "TimerManager.h"
#include "AsyncAction_FollowSplinePath.generated.h"

class ACharacter;
class USplineComponent;
class UWorld;

/**
 * Makes a character follow along a spline path.
 */
UCLASS(BlueprintType)
class UAsyncAction_FollowSplinePath : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFollowSplinePathDelegate);
	UPROPERTY(BlueprintAssignable)
	FFollowSplinePathDelegate OnComplete;
	UPROPERTY(BlueprintAssignable)
	FFollowSplinePathDelegate OnCancel;

private:
	UPROPERTY()
	TWeakObjectPtr<ACharacter> OwnerCharacter = nullptr;

	UPROPERTY()
	TWeakObjectPtr<USplineComponent> SplinePath = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World = nullptr;

	UPROPERTY()
	float ArriveThresholdSqr = 0.f;

	UPROPERTY()
	bool bReverse = false;

private:
	FTimerHandle FollowSplineTimerHandle;

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (WorldContext = "InWorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_FollowSplinePath* FollowSplinePath(UObject* InWorldContextObject, ACharacter* Character, USplineComponent* Spline, const float InArriveThresholdSqr = 1.f, const bool bInReverse = false);

	virtual void Activate() override;
	virtual void Cancel() override;

private:
	void OnSplineFollowTick();

};
