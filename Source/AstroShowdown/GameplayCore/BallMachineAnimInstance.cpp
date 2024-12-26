/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "BallMachineAnimInstance.h"
#include "BallMachine.h"

void UBallMachineAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	if (APawn* Owner = TryGetPawnOwner())
	{
		CachedOwner = Cast<ABallMachine>(Owner);
	}
}

void UBallMachineAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	auto ExecuteDefaultRotation = [this]()
	{
		if (APawn* Owner = TryGetPawnOwner())
		{
			// Rotates forward by default
			const FRotator AimRotation = Owner->GetActorForwardVector().ToOrientationRotator();
			TargetRotation.Yaw = AimRotation.Yaw;
		}
	};

	if (!CachedOwner.IsValid())
	{
		ExecuteDefaultRotation();
		return;
	}

	if (CachedOwner->bDead)
	{
		return;
	}

	const TOptional<FVector> CurrentTargetLocation = CachedOwner->GetCurrentTargetLocation();
	if (!CurrentTargetLocation.IsSet())
	{
		ExecuteDefaultRotation();
		return;
	}

	// Finds owner's target location, and uses it to calculate the rotation
	const FVector TargetLocation = CurrentTargetLocation.GetValue();
	const FVector OwnerLocation = CachedOwner->GetActorLocation();
	const FVector AimDirection = (TargetLocation - OwnerLocation).GetSafeNormal();
	const FRotator AimRotation = AimDirection.ToOrientationRotator();
	TargetRotation.Yaw = AimRotation.Yaw;
}
