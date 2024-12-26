/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAstroAttributeChangedEvent, float /*OldValue*/, float /*NewValue*/);

#define ASTRO_DECLARE_GAS_ATTRIBUTE_HELPERS(AttributeAccessLevel, ClassName, AttributeName) \
	public:\
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, AttributeName)\
	GAMEPLAYATTRIBUTE_VALUE_GETTER(AttributeName)\
	GAMEPLAYATTRIBUTE_VALUE_SETTER(AttributeName)\
	GAMEPLAYATTRIBUTE_VALUE_INITTER(AttributeName)\
	AttributeAccessLevel: