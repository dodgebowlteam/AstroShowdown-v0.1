/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroBlockInputGameplayEffectComponent.h"
#include "AbilitySystemComponent.h"
#include "AstroCharacter.h"
#include "AstroController.h"

void UAstroBlockInputGameplayEffectComponent::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITORONLY_DATA
	EditorFriendlyName = TEXT("Astro Block Input w/ Tag");
#endif
}

void UAstroBlockInputGameplayEffectComponent::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	if (UAbilitySystemComponent* OwnerASC = ActiveGEContainer.Owner)
	{
		AAstroCharacter* OwnerCharacter = Cast<AAstroCharacter>(OwnerASC->GetAvatarActor());
		ensureMsgf(OwnerCharacter, TEXT("Tried blocking input of a Non-AstroCharacter target."));
		if (AAstroController* OwnerController = OwnerCharacter ? Cast<AAstroController>(OwnerCharacter->GetController()) : nullptr)
		{
			for (const FAstroBlockInputEffectData& ModifiedInput : ModifiedInputs)

			if (ModifiedInput.bShouldBlockInput)
			{
				OwnerController->RegisterInputBlock(ModifiedInput.InputTag);
			}
			else
			{
				OwnerController->UnregisterInputBlock(ModifiedInput.InputTag);
			}
		}
	}
}
