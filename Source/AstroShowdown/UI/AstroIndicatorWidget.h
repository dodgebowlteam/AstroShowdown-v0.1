/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Blueprint/UserWidget.h"
#include "AstroIndicatorWidget.generated.h"

class UImage;
class UProgressBar;

UCLASS()
class UAstroIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIndicatorAngle(const float InAngle);
	void SetIndicatorDistance(float InDistance);
	void SetIndicatorReviveProgress(const float InReviveProgress);
	void SetIndicatorIcon(UTexture2D* Icon);
	void SetIndicatorDead(bool bIsDead);
	void SetIndicatorOwnerBeingRendered(const bool bInIsOwnerBeingRendered);

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnOwnerBeingRenderedChanged_BP(const bool bInIsOwnerBeingRendered);


protected:
	/** Angle between the indicator and its owner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arrow")
	float IndicatedActorAngle = 0.f;

	/** Angle offset for the indicator's arrow. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arrow")
	float IndicatorAngleOffset = 90.f;

	/**
	* Indicator size will be lerped from IndicatorScaleRange.Min, IndicatorScaleRange.Max using the distance from the indicator to the owner as Alpha.
	*/
	UPROPERTY(EditAnywhere, Category = "Distance Scaling")
	FFloatRange IndicatorScaleRange;

	/**
	* Determines the distance value at which we'll use IndicatorScaleRange.Max as scale.
	*/
	UPROPERTY(EditAnywhere, Category = "Distance Scaling")
	float IndicatorScaleDistanceThreshold = 400.f;

	UPROPERTY(BlueprintReadWrite, Transient)
	UWidget* IndicatorRoot = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient)
	UWidget* IndicatorArrow = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient)
	UImage* IndicatorPortrait = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient)
	UProgressBar* IndicatorReviveBar = nullptr;

	UPROPERTY(BlueprintReadOnly)
	uint8 bIsOwnerBeingRendered : 1 = false;

};