/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace SubsystemUtils
{
	template <typename T>
	T* GetWorldSubsystem(UObject* WorldContextObject)
	{
		if (UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
		{
			return World->GetSubsystem<T>();
		}

		return nullptr;
	}


	template <typename T>
	T* GetGameInstanceSubsystem(const UObject* WorldContextObject)
	{
		if (UWorld* World = WorldContextObject->GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				return UGameInstance::GetSubsystem<T>(GameInstance);
			}
		}

		ensureMsgf(false, TEXT("::ERROR: (%s) Failed to get world subsystem, World is invalid."), ANSI_TO_TCHAR(__FUNCTION__));
		return nullptr;
	}
}

