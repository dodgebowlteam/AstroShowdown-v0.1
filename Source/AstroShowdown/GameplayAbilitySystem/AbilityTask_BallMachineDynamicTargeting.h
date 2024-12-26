/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_BallMachineDynamicTargeting.generated.h"

class ABallMachine;

UCLASS()
class UAbilityTask_BallMachineDynamicTargeting : public UAbilityTask
{
#pragma region UAbilityTask
	GENERATED_BODY()
	
public:
	UAbilityTask_BallMachineDynamicTargeting(const FObjectInitializer& InObjectInitializer);

	virtual void TickTask(float DeltaTime) override;
#pragma endregion

#pragma region UAbilityTask_BallMachineDynamicTargeting
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_BallMachineDynamicTargeting* ExecuteBallMachineDynamicTargeting(UGameplayAbility* OwningAbility, ABallMachine* InBallMachine, FName TaskInstanceName = NAME_None);

private:
	TWeakObjectPtr<AActor> FindOrGetDynamicTarget();
	TWeakObjectPtr<UMovementComponent> FindOrGetDynamicTargetMovementComponent();

public:
	static void ClampTargetPositionWithLineOfSight(AActor* Instigator, const FVector& StartPosition, OUT FVector& OutTargetPosition);

private:
	float LineOfSightClampCounter = 0.f;
	float CurrentLineOfSightCheckedDistance = 99999.f;

private:
	UPROPERTY()
	TWeakObjectPtr<ABallMachine> BallMachine = nullptr;
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedThrowTargetActor = nullptr;
	UPROPERTY()
	TWeakObjectPtr<UMovementComponent> CachedThrowTargetMovementComponent = nullptr;

#pragma endregion
};