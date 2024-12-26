/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/


#include "AstroCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AstroBall.h"
#include "AstroDashAimComponent.h"
#include "AstroDashBlockComponent.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroInteractionComponent.h"
#include "AstroPlayerAttributeSet.h"
#include "AstroPlayerState.h"
#include "AstroRoomNavigationTypes.h"
#include "AstroStateMachine.h"
#include "AstroThrowAimComponent.h"
#include "AstroTimeDilationSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayAbility_AstroDash.h"
#include "GameplayAbility_AstroThrow.h"
#include "GameplayAbility_BulletTime.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HealthAttributeSet.h"
#include "SubsystemUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroAstroCharacter, Log, All);
DEFINE_LOG_CATEGORY(LogAstroAstroCharacter);

namespace AstroCharacterVars
{
	int32 InitialBulletTimeCharges = 1;
	static FAutoConsoleVariableRef CVarInitialBulletTimeCharges(
		TEXT("AstroCharacter.InitialBulletTimeCharges"),
		InitialBulletTimeCharges,
		TEXT("Determines how many bullet time charges the player has when starting the match."),
		ECVF_Default);

	bool bForceInvulnerable = false;
	static FAutoConsoleVariableRef CVarForceInvulnerable(
		TEXT("AstroCharacter.ForceInvulnerable"),
		bForceInvulnerable,
		TEXT("When enabled, the player can't be damaged."),
		ECVF_Default);
}

namespace AstroCharacterStatics
{
	static const FName BallPickupSocket = "RightHandSocket";
}

AAstroCharacter::AAstroCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AstroStateMachine = CreateDefaultSubobject<UAstroStateMachine>("AstroStateMachine");
	AstroInteractionComponent = CreateDefaultSubobject<UAstroInteractionComponent>("AstroInteractionComponent");
	AstroDashBlockComponent = CreateDefaultSubobject<UAstroDashBlockComponent>("AstroDashBlockComponent");
}

void AAstroCharacter::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameResolved.AddUniqueDynamic(this, &AAstroCharacter::OnGameResolved);
		AstroGameState->OnAstroGameReset.AddUniqueDynamic(this, &AAstroCharacter::OnGameReset);
	}

	// Starts listening to room enter messages
	FGameplayMessageListenerParams<FAstroRoomGenericMessage> RoomEnterListenerParams;
	RoomEnterListenerParams.SetMessageReceivedCallback(this, &AAstroCharacter::OnRoomEntered);
	RoomEnterMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_Room_Enter, RoomEnterListenerParams);
}

void AAstroCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameResolved.RemoveDynamic(this, &AAstroCharacter::OnGameResolved);
		AstroGameState->OnAstroGameReset.RemoveDynamic(this, &AAstroCharacter::OnGameReset);
	}

	// Stops listening to room enter messages
	UGameplayMessageSubsystem::Get(this).UnregisterListener(RoomEnterMessageHandle);
}

void AAstroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Sets the ASC on the Server. Clients do this in OnRep_PlayerState().
	InitializeAbilitySystemComponent();

	// Enters the Idle state
	ensure(IdleStateAbilityClass);
	AstroStateMachine->SwitchState(this, IdleStateAbilityClass);
}

void AAstroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Sets the ASC on the Client. Server does this in OnRep_PlayerState().
	InitializeAbilitySystemComponent();
}

void AAstroCharacter::InitializeAbilitySystemComponent()
{
	// Early-out if the ASC has already been initialized
	if (AbilitySystemComponent)
	{
		return;
	}

	if (AAstroPlayerState* AstroPlayerState = GetPlayerState<AAstroPlayerState>())
	{
		AbilitySystemComponent = AstroPlayerState->GetAbilitySystemComponent();
		AbilitySystemComponent->InitAbilityActorInfo(AstroPlayerState, this);

		// Applies GE with the default values for all attributes
		const FGameplayEffectSpecHandle AstroPlayerStatsSpec = AbilitySystemComponent->MakeOutgoingSpec(AstroPlayerStatsGE, 0.f, {});
		if (AstroPlayerStatsSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*AstroPlayerStatsSpec.Data.Get());
		}

		// Adds default tags
		AbilitySystemComponent->SetLooseGameplayTagCount(AstroGameplayTags::Gameplay_Team_Ally, 1);
		AbilitySystemComponent->RegisterGameplayTagEvent(AstroGameplayTags::AstroState_Fatigued, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAstroCharacter::OnFatigueStateChanged);

		// Grants bullet time to player
		ensure(BulletTimeAbilityClass.Get());
		TSubclassOf<UGameplayAbility> BulletTimeAbilitySubclass = BulletTimeAbilityClass;
		BulletTimeAbilityHandle = AbilitySystemComponent->GiveAbility(BulletTimeAbilitySubclass);

		// Grants dash to player
		ensure(AstroDashAbilityClass.Get());
		TSubclassOf<UGameplayAbility> AstroDashAbilitySubclass = AstroDashAbilityClass;
		AbilitySystemComponent->GiveAbility(AstroDashAbilitySubclass);

		// Binds attribute change callbacks
		PlayerAttributeSet = AstroPlayerState->GetPlayerAttributeSet();
		HealthAttributeSet = AstroPlayerState->GetHealthAttributeSet();
		HealthAttributeSet->OnDamaged.AddDynamic(this, &AAstroCharacter::OnDamaged);
		PlayerAttributeSet->OnStaminaChanged.AddUObject(this, &AAstroCharacter::OnStaminaChanged);
		PlayerAttributeSet->OnStaminaRechargeCounterChanged.AddUObject(this, &AAstroCharacter::OnStaminaRechargeCounterChanged);
		PlayerAttributeSet->OnStaminaRechargeCounterCompleted.AddUObject(this, &AAstroCharacter::OnStaminaRechargeCounterCompleted);
		PlayerAttributeSet->OnCurrentMovementSpeedChanged.AddUObject(this, &AAstroCharacter::OnCurrentMovementSpeedChanged);
	}
}

