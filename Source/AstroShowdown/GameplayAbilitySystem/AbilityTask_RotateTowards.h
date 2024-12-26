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
#include "AbilityTask_RotateTowards.generated.h"

class AAstroBall;

USTRUCT(BlueprintType)
struct FAbilityTaskRotateTowardsParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient)
	TWeakObjectPtr<AActor> TargetActor = nullptr;

	/** Rotation stop precision in degrees */
	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Precision = 0.01f;

};


UCLASS()
class ASTROSHOWDOWN_API UAbilityTask_RotateTowards : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"), Category = "Ability|Tasks")
	static UAbilityTask_RotateTowards* RotateTowardsActor(UGameplayAbility* OwningAbility, const FAbilityTaskRotateTowardsParameters& InRotateTowardsParams);

	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished);

private:
	void StartRotateTowardsTarget();
	void EndRotateTowardsTarget();

private:
	UPROPERTY()
	FAbilityTaskRotateTowardsParameters RotateTowardsParams;

	/** Cached rotation stop precision. This will be checked against dot(current rotation, target rotation). */
	UPROPERTY()
	float CachedPrecisionDot = 0.f;

	UPROPERTY()
	FTimerHandle RotationTimerHandle;

};
