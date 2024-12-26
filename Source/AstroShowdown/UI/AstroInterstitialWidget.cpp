/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroInterstitialWidget.h"
#include "AstroGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"


void UAstroInterstitialWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// Broadcasts interstitial start message
	FAstroInterstitialGameplayMessage Payload;
	Payload.InterstitialInstance = this;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_Interstitial_Start, Payload);
}

void UAstroInterstitialWidget::OnInterstitialEnd_Implementation()
{
	OnInterstitialEndEvent.Broadcast();

	// Broadcasts interstitial end message
	FAstroInterstitialGameplayMessage Payload;
	Payload.InterstitialInstance = this;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_Interstitial_End, Payload);
}
