/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "ModularGameState.h"
#include "AstroGameState.generated.h"

class UAstroGameContextManagerComponent;

UENUM()
enum class EAstroGameState : uint8
{
	Inactive,			// Inactive, waiting for initialization
	Initializing,		// Initialization steps take place here (e.g., enemy spawning, camera panning, etc)
	Active,				// Once Initializing is done, the game is now considered Active
	Resolved,			// The goals were all cleared, and the game is now Resolved (e.g., all enemies were killed, all buttons pressed, etc)
};


/** This message may be used to send/receive game initialize requests. */
USTRUCT(BlueprintType)
struct FAstroGameInitializeRequestMessage
{
	GENERATED_BODY()
};


UCLASS(Config = Game)
class AAstroGameState : public AModularGameStateBase
{
#pragma region AGameState
	GENERATED_BODY()

public:
	AAstroGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion


	// Properties
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstroGameStateGenericEvent);
	DECLARE_MULTICAST_DELEGATE(FAstroGameStateGenericStaticEvent);
	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnAstroGameInitialized;
	FAstroGameStateGenericStaticEvent OnAstroGameInitializedStatic;

	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnAstroGameActivated;
	FAstroGameStateGenericStaticEvent OnAstroGameActivatedStatic;

	/** Notifies when the player should start focusing. */
	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnAstroGameBeginLevelCue;

	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnAstroGameResolved;
	FAstroGameStateGenericStaticEvent OnAstroGameResolvedStatic;

	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnPostAstroGameResolved;

	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnAstroGameReset;

	/** Called right before the current level is restarted. Essentially forwards FAstroCoreDelegates::OnPreRestartCurrentLevel. */
	UPROPERTY(BlueprintAssignable)
	FAstroGameStateGenericEvent OnPreAstroWorldRestart;

private:
	UPROPERTY()
	TObjectPtr<UAstroGameContextManagerComponent> GameContextManagerComponent;

private:
	EAstroGameState CurrentGameState = EAstroGameState::Inactive;
	// ~Properties


public:
	/** Transitions EAstroGameState from Inactive -> Initializing. */
	void InitializeGame();

	/** Transitions EAstroGameState from Initializing -> Active. */
	void ActivateGame();

	/** Transitions EAstroGameState from Active -> Resolved. */
	void ResolveGame();

	/** Transitions EAstroGameState to Inactive, from whatever state it currently is. */
	void ResetGameState();

	/** Alerts all entities that the level has begun. */
	UFUNCTION(BlueprintCallable)
	void BroadcastBeginLevelCue();

protected:
	UFUNCTION()
	void OnPreRestartCurrentLevel(bool& bShouldDeferRestart);

public:
	FORCEINLINE bool IsGameActive() const { return CurrentGameState == EAstroGameState::Active; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsGameResolved() const { return CurrentGameState == EAstroGameState::Resolved; }

	bool IsInPracticeMode() const;
	

};
