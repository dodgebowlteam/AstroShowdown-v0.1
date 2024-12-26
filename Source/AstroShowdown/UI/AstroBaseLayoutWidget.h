/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
* 
* NOTE: Based on Epic Games Lyra's implementation of UI layouts, though we modified it a lot to suit our needs.
*/

#pragma once

#include "AstroActivatableWidget.h"
#include "AstroBaseLayoutWidget.generated.h"

class UObject;

/**
 * UAstroBaseLayoutWidget
 *
 * Widget used to lay out the player's HUD (typically specified by an Add Widgets action in the game context)
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "Astro Base Layout Widget", Category = "Astro|UI"))
class UAstroBaseLayoutWidget : public UAstroActivatableWidget
{
	GENERATED_BODY()

#pragma region AstroBaseLayoutWidget
public:
	UAstroBaseLayoutWidget(const FObjectInitializer& ObjectInitializer);
	virtual void NativeOnInitialized() override;
#pragma endregion


protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> PauseMenuClass;

protected:
	void HandlePauseAction();
	void HandleRestartAction();
};
