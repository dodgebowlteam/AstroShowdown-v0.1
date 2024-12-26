/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroGameplayHintSubsystem.h"
#include "AstroCampaignPersistenceSubsystem.h"
#include "AstroGameplayHintWidget.h"
#include "AstroGameplayTags.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"


namespace AstroCVars
{
	static bool bIgnoreDisplayedHintCheck = false;
	static FAutoConsoleVariableRef CVarIgnoreDisplayedHintCheck(
		TEXT("AstroGameplayHint.IgnoreDisplayedHintCheck"),
		bIgnoreDisplayedHintCheck,
		TEXT("When enabled, the hint display check will be ignored, and hints will always be displayed."),
		ECVF_Default);
}

DECLARE_LOG_CATEGORY_EXTERN(LogAstroGameplayHint, Log, All);
DEFINE_LOG_CATEGORY(LogAstroGameplayHint);

void UAstroGameplayHintSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency(UAstroCampaignPersistenceSubsystem::StaticClass());

	if (UAstroCampaignPersistenceSubsystem* CampaignPersistenceSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroCampaignPersistenceSubsystem>(this))
	{
		CachedCampaignPersistenceSubsystem = CampaignPersistenceSubsystem;
	}

	// Converts GameplayHintWidgetsConfig to GameplayHintWidgets
	for (const FGameplayHintWidgetPair& GameplayHintWidgetConfig : GameplayHintWidgetsConfig)
	{
		ensureMsgf(!GameplayHintWidgets.Contains(GameplayHintWidgetConfig.Type), TEXT("Duplicate GameplayHint entry"));
		GameplayHintWidgets.Add(GameplayHintWidgetConfig.Type, GameplayHintWidgetConfig.Widget);

		if (GameplayHintWidgetConfig.bForceDisplayDuringOnboarding)
		{
			OnboardingPermanentHints.Add(GameplayHintWidgetConfig.Type);
		}
	}
}

void UAstroGameplayHintSubsystem::Deinitialize()
{
	Super::Deinitialize();

	QueuedHints.Empty();
}

void UAstroGameplayHintSubsystem::QueueHint(const EAstroGameplayHintType GameplayHint)
{
	if (GameplayHint == EAstroGameplayHintType::None)
	{
		return;
	}

	if (!CanDisplayHint(GameplayHint))
	{
		return;
	}

	if (ensureMsgf(GameplayHintWidgets.Contains(GameplayHint), TEXT("Couldn't find widget for the GameplayHint")))
	{
		QueuedHints.AddUnique(GameplayHint);
		OnHintQueued.Broadcast(GameplayHint);
	}
}

void UAstroGameplayHintSubsystem::DequeueHint()
{
	if (AreHintsLocked())
	{
		return;
	}

	if (QueuedHints.IsEmpty())
	{
		return;
	}

	const EAstroGameplayHintType TopmostHint = QueuedHints[0];
	QueuedHints.RemoveAt(0);

	// Consider moving to AstroCampaignPersistenceComponent
	if (CachedCampaignPersistenceSubsystem.IsValid())
	{
		CachedCampaignPersistenceSubsystem->SaveDisplayedHint(static_cast<uint8>(TopmostHint));
	}
}

void UAstroGameplayHintSubsystem::AddLockInstigator(UObject* NewLockInstigator)
{
	const bool bOldLockState = AreHintsLocked();

	ActiveLockInstigators.Remove(nullptr);		// Clears invalid instigators
	ActiveLockInstigators.Add(NewLockInstigator);

	const bool bNewLockState = AreHintsLocked();
	if (bOldLockState != bNewLockState)
	{
		OnLockStateChanged.Broadcast(bNewLockState);
	}
}

void UAstroGameplayHintSubsystem::RemoveLockInstigator(UObject* LockInstigator)
{
	const bool bOldLockState = AreHintsLocked();

	ActiveLockInstigators.Remove(nullptr);		// Clears invalid instigators
	ActiveLockInstigators.Remove(LockInstigator);

	const bool bNewLockState = AreHintsLocked();
	if (bOldLockState != bNewLockState)
	{
		OnLockStateChanged.Broadcast(bNewLockState);
	}
}

bool UAstroGameplayHintSubsystem::CanDisplayHint(const EAstroGameplayHintType GameplayHint) const
{
	if (AstroCVars::bIgnoreDisplayedHintCheck)
	{
		return true;
	}

	if (ShouldForceDisplayHint(GameplayHint))
	{
		return true;
	}

	return !WasHintDisplayed(GameplayHint);
}

bool UAstroGameplayHintSubsystem::WasHintDisplayed(const EAstroGameplayHintType GameplayHint) const
{
	if (AstroCVars::bIgnoreDisplayedHintCheck)
	{
		return false;
	}

	if (CachedCampaignPersistenceSubsystem.IsValid())
	{
		return CachedCampaignPersistenceSubsystem->WasHintDisplayed(static_cast<uint8>(GameplayHint));
	}

	return false;
}

bool UAstroGameplayHintSubsystem::ShouldForceDisplayHint(const EAstroGameplayHintType GameplayHint) const
{
	if (CachedCampaignPersistenceSubsystem.IsValid() && OnboardingPermanentHints.Contains(GameplayHint))
	{
		return CachedCampaignPersistenceSubsystem->NumSectionsUnlocked() <= 1;
	}

	return false;
}
