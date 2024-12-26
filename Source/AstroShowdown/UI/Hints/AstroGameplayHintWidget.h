/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroActivatableWidget.h"
#include "AstroGameplayHintWidget.generated.h"

UCLASS()
class UAstroGameplayHintWidget : public UAstroActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Dismiss_BP();
};