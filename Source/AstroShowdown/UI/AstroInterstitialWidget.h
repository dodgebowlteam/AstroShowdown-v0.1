/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroActivatableWidget.h"
#include "AstroInterstitialWidget.generated.h"

/** Generic struct used by interstitial gameplay messages. */
USTRUCT(BlueprintType)
struct FAstroInterstitialGameplayMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UAstroInterstitialWidget> InterstitialInstance;
};

UCLASS()
class UAstroInterstitialWidget : public UAstroActivatableWidget
{
	GENERATED_BODY()

#pragma region UAstroActivatableWidget
protected:
	virtual void NativeOnActivated();
#pragma endregion

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 bShouldMuteBackgroundMusic : 1 = true;

public:
	DECLARE_MULTICAST_DELEGATE(FOnInterstitialEnd);
	FOnInterstitialEnd OnInterstitialEndEvent;

protected:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnInterstitialEnd();
	virtual void OnInterstitialEnd_Implementation();

};
