/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroCampaignStateComponent.h"
#include "AstroCharacter.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"

UAstroCampaignStateComponent::UAstroCampaignStateComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroCampaignStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController<APlayerController>(); ensure(PlayerController))
	{
		if (AAstroCharacter* PlayerCharacter = Cast<AAstroCharacter>(PlayerController->GetPawn()))
		{
			PlayerCharacter->OnAstroCharacterDied.AddDynamic(this, &UAstroCampaignStateComponent::OnPlayerCharacterDied);
		}
	}
}

void UAstroCampaignStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UAstroCampaignStateComponent::OnPlayerCharacterDied()
{
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
	{
		constexpr bool bSuspendInputUntilComplete = true;
		RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Menu, bSuspendInputUntilComplete, DefeatScreenClass);
	}
}
