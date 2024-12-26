/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroWorldManagerSubsystem.h"
#include "Engine/Engine.h"


void UAstroWorldManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAstroWorldManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UAstroWorldManagerSubsystem::RegisterEnemy(AActor* Enemy)
{
	if (Enemy)
	{
		const int32 OldEnemyCount = Enemies.Num();
		Enemies.Remove(nullptr);
		Enemies.AddUnique(Enemy);

		const int32 NewEnemyCount = Enemies.Num();
		if (OldEnemyCount != NewEnemyCount)
		{
			OnAliveEnemyCountChanged.Broadcast(NewEnemyCount - 1, NewEnemyCount);
		}

		OnEnemyRegistered.Broadcast(Enemy);
	}
}

void UAstroWorldManagerSubsystem::UnregisterEnemy(AActor* Enemy, bool bShouldPlayCallbacks)
{
	if (Enemy && Enemies.Remove(Enemy) > 0)
	{
		if (bShouldPlayCallbacks)
		{
			const int32 NewEnemyCount = Enemies.Num();
			OnAliveEnemyCountChanged.Broadcast(NewEnemyCount + 1, NewEnemyCount);
		}

		OnEnemyUnregistered.Broadcast(Enemy);
	}

	if (InitializedEnemies.Contains(Enemy))
	{
		InitializedEnemies.Remove(Enemy);
	}
}

void UAstroWorldManagerSubsystem::NotifyEnemyInitialized(AActor* Enemy)
{
	InitializedEnemies.Remove(nullptr);
	if (ensure(Enemies.Contains(Enemy)))
	{
		if (Enemy && !InitializedEnemies.Contains(Enemy))
		{
			InitializedEnemies.Add(Enemy);
			
			if (const bool bInitializedAllEnemies = InitializedEnemies.Num() == Enemies.Num())
			{
				OnInitializedAllEnemies.Broadcast();
			}
		}
	}
}
