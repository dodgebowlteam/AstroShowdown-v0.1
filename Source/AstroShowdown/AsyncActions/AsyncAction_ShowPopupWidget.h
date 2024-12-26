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
#include "AsyncAction_ShowPopupWidget.generated.h"

class APlayerController;
class UAstroPopupWidget;
class UGameInstance;
class UWorld;

/**
 * Loads a popup asynchronously, and once it's interacted with, returns its Confirm/Cancel status.
 */
UCLASS(BlueprintType)
class UAsyncAction_ShowPopup : public UCancellableAsyncAction
{
	GENERATED_BODY()

#pragma region UCancellableAsyncAction
public:
	virtual void Activate() override;
#pragma endregion

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShowPopupDelegate);
	UPROPERTY(BlueprintAssignable)
	FShowPopupDelegate OnConfirm;

	UPROPERTY(BlueprintAssignable)
	FShowPopupDelegate OnCancel;

private:
	TWeakObjectPtr<UWorld> World = nullptr;
	TWeakObjectPtr<UGameInstance> GameInstance = nullptr;
	TSoftClassPtr<UAstroPopupWidget> PopupClass = nullptr;

	uint8 bWasPopupResolved : 1 = false;

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (WorldContext = "InWorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ShowPopup* ShowPopup(UObject* InWorldContextObject, TSoftClassPtr<UAstroPopupWidget> InPopupClass);

private:
	UFUNCTION()
	void OnPopupConfirmed();

	UFUNCTION()
	void OnPopupCanceled();

};
