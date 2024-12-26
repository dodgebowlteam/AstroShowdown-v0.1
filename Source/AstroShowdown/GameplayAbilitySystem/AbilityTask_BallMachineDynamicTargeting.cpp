/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AbilityTask_BallMachineDynamicTargeting.h"
#include "BallMachine.h"
#include "GameFramework/MovementComponent.h"
#include "Kismet/GameplayStatics.h"

namespace BallMachineDynamicTargetingVars
{
	static bool bDebugLineofSightEnabled = false;
	static FAutoConsoleVariableRef CVarDebugLineofSightEnabled(
		TEXT("BallMachine.DebugLineofSightEnabled"),
		bDebugLineofSightEnabled,
		TEXT("When enabled, will allow debug drawing of target LoS checks."),
		ECVF_Default);

	constexpr float LineOfSightClampInterval = 0.15f;
}

UAbilityTask_BallMachineDynamicTargeting::UAbilityTask_BallMachineDynamicTargeting(const FObjectInitializer& InObjectInitializer) : Super(InObjectInitializer)
{
	bTickingTask = true;
	LineOfSightClampCounter = BallMachineDynamicTargetingVars::LineOfSightClampInterval;		// Forces first tick to do LoS
}

UAbilityTask_BallMachineDynamicTargeting* UAbilityTask_BallMachineDynamicTargeting::ExecuteBallMachineDynamicTargeting(UGameplayAbility* OwningAbility, ABallMachine* InBallMachine, FName TaskInstanceName)
{
	UAbilityTask_BallMachineDynamicTargeting* MyObj = NewAbilityTask<UAbilityTask_BallMachineDynamicTargeting>(OwningAbility, TaskInstanceName);
	MyObj->BallMachine = InBallMachine;
	return MyObj;
}

void UAbilityTask_BallMachineDynamicTargeting::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!BallMachine.IsValid())
	{
		return;
	}

	if (TWeakObjectPtr<AActor> DynamicTargetActor = FindOrGetDynamicTarget(); DynamicTargetActor.IsValid())
	{
		// Checks if target position is within line of sight, and if it's not, clamps it to the closest object in LoS.
		// NOTE (1): This operation performs a line trace, so as an optimization, we're NOT gonna do it every frame.
		// NOTE (2): Each time LoS is calculated, we're going to cache the distance from ball machine to target, and use it as a limiter
		// to avoid going beyond geometry in frames where LoS is not calculated, but player is still behind an object.
		const FVector BallMachinePosition = BallMachine->GetActorLocation();
		FVector TargetActorPosition = DynamicTargetActor->GetActorLocation();
		LineOfSightClampCounter += DeltaTime;
		if (LineOfSightClampCounter >= BallMachineDynamicTargetingVars::LineOfSightClampInterval)
		{
			UAbilityTask_BallMachineDynamicTargeting::ClampTargetPositionWithLineOfSight(BallMachine.Get(), BallMachinePosition, OUT TargetActorPosition);
			CurrentLineOfSightCheckedDistance = (TargetActorPosition - BallMachinePosition).Length();
			LineOfSightClampCounter = FMath::RandRange(-0.02f, 0.02f);		// Avoids bunching LoS checks in the same frame
		}

		// Calculates target position. Might clamp it with the LoS limiter.
		constexpr float MinDistanceFromTarget = 200.f;
		const FVector TargetDirection = (TargetActorPosition - BallMachinePosition).GetSafeNormal2D();
		const float DistanceFromPawn = (TargetActorPosition - BallMachinePosition).Length();
		const float DistanceFromTarget = FMath::Max(DistanceFromPawn, MinDistanceFromTarget);
		const float DistanceFromTargetWithLimiter = FMath::Min(DistanceFromTarget, CurrentLineOfSightCheckedDistance);		// Uses last LoS checked distance, to avoid having to check every frame
		FVector ThrowTargetPosition = BallMachinePosition + TargetDirection * DistanceFromTargetWithLimiter;

		// If aiming at a moving target, will aim at the direction that the target is heading
		if (TWeakObjectPtr<UMovementComponent> DynamicTargetActorMovementComponent = FindOrGetDynamicTargetMovementComponent(); DynamicTargetActorMovementComponent.IsValid())
		{
			const FThrowAtTargetParameters& ThrowParameters = BallMachine->ThrowParameters;
			const FVector TargetHeading = DynamicTargetActor->GetActorForwardVector();
			const float TargetSpeed = DynamicTargetActorMovementComponent->Velocity.Length();
			ThrowTargetPosition += TargetHeading * TargetSpeed * ThrowParameters.ThrowLookaheadStrength;
		}

		// Updates the current target position
		BallMachine->SetCurrentTargetLocation(ThrowTargetPosition);
	}
}

TWeakObjectPtr<AActor> UAbilityTask_BallMachineDynamicTargeting::FindOrGetDynamicTarget()
{
	if (!CachedThrowTargetActor.IsValid())
	{
		if (BallMachine.IsValid())
		{
			const FThrowAtTargetParameters& ThrowParameters = BallMachine->ThrowParameters;
			TArray<AActor*> OutCandidateActors;
			UGameplayStatics::GetAllActorsOfClass(this, ThrowParameters.ThrowTargetActorClass, OUT OutCandidateActors);

			CachedThrowTargetActor = OutCandidateActors.IsValidIndex(0) ? OutCandidateActors[0] : nullptr;
		}
	}

	return CachedThrowTargetActor;
}

TWeakObjectPtr<UMovementComponent> UAbilityTask_BallMachineDynamicTargeting::FindOrGetDynamicTargetMovementComponent()
{
	if (!CachedThrowTargetMovementComponent.IsValid())
	{
		if (CachedThrowTargetActor.IsValid())
		{
			CachedThrowTargetMovementComponent = CachedThrowTargetActor->FindComponentByClass<UMovementComponent>();
		}
	}

	return CachedThrowTargetMovementComponent;
}

void UAbilityTask_BallMachineDynamicTargeting::ClampTargetPositionWithLineOfSight(AActor* Instigator, const FVector& StartPosition, OUT FVector& OutTargetPosition)
{
	// Performs a trace to check if the aim beam hit something.
	// NOTE: We're moving the trace origin to near the ground to ensure it doesn't go above walls.
	constexpr bool bTraceComplex = false, bIgnoreSelf = true;
	const EDrawDebugTrace::Type DrawDebugType = BallMachineDynamicTargetingVars::bDebugLineofSightEnabled ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	const TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes
	{
		EObjectTypeQuery::ObjectTypeQuery1,		// WorldStatic
		EObjectTypeQuery::ObjectTypeQuery2,		// WorldDynamic
	};
	const TArray<AActor*> ActorsToIgnore{ };
	const FVector HeightOffset{ 0, 0, 10.f };

	FHitResult OutHit;
	UKismetSystemLibrary::LineTraceSingleForObjects(Instigator,
		FVector::VectorPlaneProject(StartPosition, FVector::UpVector) + HeightOffset,
		FVector::VectorPlaneProject(OutTargetPosition, FVector::UpVector) + HeightOffset,
		TraceObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, OUT OutHit, bIgnoreSelf);

	OutTargetPosition = OutHit.bBlockingHit ? FVector{ OutHit.Location.X, OutHit.Location.Y, OutTargetPosition.Z } : OutTargetPosition;
}
