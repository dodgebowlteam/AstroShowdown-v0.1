/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "AstroWorldManagerSubsystem.generated.h"


/**
* AstroWorldManagerSubsystem manages all enemy instances in the world.
* 
* NOTE: This is not a reference to Travis Scott - AstroWorld.
*/
UCLASS()
class UAstroWorldManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

#pragma region UWorldSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
#pragma endregion


#pragma region UAstroWorldManagerSubsystem
public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FAstroWorldEnemyCountChanged, int32/* OldCount*/, int32/*NewCount*/);
	FAstroWorldEnemyCountChanged OnAliveEnemyCountChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstroWorldGenericEvent);
	FAstroWorldGenericEvent OnInitializedAllEnemies;

	DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyRegisterEvent, AActor*);
	FEnemyRegisterEvent OnEnemyRegistered;
	FEnemyRegisterEvent OnEnemyUnregistered;

private:
	/** Keeps track of all alive enemies in the world. */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Enemies;

	/** Keeps track of which enemies were initialized. */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> InitializedEnemies;

public:
	UFUNCTION(BlueprintCallable)
	void RegisterEnemy(AActor* Enemy);
	UFUNCTION(BlueprintCallable)
	void UnregisterEnemy(AActor* Enemy, bool bShouldTryResolve);

	UFUNCTION(BlueprintCallable)
	void NotifyEnemyInitialized(AActor* Enemy);

public:
	const TArray<TWeakObjectPtr<AActor>>& GetActiveEnemies() { return Enemies; }

#pragma endregion
};