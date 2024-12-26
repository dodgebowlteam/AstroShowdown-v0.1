/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Blueprint/UserWidget.h"
#include "AstroPerfDisplayWidget.generated.h"

class UTextBlock;

UCLASS()
class UAstroPerfDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

#pragma region UserWidget
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
#pragma endregion

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UTextBlock* FPSTextWidget = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float TickInterval = 0.1f;

private:
	float CurrentTickCounter = 0.f;

};