void AAstroCharacter::SwitchState(TSubclassOf<UAstroStateBase> NewState)
{
	if (UClass* NewStateClass = NewState.Get())
	{
		AstroStateMachine->SwitchState(this, NewStateClass);
	}
}

void AAstroCharacter::EnableInteraction()
{
	if (AstroInteractionComponent)
	{
		// Always resets, to ensure that the activation callback is triggered
		constexpr bool bActive = true;
		constexpr bool bReset = true;
		AstroInteractionComponent->SetActive(bActive, bReset);
	}
}

void AAstroCharacter::DisableInteraction()
{
	if (AstroInteractionComponent)
	{
		// Always resets, to ensure that the deactivation callback is triggered
		constexpr bool bActive = false;
		constexpr bool bReset = true;
		AstroInteractionComponent->SetActive(bActive, bReset);
	}
}

void AAstroCharacter::BroadcastFocusStateEntered()
{
	OnAstroFocusStarted.Broadcast();
}

void AAstroCharacter::BroadcastFocusStateEnded()
{
	OnAstroFocusEnded.Broadcast();
}

void AAstroCharacter::DropBallPickup()
{
	if (BallPickup.IsValid())
	{
		BallPickup->DropGrab();
		HandleBallReleased();
	}
}

void AAstroCharacter::HandleBallReleased()
{
	if (BallPickup.IsValid())
	{;
		BallPickup->OnAstroBallDeactivated.RemoveDynamic(this, &AAstroCharacter::OnPickupBallDeactivated);
		BallPickup = nullptr;
		OnBallDropped.Broadcast();
	}
}

void AAstroCharacter::OnMoveCancelAction()
{
	ConsumeMovementInputVector();
}

void AAstroCharacter::OnMoveReleasedAction()
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_Move_Released, {});
}

void AAstroCharacter::OnFocusAction()
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_Focus, {});
}

void AAstroCharacter::OnCancelFocusAction()
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_CancelFocus, {});
}

void AAstroCharacter::OnAstroDashAction(const FAstroDashAimResult& DashAimResult)
{
	// Sends dash data along with its input event
	UAstroDashPayloadData* AstroDashPayloadData = NewObject<UAstroDashPayloadData>();
	AstroDashPayloadData->AstroDashAimResult = DashAimResult;

	FGameplayEventData AstroDashEventData;
	AstroDashEventData.EventTag = AstroGameplayTags::InputTag_AstroDash;
	AstroDashEventData.OptionalObject = AstroDashPayloadData;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_AstroDash, AstroDashEventData);
}

void AAstroCharacter::OnAstroThrowReleasedAction()
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_AstroThrow_Release, {});
}

void AAstroCharacter::OnAstroThrowCanceledAction()
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_AstroThrow_Cancel, {});
}

void AAstroCharacter::OnInteractAction()
{
	if (AstroInteractionComponent)
	{
		AstroInteractionComponent->Interact(this);
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_Interact, {});
}

void AAstroCharacter::OnExitPracticeModeAction()
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_ExitPracticeMode, {});
}

void AAstroCharacter::OnDropBallAction()
{
	DropBallPickup();
}

void AAstroCharacter::OnPickupBallDeactivated(AAstroBall* Ball)
{
	if (Ball && Ball == BallPickup)
	{
		DropBallPickup();
	}
}

void AAstroCharacter::OnMoveAction_Implementation(const FVector& MovementInputAxis)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_Move, {});
}

void AAstroCharacter::OnAstroThrowAction_Implementation(class UAstroThrowAimProvider* ThrowAimProvider)
{
	FGameplayEventData ThrowEventData;
	ThrowEventData.OptionalObject = ThrowAimProvider;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, AstroGameplayTags::InputTag_AstroThrow, ThrowEventData);
}

void AAstroCharacter::OnGameResolved()
{
	SwitchState(WinStateAbilityClass);
}

