/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "UObject/ObjectPtr.h"
#include "AstroRoomNavigationTypes.generated.h"

class UAstroRoomData;
class UAstroSectionData;

/** Generic struct to broadcast section-related events. */
USTRUCT(BlueprintType)
struct FAstroSectionGenericMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UAstroSectionData> Section = nullptr;
};

/** Generic struct to broadcast room-related events. */
USTRUCT(BlueprintType)
struct FAstroRoomGenericMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<const UAstroRoomData> Room = nullptr;
};
