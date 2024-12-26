/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AstroGameplayHintSubsystem.generated.h"

class UAstroCampaignPersistenceSubsystem;
class UAstroGameplayHintWidget;
class UCommonActivatableWidget;

UENUM(BlueprintType)
enum class EAstroGameplayHintType : uint8
{
	None,
	PlayerFocus,
	PlayerDeath,
	PlayerFatigue,
	PracticeMode,
	StaminaPickup,
	NPCRevive,
};

USTRUCT()
struct FGameplayHintWidgetPair
{
	GENERATED_BODY()

	UPROPERTY()
	EAstroGameplayHintType Type = EAstroGameplayHintType::None;

	UPROPERTY()
	TSoftClassPtr<UAstroGameplayHintWidget> Widget = nullptr;

	UPROPERTY()
	uint8 bForceDisplayDuringOnboarding = false;
};


UCLASS(Config=Game)
class UAstroGameplayHintSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

#pragma region GameInstanceSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
#pragma endregion

private:
	/** .ini-configurable version of GameplayHintWidgets. Prefer using GameplayHintWidgets during runtime processing. */
	UPROPERTY(Config)
	TArray<FGameplayHintWidgetPair> GameplayHintWidgetsConfig;

	/** Maps AstroGameplayHintTypes to their respective widget class. This is derived from GameplayHintWidgetsConfig. */
	UPROPERTY()
	TMap<EAstroGameplayHintType, TSoftClassPtr<UAstroGameplayHintWidget>> GameplayHintWidgets;

	/** Contains the list of hints that should always be displayed during onboarding, no matter if they already were. */
	UPROPERTY()
	TSet<EAstroGameplayHintType> OnboardingPermanentHints;

	/** Hints waiting to get processed and displayed on the screen. */
	UPROPERTY()
	TArray<EAstroGameplayHintType> QueuedHints;

	/** Stores a list of objects that are actively locking hints. */
	UPROPERTY()
	TSet<TWeakObjectPtr<UObject>> ActiveLockInstigators;

	UPROPERTY()
	TWeakObjectPtr<UAstroCampaignPersistenceSubsystem> CachedCampaignPersistenceSubsystem = nullptr;

protected:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLockStateChanged, bool, bIsLocked);
	UPROPERTY(BlueprintAssignable)
	FOnLockStateChanged OnLockStateChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHintQueued, EAstroGameplayHintType, NewGameplayHint);
	UPROPERTY(BlueprintAssignable)
	FOnHintQueued OnHintQueued;

public:
	/** Adds a hint to the queue. */
	UFUNCTION(BlueprintCallable)
	void QueueHint(const EAstroGameplayHintType GameplayHint);

	/** Removes the least recent hint from the queue (FIFO). */
	UFUNCTION(BlueprintCallable)
	void DequeueHint();

	/** Adds a lock instigator. Those will prevent the UI from dequeueing/displaying hints. */
	void AddLockInstigator(UObject* NewLockInstigator);

	/** Removes a lock instigator, if it's active. */
	void RemoveLockInstigator(UObject* LockInstigator);

public:
	/** @return Least recent hint from the queue (FIFO). */
	UFUNCTION(BlueprintPure)
	EAstroGameplayHintType GetQueuedHint() const { return QueuedHints.IsEmpty() ? EAstroGameplayHintType::None : QueuedHints[0]; }

	/** @return UAstroGameplayHintWidget that represents the specified hint, as defined in GameplayHintWidgetsConfig. */
	UFUNCTION(BlueprintPure)
	TSoftClassPtr<UAstroGameplayHintWidget> GetHintWidget(const EAstroGameplayHintType GameplayHint) const { return GameplayHintWidgets.FindRef(GameplayHint, nullptr); }

	/** @return True if hints are locked. */
	UFUNCTION(BlueprintPure)
	bool AreHintsLocked() const { return ActiveLockInstigators.Num() > 0; }

	/** @return True if the hint passes the display checks. */
	bool CanDisplayHint(const EAstroGameplayHintType GameplayHint) const;

private:
	/** @return True if the hint was ever displayed. */
	bool WasHintDisplayed(const EAstroGameplayHintType GameplayHint) const;

	/** @return True if the hint should always be displayed, no matter if it already was. */
	bool ShouldForceDisplayHint(const EAstroGameplayHintType GameplayHint) const;

};
