/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AbilityTask_SpawnBall.h"

#include "AbilitySystemComponent.h"
#include "AstroBall.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_SpawnBall)

namespace AbilityTaskSpawnBallVars
{
	bool ShouldSpawnFromPool = true;
	static FAutoConsoleVariableRef CVarInitialBulletTimeCharges(
		TEXT("AstroBall.ShouldSpawnFromPool"),
		ShouldSpawnFromPool,
		TEXT("When true, will spawn balls from a pool."),
		ECVF_Default);
}


UAbilityTask_SpawnBall* UAbilityTask_SpawnBall::SpawnBall(UGameplayAbility* OwningAbility, const FSpawnBallParameters& SpawnParameters)
{
	UAbilityTask_SpawnBall* MyObj = NewAbilityTask<UAbilityTask_SpawnBall>(OwningAbility);
	MyObj->BallSpawnParameters = SpawnParameters;
	MyObj->bHasSpawnedActor = false;
	return MyObj;
}

void UAbilityTask_SpawnBall::Activate()
{
	Super::Activate();

	// BeginSpawningActor and FinishSpawningActor are called automatically when the node is invoked via BP, but not via C++
	AAstroBall* SpawnedBall = nullptr;
	if (BeginSpawningActor(Ability, BallSpawnParameters.BallClass, SpawnedBall))
	{
		FinishSpawningActor(Ability, SpawnedBall);
	}
}


////////////////////////////////////////////////////////////////
// UAbilityTask_SpawnBall
////////////////////////////////////////////////////////////////

bool UAbilityTask_SpawnBall::BeginSpawningActor(UGameplayAbility* OwningAbility, TSubclassOf<AAstroBall> BallClass, AAstroBall*& SpawnedBall)
{
	if (bHasSpawnedActor)
	{
		return false;
	}

	bHasSpawnedActor = true;

	if (Ability)
	{
		UWorld* const World = GEngine->GetWorldFromContextObject(OwningAbility, EGetWorldErrorMode::LogAndReturnNull);
		if (World)
		{
			AActor* Owner = OwningAbility->GetActorInfo().OwnerActor.Get();
			APawn* Instigator = Owner->GetInstigator();

			if (AbilityTaskSpawnBallVars::ShouldSpawnFromPool)
			{
				SpawnedBall = UAstroBallPoolManager::Get(this)->SpawnBall(BallClass);
			}
			else
			{
				SpawnedBall = World->SpawnActorDeferred<AAstroBall>(BallClass, FTransform::Identity, Owner, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			}
		}
	}

	if (!SpawnedBall)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			SpawnFailure.Broadcast(nullptr);
		}
		return false;
	}

	return true;
}

void UAbilityTask_SpawnBall::FinishSpawningActor(UGameplayAbility* OwningAbility, AAstroBall* SpawnedBall)
{
	if (ensure(SpawnedBall))
	{
		const FVector BallStartPosition = BallSpawnParameters.bApplyPlaneHeightFixup ?
			AAstroBall::ApplyTravelHeightFixupToPosition(BallSpawnParameters.ThrowStartLocation) : BallSpawnParameters.ThrowStartLocation;
		const FVector BallTargetPosition = BallSpawnParameters.bApplyPlaneHeightFixup ?
			AAstroBall::ApplyTravelHeightFixupToPosition(BallSpawnParameters.ThrowTargetLocation) : BallSpawnParameters.ThrowTargetLocation;

		FVector ThrowDirection = (BallTargetPosition - BallStartPosition);
		ThrowDirection.Normalize();

		// Finishes spawn process
		const FVector BallLocation = BallStartPosition + ThrowDirection * BallSpawnParameters.ThrowStartOffset;
		if (AbilityTaskSpawnBallVars::ShouldSpawnFromPool)
		{
			constexpr bool bSweep = false;
			SpawnedBall->SetActorLocation(BallLocation, bSweep, nullptr, ETeleportType::TeleportPhysics);
			SpawnedBall->ActivateFromPool();
		}
		else
		{
			FTransform BallTransform;
			BallTransform.SetLocation(BallLocation);
			SpawnedBall->FinishSpawning(BallTransform);
		}

		// Throws the ball, passing GE effect spec handle to it
		if (BallSpawnParameters.DamageEffectHandle.IsValid())
		{
			SpawnedBall->Throw(BallSpawnParameters.DamageEffectHandle, ThrowDirection, BallSpawnParameters.ThrowSpeed);
		}

		if (ShouldBroadcastAbilityTaskDelegates())
		{
			SpawnSuccess.Broadcast(SpawnedBall);
		}
	}

	EndTask();
}


