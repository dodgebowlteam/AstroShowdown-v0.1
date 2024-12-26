/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroAICharacter.h"

#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroGenericGameplayMessageTypes.h"
#include "AstroIndicatorWidgetManagerComponent.h"
#include "AstroWorldManagerSubsystem.h"
#include "HealthAttributeSet.h"
#include "SubsystemUtils.h"

AAstroAICharacter::AAstroAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);														// We're setting this to replicated just in case replay needs it, but we have no intention of enabling multiplayer
	AbilitySystemComponent->SetLooseGameplayTagCount(AstroGameplayTags::Gameplay_Team_Enemy, 1);		// Assumes all AIs are enemies. This is true for now.

	// Attributes are initialized by an Instant GE, during the ASC initialization at runtime
	HealthAttributeSet = CreateDefaultSubobject<UHealthAttributeSet>("HealthAttributeSet");
}

void AAstroAICharacter::BeginPlay()
{
	Super::BeginPlay();

	// Listens to game start. This may be used to spawn the actor when the level is initialized.
	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameInitialized.AddDynamic(this, &AAstroAICharacter::OnAstroWorldInitialized);
		AstroGameState->OnAstroGameActivated.AddDynamic(this, &AAstroAICharacter::OnAstroWorldActivated);
	}
}

void AAstroAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HealthAttributeSet)
	{
		HealthAttributeSet->OnDamaged.RemoveDynamic(this, &AAstroAICharacter::OnDamaged);
	}

	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		const bool bShouldTryResolve = EndPlayReason == EEndPlayReason::Type::Destroyed;
		AstroWorldManagerSubsystem->UnregisterEnemy(this, bShouldTryResolve);
	}
}

void AAstroAICharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	check(AbilitySystemComponent && "::ERROR: AbilitySystemComponent was not initialized for AAstroAICharacter");

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Initializes ASC attributes through an instant GE
		ResetAbilitySystemProperties();
	}

	if (HealthAttributeSet)
	{
		HealthAttributeSet->OnDamaged.AddDynamic(this, &AAstroAICharacter::OnDamaged);
	}
}

void AAstroAICharacter::ResetAbilitySystemProperties()
{
	if (AbilitySystemComponent)
	{
		ensure(InitialStatsGE);
		if (const FGameplayEffectSpecHandle InitialStatsSpec = AbilitySystemComponent->MakeOutgoingSpec(InitialStatsGE, 0.f, {}); InitialStatsSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*InitialStatsSpec.Data.Get());
		}
	}
}

void AAstroAICharacter::StartReviveCounter()
{
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
		HealthAttributeSet->OnReviveFinished.AddUObject(this, &AAstroAICharacter::OnReviveCounterFinished);
		HealthAttributeSet->OnReviveCounterChanged.AddUObject(this, &AAstroAICharacter::OnReviveCounterChanged);
	}

	// Starts listening to world resolve. Revive will be stopped if the world is resolved before the revive duration is reached.
	AstroGameState->OnAstroGameResolved.AddDynamic(this, &AAstroAICharacter::OnAstroWorldResolved);
}

void AAstroAICharacter::StopReviveCounter()
{
	// Removes the revive gameplay effect
	if (AbilitySystemComponent && ReviveEffectHandle.IsValid())
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

	// Stops listening to game resolve, as revive was already stopped
	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameResolved.RemoveDynamic(this, &AAstroAICharacter::OnAstroWorldResolved);
	}

	OnReviveCounterStopped_BP();
}

void AAstroAICharacter::Die_Implementation(const FHitResult& DeathHitResult)
{
	bDead = true;

	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		constexpr bool bShouldTryResolve = true;
		AstroWorldManagerSubsystem->UnregisterEnemy(this, bShouldTryResolve);
	}

	StartReviveCounter();
}

void AAstroAICharacter::Revive_Implementation()
{
	bDead = false;

	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManagerSubsystem->RegisterEnemy(this);
	}

	const FAstroGenericNPCReviveMessage ReviveMessagePayload;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_NPC_Revive, ReviveMessagePayload);

	StopReviveCounter();
}

void AAstroAICharacter::OnDamaged(const float OldHealthValue, const float NewHealthValue, const FHitResult& HitResult)
{
	if (const bool bJustDied = OldHealthValue > 0.f && NewHealthValue <= 0.f)
	{
		Die(HitResult);
	}
}

void AAstroAICharacter::OnReviveCounterFinished()
{
	Revive();
	OnReviveCounterFinished_BP();
}

void AAstroAICharacter::OnReviveCounterChanged(const float OldReviveCounterValue, const float NewReviveCounterValue)
{
	OnReviveCounterChanged_BP(OldReviveCounterValue, NewReviveCounterValue);
}

void AAstroAICharacter::OnAstroWorldInitialized_Implementation()
{
	if (UAstroWorldManagerSubsystem* AstroWorldManagerSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroWorldManagerSubsystem>(this))
	{
		AstroWorldManagerSubsystem->RegisterEnemy(this);
	}
}

void AAstroAICharacter::OnAstroWorldActivated_Implementation()
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

void AAstroAICharacter::OnAstroWorldResolved()
{
	StopReviveCounter();
}

float AAstroAICharacter::GetReviveDuration() const
{
	return HealthAttributeSet ? HealthAttributeSet->GetReviveDuration() : 0.f;
}