void AAstroCharacter::OnGameReset()
{
	if (AbilitySystemComponent)
	{
		// Resets the player's stats to default
		const FGameplayEffectSpecHandle AstroPlayerStatsSpec = AbilitySystemComponent->MakeOutgoingSpec(AstroPlayerStatsGE, 0.f, {});
		if (AstroPlayerStatsSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*AstroPlayerStatsSpec.Data.Get());
		}

		const FGameplayTagContainer WinTagContainer = FGameplayTagContainer(AstroGameplayTags::AstroState_Win);
		AbilitySystemComponent->RemoveActiveEffectsWithAppliedTags(WinTagContainer);
	}

	SwitchState(IdleStateAbilityClass);
}

void AAstroCharacter::OnStaminaChanged(float OldStamina, float NewStamina)
{
	OnStaminaChangedDelegate.Broadcast(OldStamina, NewStamina);
}

void AAstroCharacter::OnStaminaRechargeCounterChanged(float OldStaminaRecharge, float NewStaminaRecharge)
{
	if (const bool bJustStartedRecharging = OldStaminaRecharge == 0.f)
	{
		OnStaminaRechargeCounterStartedDelegate.Broadcast();
	}
	OnStaminaRechargeCounterChangedDelegate.Broadcast(OldStaminaRecharge, NewStaminaRecharge);
}

void AAstroCharacter::OnStaminaRechargeCounterCompleted()
{
	OnStaminaRechargeCounterCompletedDelegate.Broadcast();
}

void AAstroCharacter::OnCurrentMovementSpeedChanged(float OldMovementSpeed, float NewMovementSpeed)
{
	if (UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement<UCharacterMovementComponent>())
	{
		CharacterMovementComponent->MaxWalkSpeed = NewMovementSpeed;
	}
}

void AAstroCharacter::OnDamaged(float OldHealth, float NewHealth, const FHitResult& HitResult)
{
	if (AstroCharacterVars::bForceInvulnerable)
	{
		return;
	}

	if (const bool bJustDied = OldHealth != 0.f && NewHealth == 0.f)
	{
		SwitchState(LoseStateAbilityClass);
	}
}

void AAstroCharacter::OnFatigueStateChanged(const FGameplayTag GameplayTag, const int32 GameplayTagCount)
{
	if (GameplayTag.MatchesTagExact(AstroGameplayTags::AstroState_Fatigued) && GameplayTagCount == 1)
	{
		OnAstroCharacterFatigued.Broadcast();
	}
}

void AAstroCharacter::OnRoomEntered(const FGameplayTag GameplayTag, const FAstroRoomGenericMessage& EnterRoomPayload)
{
	if (BallPickup.IsValid())
	{
		DropBallPickup();
	}
}

void AAstroCharacter::OnBallInteract(AAstroBall* NewBallPickup)
{
	// Handles trying to interact with the ball that's currently picked up (useful in case the interact component doesn't update fast enough)
	if (NewBallPickup == BallPickup && NewBallPickup && BallPickup.IsValid())
	{
		return;
	}

	// Drops previously picked up ball
	if (BallPickup.IsValid())
	{
		DropBallPickup();
	}

	// Replaces picked up ball with the new one, and plays the appropriate events
	if (NewBallPickup)
	{
		BallPickup = NewBallPickup;
		BallPickup->StartGrab(GetMesh(), AstroCharacterStatics::BallPickupSocket);
		OnBallPickedUp.Broadcast();
	}

	if (BallPickup.IsValid())
	{
		BallPickup->OnAstroBallDeactivated.AddDynamic(this, &AAstroCharacter::OnPickupBallDeactivated);
	}
}

void AAstroCharacter::OnBallThrowFinished()
{
	HandleBallReleased();
}

void AAstroCharacter::ActivateBulletTimeAbility()
{
	if (BulletTimeAbilityHandle.IsValid())
	{
		if (PlayerAttributeSet)
		{
			AbilitySystemComponent->TryActivateAbility(BulletTimeAbilityHandle);
		}
	}
}

void AAstroCharacter::CancelBulletTimeAbility()
{
	if (BulletTimeAbilityHandle.IsValid())
	{
		AbilitySystemComponent->CancelAbilityHandle(BulletTimeAbilityHandle);
	}
}

float AAstroCharacter::GetCurrentStamina() const
{
	return PlayerAttributeSet ? PlayerAttributeSet->GetStamina() : 0.f;
}

float AAstroCharacter::GetMaxStamina() const
{
	return PlayerAttributeSet ? PlayerAttributeSet->GetMaxStamina() : 1.f;
}

float AAstroCharacter::GetStaminaLimit() const
{
	return PlayerAttributeSet ? PlayerAttributeSet->GetStaminaLimit() : 1.f;
}

float AAstroCharacter::GetCurrentStaminaRecharge() const
{
	return PlayerAttributeSet ? PlayerAttributeSet->GetStaminaRechargeCounter() : 0.f;
}

float AAstroCharacter::GetStaminaRechargeDuration() const
{
	return PlayerAttributeSet ? PlayerAttributeSet->GetStaminaRechargeDuration() : 0.f;
}

float AAstroCharacter::GetAstroDashStaminaCost() const
{
	return AstroDashCost.GetValue();
}

UAstroInteractionComponent* AAstroCharacter::GetInteractionComponent() const
{
	return AstroInteractionComponent;
}
