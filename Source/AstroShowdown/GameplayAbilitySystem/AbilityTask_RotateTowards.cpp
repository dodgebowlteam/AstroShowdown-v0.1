/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AbilityTask_RotateTowards.h"

#include "AbilitySystemComponent.h"
#include "AstroBall.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_RotateTowards)


UAbilityTask_RotateTowards* UAbilityTask_RotateTowards::RotateTowardsActor(UGameplayAbility* OwningAbility, const FAbilityTaskRotateTowardsParameters& InRotateTowardsParams)
{
	UAbilityTask_RotateTowards* MyObj = NewAbilityTask<UAbilityTask_RotateTowards>(OwningAbility);
	MyObj->RotateTowardsParams = InRotateTowardsParams;
	MyObj->CachedPrecisionDot = FMath::Cos(FMath::DegreesToRadians(InRotateTowardsParams.Precision));
	return MyObj;
}

namespace RotateTowardsPrivate
{
	FORCEINLINE_DEBUGGABLE FVector::FReal CalculateAngleDifferenceDot(const FVector& VectorA, const FVector& VectorB)
	{
		return (VectorA.IsNearlyZero() || VectorB.IsNearlyZero()) ? 1.f : VectorA.CosineAngle2D(VectorB);
	}
}

void UAbilityTask_RotateTowards::Activate()
{
	Super::Activate();

	AActor* Owner = GetOwnerActor();
	if (!Owner)
	{
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		return;
	}

	StartRotateTowardsTarget();
}

void UAbilityTask_RotateTowards::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);

	EndRotateTowardsTarget();

	if (AActor* Owner = GetOwnerActor())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			World->GetTimerManager().ClearTimer(RotationTimerHandle);
		}
	}
}

void UAbilityTask_RotateTowards::StartRotateTowardsTarget()
{
	const APawn* OwnerPawn = Cast<APawn>(GetAvatarActor());
	if (!OwnerPawn)
	{
		return;
	}

	UWorld* World = OwnerPawn->GetWorld();
	if (!World)
	{
		return;
	}

	if (RotateTowardsParams.TargetActor.IsValid())
	{
		const FVector OwnerPawnForward = OwnerPawn->GetActorForwardVector();
		const FVector OwnerPawnPosition = OwnerPawn->GetActorLocation();
		const FVector TargetPosition = RotateTowardsParams.TargetActor->GetActorLocation();
		const FVector::FReal AngleDifference = RotateTowardsPrivate::CalculateAngleDifferenceDot(OwnerPawnForward, (TargetPosition - OwnerPawnPosition));

		// Only rotates if the angle is still not there
		if (AngleDifference < CachedPrecisionDot)
		{
			if (AAIController* OwnerAIController = OwnerPawn->GetController<AAIController>())
			{
				OwnerAIController->SetFocus(RotateTowardsParams.TargetActor.Get(), EAIFocusPriority::Gameplay);
				return;
			}

			if (APlayerController* OwnerController = OwnerPawn->GetController<APlayerController>())
			{
				FVector TargetForward = (RotateTowardsParams.TargetActor->GetActorLocation() - OwnerPawn->GetActorLocation());
				TargetForward.Z = 0.f;
				TargetForward.Normalize();

				const FRotator TargetRotation = TargetForward.Rotation();
				OwnerPawn->GetRootComponent()->SetWorldRotation(TargetRotation);
				EndTask();
				return;
			}

			ensureMsgf(false, TEXT("Controller type not supported"));
		}
	}

	EndTask();
}

void UAbilityTask_RotateTowards::EndRotateTowardsTarget()
{
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwnerActor()))
	{
		if (AAIController* OwnerAIController = OwnerPawn->GetController<AAIController>())
		{
			OwnerAIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}
