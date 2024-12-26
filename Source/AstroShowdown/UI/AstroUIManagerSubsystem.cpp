/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroUIManagerSubsystem.h"
#include "AstroAssetManager.h"
#include "CommonInputActionDomain.h"
#include "CommonLocalPlayer.h"
#include "Engine/GameInstance.h"
#include "GameFramework/HUD.h"
#include "GameUIPolicy.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroUIManagerSubsystem)

class FSubsystemCollectionBase;

UAstroUIManagerSubsystem::UAstroUIManagerSubsystem()
{
}


void UAstroUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// TODO (#review): Check whether we really need this. We probably don't, at least for now.
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAstroUIManagerSubsystem::Tick), 0.0f);

	// Forcibly loads all UCommonInputActionDomain, allowing CommonActivatableWidgets to reference them at runtime.
	UAstroAssetManager::Get().LoadAllAssetsOfClass<UCommonInputActionDomain>();
}

void UAstroUIManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

void UAstroUIManagerSubsystem::SyncRootLayoutVisibilityToShowHUD()
{
	if (const UGameUIPolicy* Policy = GetCurrentUIPolicy())
	{
		for (const ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
		{
			bool bShouldShowUI = true;

			if (const APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
			{
				const AHUD* HUD = PC->GetHUD();

				if (HUD && !HUD->bShowHUD)
				{
					bShouldShowUI = false;
				}
			}

			if (UPrimaryGameLayout* RootLayout = Policy->GetRootLayout(CastChecked<UCommonLocalPlayer>(LocalPlayer)))
			{
				const ESlateVisibility DesiredVisibility = bShouldShowUI ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
				if (DesiredVisibility != RootLayout->GetVisibility())
				{
					RootLayout->SetVisibility(DesiredVisibility);
				}
			}
		}
	}
}

bool UAstroUIManagerSubsystem::Tick(float DeltaTime)
{
	// Uses polling to check if AHUD::bShowHUD has changed, and if so, disables the current UPrimaryGameLayout
	SyncRootLayoutVisibilityToShowHUD();

	return true;
}
