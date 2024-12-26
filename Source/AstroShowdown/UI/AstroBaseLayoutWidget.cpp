/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's implementation of UI layouts, though we modified it a lot to suit our needs.
*/

#include "AstroBaseLayoutWidget.h"
#include "AstroGameplayTags.h"
#include "AstroUtilitiesBlueprintLibrary.h"
#include "CommonUIExtensions.h"
#include "Input/CommonUIInputTypes.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroBaseLayoutWidget)

UAstroBaseLayoutWidget::UAstroBaseLayoutWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroBaseLayoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AstroGameplayTags::UI_Action_Pause), false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandlePauseAction)));
	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(AstroGameplayTags::UI_Action_Restart), false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleRestartAction)));
}

void UAstroBaseLayoutWidget::HandlePauseAction()
{
	if (ensure(!PauseMenuClass.IsNull()))
	{
		UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), AstroGameplayTags::UI_Layer_Menu, PauseMenuClass);
	}
}

void UAstroBaseLayoutWidget::HandleRestartAction()
{
	UAstroUtilitiesBlueprintLibrary::RestartLevel(this);
}
