/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Animation/AnimInstance.h"
#include "BallMachineAnimInstance.generated.h"

class ABallMachine;

UCLASS()
class ASTROSHOWDOWN_API UBallMachineAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FRotator TargetRotation = FRotator::ZeroRotator;

	UPROPERTY()
	TWeakObjectPtr<ABallMachine> CachedOwner;
};
