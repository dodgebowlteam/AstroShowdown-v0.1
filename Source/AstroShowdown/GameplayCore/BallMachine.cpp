/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "BallMachine.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "HealthAttributeSet.h"
#include "TimerManager.h"

ABallMachine::ABallMachine(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BallMachineMesh = CreateDefaultSubobject<USkeletalMeshComponent>("BallMachineMesh");
	SetRootComponent(BallMachineMesh);
}

void ABallMachine::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void ABallMachine::NativeDie()
{
	Super::NativeDie();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAbilities();
	}
}

void ABallMachine::ActivateShootingAbility(const bool bInstant)
{
	auto ActivateShootingAbilityFn = [WeakThis = TWeakObjectPtr<ABallMachine>(this)]()
	{
		if (!WeakThis.IsValid() || !WeakThis->AbilitySystemComponent || WeakThis->bDead)
		{
			return;
		}

		FGameplayAbilitySpecHandle ThrowAbilityHandle = WeakThis->AbilitySystemComponent->GiveAbility(WeakThis->ThrowBallAbilityClass);
		WeakThis->AbilitySystemComponent->TryActivateAbility(ThrowAbilityHandle);
	};

	if (ThrowParameters.ThrowStartDelay <= 0.f || bInstant)
	{
		ActivateShootingAbilityFn();
	}
	else if (UWorld* World = GetWorld())
	{
		FTimerHandle ThrowAbilityTimerHandle;
		World->GetTimerManager().SetTimer(ThrowAbilityTimerHandle, FTimerDelegate::CreateLambda(ActivateShootingAbilityFn), ThrowParameters.ThrowStartDelay, false);
	}
}

void ABallMachine::SetCurrentTargetLocation(const FVector& NewTargetLocation)
{
	CurrentTargetLocation = NewTargetLocation;
	OnTargetLocationUpdated.Broadcast(NewTargetLocation);
}

