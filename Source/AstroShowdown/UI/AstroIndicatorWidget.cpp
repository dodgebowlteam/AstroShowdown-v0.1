/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroIndicatorWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMathLibrary.h"

namespace AstroIndicatorWidgetStatics
{
	static const FName PortraitParameterName = "PortraitTexture";
	static const FName DeadParameterName = "Grayscale";
}

void UAstroIndicatorWidget::SetIndicatorAngle(const float InAngle)
{
	IndicatedActorAngle = InAngle + IndicatorAngleOffset;

	if (IndicatorArrow)
	{
		IndicatorArrow->SetRenderTransformAngle(IndicatedActorAngle);
	}
}

void UAstroIndicatorWidget::SetIndicatorDistance(float Distance)
{
	if (IndicatorRoot)
	{
		const float DistanceScaleAlpha = FMath::Clamp(UKismetMathLibrary::SafeDivide(Distance, IndicatorScaleDistanceThreshold), 0.f, 1.f);
		const float CurrentScale = FMath::Lerp(IndicatorScaleRange.GetUpperBound().GetValue(), IndicatorScaleRange.GetLowerBound().GetValue(), DistanceScaleAlpha);
		IndicatorRoot->SetRenderScale({ CurrentScale, CurrentScale });
	}
}

void UAstroIndicatorWidget::SetIndicatorReviveProgress(const float InReviveProgress)
{
	if (IndicatorReviveBar)
	{
		IndicatorReviveBar->SetPercent(InReviveProgress);
	}
}

void UAstroIndicatorWidget::SetIndicatorIcon(UTexture2D* Icon)
{
	if (Icon && IndicatorPortrait)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = IndicatorPortrait->GetDynamicMaterial())
		{
			DynamicMaterial->SetTextureParameterValue(AstroIndicatorWidgetStatics::PortraitParameterName, Icon);
		}
	}
}

void UAstroIndicatorWidget::SetIndicatorDead(bool bIsDead)
{
	if (ensure(IndicatorPortrait))
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = IndicatorPortrait->GetDynamicMaterial())
		{
			DynamicMaterial->SetScalarParameterValue(AstroIndicatorWidgetStatics::DeadParameterName, bIsDead ? 1.f : 0.f);
		}
	}
}

void UAstroIndicatorWidget::SetIndicatorOwnerBeingRendered(const bool bInIsOwnerBeingRendered)
{
	if (bInIsOwnerBeingRendered != bIsOwnerBeingRendered)
	{
		bIsOwnerBeingRendered = bInIsOwnerBeingRendered;
		OnOwnerBeingRenderedChanged_BP(bIsOwnerBeingRendered);
	}
}
