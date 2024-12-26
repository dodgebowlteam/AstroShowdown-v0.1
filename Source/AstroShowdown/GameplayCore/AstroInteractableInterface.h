/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "UObject/Interface.h"
#include "AstroInteractableInterface.generated.h"

class AAstroCharacter;

UINTERFACE(MinimalAPI, BlueprintType)
class UAstroInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class IAstroInteractableInterface : public IInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void Interact(AAstroCharacter* InteractionInstigator);

	UFUNCTION(BlueprintNativeEvent)
	bool IsInteractable() const;

	UFUNCTION(BlueprintNativeEvent)
	void OnEnterInteractionRange();

	UFUNCTION(BlueprintNativeEvent)
	void OnExitInteractionRange();
};