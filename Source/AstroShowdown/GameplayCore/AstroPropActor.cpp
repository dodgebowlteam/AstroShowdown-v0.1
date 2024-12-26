/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroPropActor.h"

#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HealthAttributeSet.h"

AAstroPropActor::AAstroPropActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	PropStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("PropStaticMeshComponent");

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);

	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>("HealthAttributeSet");
	HealthAttributeSet->InitMaxHealth(1.0f);
	HealthAttributeSet->InitCurrentHealth(1.0f);
	HealthAttributeSet->InitDamage(0.f);
}

void AAstroPropActor::BeginPlay()
{
	Super::BeginPlay();

	if (ensure(HealthAttributeSet))
	{
		HealthAttributeSet->OnDamaged.AddDynamic(this, &AAstroPropActor::OnDamaged);
	}
}

void AAstroPropActor::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ensure(HealthAttributeSet))
	{
		HealthAttributeSet->OnDamaged.RemoveDynamic(this, &AAstroPropActor::OnDamaged);
	}
}

void AAstroPropActor::OnDamaged_Implementation(float OldHealthValue, float NewHealthValue, const FHitResult& HitResult)
{
	if (const bool bJustDied = OldHealthValue > 0.f && NewHealthValue <= 0.f)
	{
		if (bShouldAutoDestroyOnDeath)
		{
			SetLifeSpan(0.15f);
		}

		OnPropDied();
	}
}
