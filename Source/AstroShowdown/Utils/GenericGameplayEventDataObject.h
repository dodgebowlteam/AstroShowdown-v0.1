/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "InstancedStruct.h"
#include "UObject/Object.h"
#include "GenericGameplayEventDataObject.generated.h"

/**
 * Allows users to pass data to GAs through Gameplay Events without having to create a new UObject class.
 */
UCLASS(BlueprintType)
class UGenericGameplayEventDataObject : public UObject
{
	GENERATED_BODY()

public:
	/** Container for the generic data. Will be cast to a specific type when accessed by the GAs. */
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadWrite)
	FInstancedStruct GenericData;

};
