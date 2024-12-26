/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AsyncAction_FollowSplinePath.h"
#include "AstroTimeDilationSubsystem.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SubsystemUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_FollowSplinePath)


UAsyncAction_FollowSplinePath* UAsyncAction_FollowSplinePath::FollowSplinePath(UObject* InWorldContextObject, ACharacter* Character, USplineComponent* Spline, const float InArriveThresholdSqr, const bool bInReverse)
{
	UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UAsyncAction_FollowSplinePath* Action = NewObject<UAsyncAction_FollowSplinePath>();
	Action->OwnerCharacter = Character;
	Action->SplinePath = Spline;
	Action->World = World;
	Action->ArriveThresholdSqr = InArriveThresholdSqr;
	Action->bReverse = bInReverse;
	Action->RegisterWithGameInstance(World);

	return Action;
}

namespace FollowSplinePathUtils
{
	static int32 GetNextSplinePointByDistanceAlongSpline(USplineComponent* SplinePath, const float DistanceAlongSpline, const bool bReverse)
	{
		if (!SplinePath)
		{
			return 0;
		}

		const int32 SplinePointCount = SplinePath->GetNumberOfSplinePoints();
		for (int32 PointIndex = 0; PointIndex < SplinePointCount; PointIndex++)
		{
			const float SplinePointDistance = SplinePath->GetDistanceAlongSplineAtSplinePoint(PointIndex);
			if (const bool bIsNextSplinePoint = SplinePointDistance > DistanceAlongSpline)
			{
				return bReverse ? PointIndex - 1 : PointIndex;
			}
		}

		return -1;
	}
}

void UAsyncAction_FollowSplinePath::Activate()
{
	Super::Activate();

	const int32 SplinePointsCount = SplinePath.IsValid() ? SplinePath->GetNumberOfSplinePoints() : 0;
	if (!OwnerCharacter.IsValid() || !World.IsValid() || !SplinePath.IsValid() || SplinePointsCount == 0)
	{
		ensureMsgf(false, TEXT("Failed to initialize AsyncAction"));
		Cancel();
		return;
	}

	FollowSplineTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UAsyncAction_FollowSplinePath::OnSplineFollowTick);
}

void UAsyncAction_FollowSplinePath::Cancel()
{
	Super::Cancel();

	if (FollowSplineTimerHandle.IsValid() && World.IsValid())
	{
		World->GetTimerManager().ClearTimer(FollowSplineTimerHandle);
	}

	OnCancel.Broadcast();
}

void UAsyncAction_FollowSplinePath::OnSplineFollowTick()
{
	UCharacterMovementComponent* MovementComponent = OwnerCharacter.IsValid() ? OwnerCharacter->GetCharacterMovement() : nullptr;
	if (!OwnerCharacter.IsValid() || !SplinePath.IsValid() || !World.IsValid() || !MovementComponent)
	{
		Cancel();
		return;
	}

	// Computes the current distance along the spline, based on the Character's position
	const FVector CharacterPosition = FVector::VectorPlaneProject(OwnerCharacter->GetActorLocation(), FVector::UpVector);
	const float CurrentDistanceAlongSpline = SplinePath->GetDistanceAlongSplineAtLocation(CharacterPosition, ESplineCoordinateSpace::World);
	const FVector CurrentPositionAlongSpline = FVector::VectorPlaneProject(SplinePath->GetWorldLocationAtDistanceAlongSpline(CurrentDistanceAlongSpline), FVector::UpVector);

	// Checks if the Character has reached the end of the spline.
	const float ArriveThreshold = FMath::Sqrt(ArriveThresholdSqr);
	const float TotalSplineLength = SplinePath->GetSplineLength();
	if (const bool bReachedSplineEnd = bReverse ? CurrentDistanceAlongSpline <= ArriveThreshold : CurrentDistanceAlongSpline >= TotalSplineLength - ArriveThreshold)
	{
		OnComplete.Broadcast();
		SetReadyToDestroy();
	}
	// Moves the character towards the spline points
	else
	{
		const int32 TargetSplinePoint = FollowSplinePathUtils::GetNextSplinePointByDistanceAlongSpline(SplinePath.Get(), CurrentDistanceAlongSpline, bReverse);
		const FVector TargetPosition = FVector::VectorPlaneProject(SplinePath->GetLocationAtSplinePoint(TargetSplinePoint, ESplineCoordinateSpace::World), FVector::UpVector);
		const FVector MoveDirection = (TargetPosition - CharacterPosition).GetSafeNormal();
		const FVector PlanarMoveDirection = FVector::VectorPlaneProject(MoveDirection, FVector::UpVector);
		MovementComponent->Velocity = PlanarMoveDirection * MovementComponent->GetMaxSpeed();
		OwnerCharacter->AddMovementInput(PlanarMoveDirection);

		FollowSplineTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &UAsyncAction_FollowSplinePath::OnSplineFollowTick);
	}
}

