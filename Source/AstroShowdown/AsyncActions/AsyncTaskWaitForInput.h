/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/GameViewportClient.h"
#include "Engine/CancellableAsyncAction.h"
#include "AsyncTaskWaitForInput.generated.h"

UCLASS(BlueprintType)
class UAsyncTaskWaitForAnyInput : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "InWorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncTaskWaitForAnyInput* WaitForAnyInput(UObject* InWorldContextObject, const bool bOnlyWaitOnce = true);

public:
	virtual void Cancel() override;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitForAnyInputDelegate, FKey, Key);
	UPROPERTY(BlueprintAssignable)
	FWaitForAnyInputDelegate OnAnyKeyPressed;

private:
	/** Reference to the UWorld object. */
	UPROPERTY()
	TWeakObjectPtr<UWorld> World = nullptr;

	/** Handle to the OverrideInputKey delegate. We'll use this to track our callback and unbind it when the task is over. */
	FOverrideInputKeyHandler OverrideInputKeyHandler;

	/** When true, will only wait for a single input click */
	uint8 bOnlyWaitOnce : 1 = true;

protected:
	/** Starts waiting for input key presses. */
	UFUNCTION(BlueprintCallable)
	void StartWaitForAnyInput();

	/** Stops waiting for input key presses. */
	UFUNCTION(BlueprintCallable)
	void StopWaitForAnyInput();

private:
	/** @return true when waiting for any input. */
	bool IsWaitingForAnyInput() const;

private:
	/** Handles input key presses. */
	bool HandleAnyKeyPressed(FInputKeyEventArgs& EventArgs);

};
