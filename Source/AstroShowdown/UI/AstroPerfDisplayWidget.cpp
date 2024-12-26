/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroPerfDisplayWidget.h"
#include "Components/TextBlock.h"

void UAstroPerfDisplayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// If TickInterval is set, we'll only run Tick every few frames
	CurrentTickCounter += InDeltaTime;
	if (CurrentTickCounter < TickInterval)
	{
		return;
	}

	CurrentTickCounter = 0.f;

	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FPSTextWidget)
	{
		const int32 CurrentFPS = FMath::RoundToInt(1.0f / InDeltaTime);
		FPSTextWidget->SetText(FText::FromString(FString::Printf(TEXT("FPS: %d"), CurrentFPS)));
	}
}
