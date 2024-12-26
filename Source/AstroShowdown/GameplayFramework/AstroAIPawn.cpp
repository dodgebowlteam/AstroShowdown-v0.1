/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroAIPawn.h"

#include "AbilitySystemComponent.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroGenericGameplayMessageTypes.h"
#include "AstroIndicatorWidgetManagerComponent.h"
#include "AstroWorldManagerSubsystem.h"
#include "HealthAttributeSet.h"
#include "SubsystemUtils.h"

AAstroAIPawn::AAstroAIPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);			// We're setting this to replicated just in case, but we have no intention of enabling multiplayer
	AbilitySystemComponent->SetLooseGameplayTagCount(AstroGameplayTags::Gameplay_Team_Enemy, 1);		// Assumes all AIs are enemies. This is true for now.

	// Attributes are initialized by an Instant GE, during the ASC initialization at runtime
	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>("HealthAttributeSet");
}

void AAstroAIPawn::BeginPlay()
{
	Super::BeginPlay();

	// Listens to game start. This may be used to spawn the actor when the level is initialized.
	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameReset.AddUniqueDynamic(this, &AAstroAIPawn::OnAstroGameReset);
		AstroGameState->OnAstroGameInitialized.AddUniqueDynamic(this, &AAstroAIPawn::OnAstroWorldInitialized);
		AstroGameState->OnAstroGameActivated.AddUniqueDynamic(this, &AAstroAIPawn::OnAstroWorldActivated);
		AstroGameState->OnAstroGameResolved.AddUniqueDynamic(this, &AAstroAIPawn::OnAstroGameResolved);
	}
}

void AAstroAIPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	HealthAttributeSet->OnDamaged.RemoveDynamic(this, &AAstroAIPawn::OnDamaged);

	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		const bool bShouldTryResolve = EndPlayReason == EEndPlayReason::Type::Destroyed;
		AstroWorldManagerSubsystem->UnregisterEnemy(this, bShouldTryResolve);
	}

	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameReset.RemoveDynamic(this, &AAstroAIPawn::OnAstroGameReset);
		AstroGameState->OnAstroGameInitialized.RemoveDynamic(this, &AAstroAIPawn::OnAstroWorldInitialized);
		AstroGameState->OnAstroGameActivated.RemoveDynamic(this, &AAstroAIPawn::OnAstroWorldActivated);
		AstroGameState->OnAstroGameResolved.RemoveDynamic(this, &AAstroAIPawn::OnAstroGameResolved);
	}
}

void AAstroAIPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	check(AbilitySystemComponent && "::ERROR: AbilitySystemComponent was not initialized for AAstroAIPawn");

	// NOTE (GAS Docs): If we create a controller for this pawn, then we should override its AcknowledgePossession method,
	// and initialize the ASC there as well
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Initializes ASC attributes through an instant GE
		ResetAbilitySystemProperties();
	}

	if (HealthAttributeSet)
	{
		HealthAttributeSet->OnDamaged.AddDynamic(this, &AAstroAIPawn::OnDamaged);
	}
}

void AAstroAIPawn::ResetAbilitySystemProperties()
{
	if (AbilitySystemComponent)
	{
		ensure(PawnInitialStatsGE);
		if (const FGameplayEffectSpecHandle InitialStatsSpec = AbilitySystemComponent->MakeOutgoingSpec(PawnInitialStatsGE, 0.f, {}); InitialStatsSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*InitialStatsSpec.Data.Get());
		}
	}
}

void AAstroAIPawn::StartReviveCounter()
{
	if (ReviveEffectHandle.IsValid())
	{
		return;
	}

	// Prevents revive from starting if the world is already resolved.
	const UWorld* World = GetWorld();
	AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr;
	if (!World || !AstroGameState || AstroGameState->IsGameResolved())
	{
		return;
	}

	// Applies the revive gameplay effect
	if (AbilitySystemComponent && ensure(ReviveGE))
	{
		if (const FGameplayEffectSpecHandle ReviveSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(ReviveGE, 0.f, {}); ReviveSpecHandle.IsValid())
		{
			ReviveEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*ReviveSpecHandle.Data.Get());
		}
	}

	// Starts listening to the revive counter
	if (HealthAttributeSet)
	{
		HealthAttributeSet->OnReviveFinished.AddUObject(this, &AAstroAIPawn::OnReviveCounterFinished);
		HealthAttributeSet->OnReviveCounterChanged.AddUObject(this, &AAstroAIPawn::OnReviveCounterChanged);
	}
}

void AAstroAIPawn::StopReviveCounter()
{
	if (!ReviveEffectHandle.IsValid())
	{
		return;
	}

	// Removes the revive gameplay effect
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(ReviveEffectHandle);
		ReviveEffectHandle.Invalidate();
	}

	// Stops listening to the revive counter
	if (HealthAttributeSet)
	{
		HealthAttributeSet->OnReviveFinished.RemoveAll(this);
		HealthAttributeSet->OnReviveCounterChanged.RemoveAll(this);
	}

	OnReviveCounterStopped_BP();
}

void AAstroAIPawn::NativeDie()
{
	if (bDead)
	{
		return;
	}

	bDead = true;

	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		constexpr bool bShouldTryResolve = true;
		AstroWorldManagerSubsystem->UnregisterEnemy(this, bShouldTryResolve);
	}

	StartReviveCounter();

	Die_BP();
}

void AAstroAIPawn::NativeRevive()
{
	// Prevents revive if the pawn is not even dead.
	if (!bDead)
	{
		return;
	}

	bDead = false;

	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManagerSubsystem->RegisterEnemy(this);
	}

	StopReviveCounter();

	const FAstroGenericNPCReviveMessage ReviveMessagePayload;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_NPC_Revive, ReviveMessagePayload);

	Revive_BP();
}

void AAstroAIPawn::OnDamaged(const float OldHealthValue, const float NewHealthValue, const FHitResult& HitResult)
{
	if (const bool bJustDied = OldHealthValue > 0.f && NewHealthValue <= 0.f)
	{
		NativeDie();
	}
}

void AAstroAIPawn::OnReviveCounterFinished()
{
	NativeRevive();
	OnReviveCounterFinished_BP();
}

void AAstroAIPawn::OnReviveCounterChanged(const float OldReviveCounterValue, const float NewReviveCounterValue)
{
	OnReviveCounterChanged_BP(OldReviveCounterValue, NewReviveCounterValue);
}

void AAstroAIPawn::OnAstroGameReset()
{
	bDead = false;
}

void AAstroAIPawn::OnAstroWorldInitialized_Implementation()
{
	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManagerSubsystem->RegisterEnemy(this);
	}
}

void AAstroAIPawn::OnAstroWorldActivated_Implementation()
{
	// NOTE: UAstroIndicatorWidgetManagerComponent is initialized during BeginPlay, so we can't assume it exists before that.
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			if (UAstroIndicatorWidgetManagerComponent* IndicatorComponent = GameState->GetComponentByClass<UAstroIndicatorWidgetManagerComponent>())
			{
				FAstroIndicatorWidgetSettings IndicatorSettings;
				IndicatorSettings.Owner = this;
				IndicatorSettings.Icon = IndicatorIcon;
				IndicatorSettings.bUseEnemyIndicator = true;
				IndicatorComponent->RegisterActorIndicator(IndicatorSettings);
			}
		}
	}
}

void AAstroAIPawn::OnAstroGameResolved()
{
	StopReviveCounter();
	NativeDie();
}

float AAstroAIPawn::GetReviveDuration() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetReviveDuration() : 0.f;
}
