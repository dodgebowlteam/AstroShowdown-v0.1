/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"
#include "AbilityTask_SpawnBall.generated.h"

////////////////////////////////////////////////////////////////
// UAbilityTask_SpawnBall
////////////////////////////////////////////////////////////////

class AAstroBall;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpawnBallDelegate, AActor*, SpawnedActor);

USTRUCT(BlueprintType)
struct FSpawnBallParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<AAstroBall> BallClass;

	UPROPERTY(BlueprintReadWrite)
	FVector ThrowStartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FVector ThrowTargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float ThrowStartOffset = 200.f;

	UPROPERTY(BlueprintReadWrite)
	float ThrowSpeed = 0.f;

	UPROPERTY(BlueprintReadWrite)
	FGameplayEffectSpecHandle DamageEffectHandle;

	/** When enabled, the ball will be spawned at a fixed Z location, as opposed to ThrowStartLocation::Z. */
	UPROPERTY(BlueprintReadWrite)
	uint8 bApplyPlaneHeightFixup : 1 = true;

};


UCLASS()
class ASTROSHOWDOWN_API UAbilityTask_SpawnBall : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FSpawnBallDelegate	SpawnSuccess;

	/** Called when we can't spawn: on clients or potentially on server if they fail to spawn (rare) */
	UPROPERTY(BlueprintAssignable)
	FSpawnBallDelegate	SpawnFailure;

	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"), Category = "Ability|Tasks")
	static UAbilityTask_SpawnBall* SpawnBall(UGameplayAbility* OwningAbility, const FSpawnBallParameters& SpawnParameters);

	virtual void Activate() override;

private:
	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"), Category = "Abilities")
	bool BeginSpawningActor(UGameplayAbility* OwningAbility, TSubclassOf<AAstroBall> Class, AAstroBall*& SpawnedActor);

	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"), Category = "Abilities")
	void FinishSpawningActor(UGameplayAbility* OwningAbility, AAstroBall* SpawnedActor);

private:
	UPROPERTY()
	FSpawnBallParameters BallSpawnParameters;
	
	bool bHasSpawnedActor = false;

};


////////////////////////////////////////////////////////////////
// UAstroBallPoolManager
////////////////////////////////////////////////////////////////

USTRUCT()
struct FBallArray
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TObjectPtr<AAstroBall>> Balls;
};

UCLASS()
class ASTROSHOWDOWN_API UAstroBallPoolManager : public UObject
{
	GENERATED_BODY()

#pragma region UObject
public:
	virtual UWorld* GetWorld() const override;
#pragma endregion


#pragma region UAstroBallPoolManager
public:
	UFUNCTION(BlueprintCallable)
	static UAstroBallPoolManager* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	AAstroBall* SpawnBall(TSubclassOf<AAstroBall> BallClass);

private:
	void ResizePoolOfClass(TSubclassOf<AAstroBall> BallClass);
	bool IsPoolOfClassEmpty(TSubclassOf<AAstroBall> BallClass) const;
	AAstroBall* GetBallFromPool(TSubclassOf<AAstroBall> BallClass);

private:
	UFUNCTION()
	void OnAstroBallActivated(AAstroBall* ActiveBall);
	UFUNCTION()
	void OnAstroBallDeactivated(AAstroBall* InactiveBall);
	UFUNCTION()
	void OnWorldBeginTearDown(UWorld* World);

private:
	/** Stores an array of balls for each class type. */
	UPROPERTY()
	TMap<TSubclassOf<AAstroBall>, FBallArray> PooledBalls;

	/** Keeps track of all balls, pooled or active. We need this to unregister the activation/deactivation delegates from balls on world cleanup. */
	UPROPERTY()
	TArray<TWeakObjectPtr<AAstroBall>> AllBalls;

private:
	static inline TWeakObjectPtr<UAstroBallPoolManager> Instance = nullptr;
#pragma endregion

};
