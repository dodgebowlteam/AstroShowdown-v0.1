/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AsyncTaskWaitForInput.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"

UAsyncTaskWaitForAnyInput* UAsyncTaskWaitForAnyInput::WaitForAnyInput(UObject* InWorldContextObject, const bool bOnlyWaitOnce/* = true*/)
{
	UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UAsyncTaskWaitForAnyInput* WaitForAnyInputTask = NewObject<UAsyncTaskWaitForAnyInput>();
	WaitForAnyInputTask->World = World;
	WaitForAnyInputTask->StartWaitForAnyInput();
	WaitForAnyInputTask->bOnlyWaitOnce = bOnlyWaitOnce;
	return WaitForAnyInputTask;
}

void UAsyncTaskWaitForAnyInput::Cancel()
{
	Super::Cancel();
	
	StopWaitForAnyInput();
}

void UAsyncTaskWaitForAnyInput::StartWaitForAnyInput()
{
	if (IsWaitingForAnyInput())
	{
		return;
	}

	// Binds input override
	if (World.IsValid())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient())
			{
				ensure(!ViewportClient->OnOverrideInputKey().IsBound());
				ViewportClient->OnOverrideInputKey().BindUObject(this, &UAsyncTaskWaitForAnyInput::HandleAnyKeyPressed);
			}
		}
	}
}

void UAsyncTaskWaitForAnyInput::StopWaitForAnyInput()
{
	if (!IsWaitingForAnyInput())
	{
		return;
	}

	// Unbinds input override
	if (World.IsValid())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient())
			{
				ViewportClient->OnOverrideInputKey().Unbind();
			}
		}
	}
}

bool UAsyncTaskWaitForAnyInput::IsWaitingForAnyInput() const
{
	if (World.IsValid())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient())
			{
				return ViewportClient->OnOverrideInputKey().IsBoundToObject(this);
			}
		}
	}

	return false;
}

bool UAsyncTaskWaitForAnyInput::HandleAnyKeyPressed(FInputKeyEventArgs& EventArgs)
{
	OnAnyKeyPressed.Broadcast(EventArgs.Key);

	if (bOnlyWaitOnce)
	{
		Cancel();
	}

	return true;
}