////////////////////////////////////////////////////////////////
// UAstroBallPoolManager
////////////////////////////////////////////////////////////////

UWorld* UAstroBallPoolManager::GetWorld() const
{
	return Cast<UWorld>(GetOuter());
}

AAstroBall* UAstroBallPoolManager::SpawnBall(TSubclassOf<AAstroBall> BallClass)
{
	AAstroBall* Ball = GetBallFromPool(BallClass);
	ensureMsgf(Ball->IsDead(), TEXT("Assumes balls pulled from the pool are always dead"));
	Ball->ActivateFromPool();
	return Ball;
}

void UAstroBallPoolManager::ResizePoolOfClass(TSubclassOf<AAstroBall> BallClass)
{
	constexpr int32 ResizePoolCount = 30;

	FBallArray& BallsArray = PooledBalls.FindOrAdd(BallClass);
	for (int32 Index = 0; Index < ResizePoolCount; Index++)
	{
		// Ball activation/deactivation events will modify PooledBalls, so there's no need to do it explicitly here
		AAstroBall* AstroBall = Cast<AAstroBall>(GetWorld()->SpawnActor(BallClass.Get()));
		AstroBall->OnAstroBallActivated.AddUObject(this, &UAstroBallPoolManager::OnAstroBallActivated);
		AstroBall->OnAstroBallDeactivated.AddDynamic(this, &UAstroBallPoolManager::OnAstroBallDeactivated);
		AstroBall->ReturnToPool();
		AllBalls.Add(AstroBall);
	}
}

bool UAstroBallPoolManager::IsPoolOfClassEmpty(TSubclassOf<AAstroBall> BallClass) const
{
	return !PooledBalls.Contains(BallClass) || PooledBalls[BallClass].Balls.IsEmpty();
}

AAstroBall* UAstroBallPoolManager::GetBallFromPool(TSubclassOf<AAstroBall> BallClass)
{
	if (IsPoolOfClassEmpty(BallClass))
	{
		ResizePoolOfClass(BallClass);
	}

	// Pulls the ball from the current index and increments it
	const FBallArray& BallsArray = PooledBalls[BallClass];
	if (ensure(BallsArray.Balls.IsValidIndex(0)))
	{
		return BallsArray.Balls[0];
	}

	return nullptr;
}

void UAstroBallPoolManager::OnAstroBallActivated(AAstroBall* ActiveBall)
{
	TSubclassOf<AAstroBall> ActiveBallClass = ActiveBall->GetClass();
	if (ensure(PooledBalls.Contains(ActiveBallClass)))
	{
		PooledBalls[ActiveBallClass].Balls.Remove(ActiveBall);
	}
}

void UAstroBallPoolManager::OnAstroBallDeactivated(AAstroBall* InactiveBall)
{
	TSubclassOf<AAstroBall> ActiveBallClass = InactiveBall->GetClass();
	if (ensure(PooledBalls.Contains(ActiveBallClass)))
	{
		PooledBalls[ActiveBallClass].Balls.AddUnique(InactiveBall);
	}
}

void UAstroBallPoolManager::OnWorldBeginTearDown(UWorld* World)
{
	// Removes UAstroBallPoolManager from the root to allow its garbage collection, as it was added to the root during spawn
	RemoveFromRoot();

	// Removes the activation/deactivation delegates from all balls
	for (TWeakObjectPtr<AAstroBall> Ball : AllBalls)
	{
		if (Ball.IsValid())
		{
			Ball->OnAstroBallActivated.RemoveAll(this);
			Ball->OnAstroBallDeactivated.RemoveDynamic(this, &UAstroBallPoolManager::OnAstroBallDeactivated);
		}
	}
	AllBalls.Empty();
}

UAstroBallPoolManager* UAstroBallPoolManager::Get(const UObject* WorldContextObject)
{
	if (!Instance.IsValid())
	{
		// Spawns UAstroBallPoolManager and adds it to the world root to avoid garbage collection
		Instance = NewObject<UAstroBallPoolManager>(WorldContextObject->GetWorld());
		Instance->AddToRoot();

		FWorldDelegates::OnWorldBeginTearDown.AddUObject(Instance.Get(), &UAstroBallPoolManager::OnWorldBeginTearDown);
	}

	return Instance.Get();
}
