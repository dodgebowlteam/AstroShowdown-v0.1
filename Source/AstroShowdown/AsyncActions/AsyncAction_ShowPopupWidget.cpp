/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AsyncAction_ShowPopupWidget.h"
#include "AstroAssetManager.h"
#include "AstroGameplayTags.h"
#include "AstroPopupWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "UObject/Stack.h"
#include "PrimaryGameLayout.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ShowPopupWidget)


UAsyncAction_ShowPopup* UAsyncAction_ShowPopup::ShowPopup(UObject* InWorldContextObject, TSoftClassPtr<UAstroPopupWidget> InPopupClass)
{
	if (InPopupClass.GetAssetName().IsEmpty())
	{
		FFrame::KismetExecutionMessage(TEXT("ShowPopup was passed an invalid PopupClass"), ELogVerbosity::Error);
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UAsyncAction_ShowPopup* Action = NewObject<UAsyncAction_ShowPopup>();
	Action->PopupClass = InPopupClass;
	Action->World = World;
	Action->GameInstance = World->GetGameInstance();
	Action->RegisterWithGameInstance(World);

	return Action;
}

void UAsyncAction_ShowPopup::Activate()
{
	if (!World.IsValid())
	{
		Cancel();
		return;
	}

	UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(World.Get());
	if (!RootLayout)
	{
		Cancel();
		return;
	}

	TWeakObjectPtr<UAsyncAction_ShowPopup> LocalWeakThis(this);
	auto HandlePopupState = [LocalWeakThis](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen)
	{
		switch (State)
		{
		case EAsyncWidgetLayerState::AfterPush:
			if (UAstroPopupWidget* PopupWidget = CastChecked<UAstroPopupWidget>(Screen))
			{
				if (LocalWeakThis.IsValid())
				{
					PopupWidget->OnConfirm.AddUniqueDynamic(LocalWeakThis.Get(), &UAsyncAction_ShowPopup::OnPopupConfirmed);
					PopupWidget->OnCancel.AddUniqueDynamic(LocalWeakThis.Get(), &UAsyncAction_ShowPopup::OnPopupCanceled);
					PopupWidget->OnDeactivated().AddUObject(LocalWeakThis.Get(), &UAsyncAction_ShowPopup::OnPopupCanceled);
				}
			}
			break;
		case EAsyncWidgetLayerState::Canceled:
			if (LocalWeakThis.IsValid())
			{
				LocalWeakThis->OnPopupCanceled();
			}
			break;
		}
	};

	constexpr bool bSuspendInputUntilComplete = true;
	RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Modal, bSuspendInputUntilComplete, PopupClass, HandlePopupState);
}

void UAsyncAction_ShowPopup::OnPopupConfirmed()
{
	if (bWasPopupResolved)
	{
		return;
	}

	OnConfirm.Broadcast();
	SetReadyToDestroy();
	bWasPopupResolved = true;
}

void UAsyncAction_ShowPopup::OnPopupCanceled()
{
	if (bWasPopupResolved)
	{
		return;
	}

	OnCancel.Broadcast();
	SetReadyToDestroy();
	bWasPopupResolved = true;
}
