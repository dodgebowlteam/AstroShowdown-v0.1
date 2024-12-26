/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/


#include "AstroController.h"
#include "AbilitySystemGlobals.h"
#include "AstroCharacter.h"
#include "AstroDashAimComponent.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroThrowAimComponent.h"
#include "AstroInteractionComponent.h"
#include "AstroTimeDilationSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/HUD.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "SubsystemUtils.h"
#include "UserSettings/EnhancedInputUserSettings.h"

namespace AstroControllerStatics
{
	static const FName AstroDashAimAxisName = "AstroDashAim";
	static const FName AstroThrowAimAxisName = "AstroThrowAim";
}

AAstroController::AAstroController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AstroDashAimComponent = CreateDefaultSubobject<UAstroDashAimComponent>("AstroDashAimComponent");
	AstroThrowAimComponent = CreateDefaultSubobject<UAstroThrowAimComponent>("AstroThrowAimComponent");
}

void AAstroController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// AstroController should ignore time dilation, effectively making it 'immune' to bullet time.
	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		CachedTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this);
		CachedTimeDilationSubsystem->RegisterIgnoreTimeDilation(this);
	}

	// Listens for ball pickup and interaction component events, and changes inputs accordingly
	if (CachedAstroCharacter = Cast<AAstroCharacter>(InPawn); CachedAstroCharacter != nullptr)
	{
		CachedAstroCharacter->OnBallPickedUp.AddUObject(this, &AAstroController::OnBallPickedUp);
		CachedAstroCharacter->OnBallDropped.AddUObject(this, &AAstroController::OnBallDropped);

		if (UAstroInteractionComponent* AstroInteractionComponent = CachedAstroCharacter->GetInteractionComponent())
		{
			AstroInteractionComponent->OnComponentActivated.AddDynamic(this, &AAstroController::OnInteractComponentEnabled);
			AstroInteractionComponent->OnComponentDeactivated.AddDynamic(this, &AAstroController::OnInteractComponentDisabled);
			if (AstroInteractionComponent->IsActive())
			{
				constexpr bool bReset = true;
				OnInteractComponentEnabled(AstroInteractionComponent, bReset);
			}
		}
	}

	// Listens for game state changes, and blocks/unblocks focus input accordingly
	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		SelfToggleFocusInputBlock(true);
		AstroGameState->OnAstroGameActivated.AddDynamic(this, &AAstroController::OnAstroGameActivated);
		AstroGameState->OnAstroGameResolved.AddDynamic(this, &AAstroController::OnAstroGameResolved);
	}

	// Listens for dash input state changes
	InputStateEvents.FindOrAdd(AstroGameplayTags::InputTag_Move).AddUObject(this, &AAstroController::OnMoveInputStateChanged);
	InputStateEvents.FindOrAdd(AstroGameplayTags::InputTag_AstroDash).AddUObject(this, &AAstroController::OnAstroDashInputStateChanged);
	InputStateEvents.FindOrAdd(AstroGameplayTags::InputTag_AstroThrow).AddUObject(this, &AAstroController::OnAstroThrowInputStateChanged);

	// Initializes some enhanced input internal systems
	InitializeInputMappings(InPawn->InputComponent);
}

void AAstroController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AAstroController::UpdateCameraManager(float DeltaSeconds)
{
	// Uses RealTimeDeltaSeconds instead of regular DeltaSeconds.
	// 
	// NOTE: UpdateCameraManager is not called during Tick, so its DeltaSeconds is not adjusted by the actor's time dilation, 
	// even though we're setting AstroController's CustomTimeDilation to ignore bullet time.
	// 
	// As a consequence, some camera animations (e.g., camera shake) would fail if we didn't correct it using RealTimeDeltaSeconds.
	DeltaSeconds = CachedTimeDilationSubsystem.IsValid() ? CachedTimeDilationSubsystem->GetRealTimeDeltaSeconds() : UAstroTimeDilationSubsystem::GetUndilatedTime(this, DeltaSeconds);
	Super::UpdateCameraManager(DeltaSeconds);
}

void AAstroController::ActivateDashAim()
{
	if (AstroDashAimComponent)
	{
		AstroDashAimComponent->ActivateAim();

		// NOTE: We're using legacy input here because as of UE 5.3, the 'Mouse XY 2D-Axis' input doesn't work well
		// with EnhancedInput. It should work properly if bShowMouseCursor is set to false on the APlayerController,
		// but we don't want to hide the cursor while aiming.
		if (InputComponent)
		{
			InputComponent->BindAxis(AstroControllerStatics::AstroDashAimAxisName, this, &AAstroController::OnAstroDashAimChanged);
		}
	}
}

void AAstroController::DeactivateDashAim()
{
	if (AstroDashAimComponent)
	{
		AstroDashAimComponent->DeactivateAimPreview();

		if (InputComponent)
		{
			InputComponent->RemoveAxisBinding(AstroControllerStatics::AstroDashAimAxisName);
		}
	}
}

void AAstroController::ActivateThrowAim()
{
	if (AstroThrowAimComponent)
	{
		AstroThrowAimComponent->ActivateAim();

		// NOTE: We're using legacy input here because as of UE 5.3, the 'Mouse XY 2D-Axis' input doesn't work well
		// with EnhancedInput. It should work properly if bShowMouseCursor is set to false on the APlayerController,
		// but we don't want to hide the cursor while aiming.
		if (InputComponent)
		{
			InputComponent->BindAxis(AstroControllerStatics::AstroThrowAimAxisName, this, &AAstroController::OnAstroThrowAimChanged);
		}
	}
}

void AAstroController::DeactivateThrowAim()
{
	if (AstroThrowAimComponent)
	{
		AstroThrowAimComponent->DeactivateAim();

		if (InputComponent)
		{
			InputComponent->RemoveAxisBinding(AstroControllerStatics::AstroThrowAimAxisName);
		}
	}
}

void AAstroController::GrantMoveInput()
{
	GrantInput(AstroGameplayTags::InputTag_Move, ETriggerEvent::Triggered, &AAstroController::OnMovePressed);
	GrantInput(AstroGameplayTags::InputTag_Move, ETriggerEvent::Completed, &AAstroController::OnMoveReleased);

	// Calls BindActionValue on this input, so that we can use UEnhancedInputComponent::GetBoundActionValue to fetch its value
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (ensureMsgf(EnhancedInputComponent, TEXT("::ERROR: Unexpected Input Component class, make sure you've enabled EnhancedInput.")))
	{
		const TObjectPtr<UInputAction> InputAction = FindPlayerInputActionByTag(AstroGameplayTags::InputTag_Move);
		EnhancedInputComponent->BindActionValue(InputAction);
	}
}

void AAstroController::GrantFocusInput()
{
	GrantInput(AstroGameplayTags::InputTag_Focus, ETriggerEvent::Started, &AAstroController::OnFocusPressed);
}

void AAstroController::GrantCancelFocusInput()
{
	GrantInput(AstroGameplayTags::InputTag_CancelFocus, ETriggerEvent::Started, &AAstroController::OnCancelFocusPressed);
}

void AAstroController::GrantDashInput()
{
	GrantInput(AstroGameplayTags::InputTag_AstroDash, ETriggerEvent::Started, &AAstroController::OnAstroDashPressed);
}

void AAstroController::GrantThrowInput()
{
	GrantInput(AstroGameplayTags::InputTag_AstroThrow, ETriggerEvent::Started, &AAstroController::OnAstroThrowPressed);
	GrantInput(AstroGameplayTags::InputTag_AstroThrow, ETriggerEvent::Completed, &AAstroController::OnAstroThrowReleased);
}

void AAstroController::GrantInteractInput()
{
	constexpr bool bUnique = true;
	GrantInput(AstroGameplayTags::InputTag_Interact, ETriggerEvent::Started, &AAstroController::OnInteractPressed, bUnique);
}

void AAstroController::GrantExitPracticeModeInput()
{
	GrantInput(AstroGameplayTags::InputTag_ExitPracticeMode, ETriggerEvent::Started, &AAstroController::OnExitPracticeModePressed);
}

void AAstroController::UnGrantInput(const FGameplayTag& InputTag, bool bRemove/* = true*/)
{
	if (InputBindings.Contains(InputTag))
	{
		UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
		if (ensureMsgf(EnhancedInputComponent, TEXT("::ERROR: Unexpected Input Component class, make sure you've enabled EnhancedInput.")))
		{
			// Caches the action pressed value for the input that is getting removed.
			// NOTE: UEnhancedInputComponent::GetBoundActionValue only works if you BindActionValue the input
			const TObjectPtr<UInputAction> InputAction = FindPlayerInputActionByTag(InputTag);
			const FInputActionValue InputActionValue = EnhancedInputComponent->GetBoundActionValue(InputAction);
			const bool bWasActionPressed = InputActionValue.IsNonZero();

			// Removes all bindings for this input
			const TArray<uint32>& BindingHandles = InputBindings[InputTag].Handles;
			for (uint32 BindingHandle : BindingHandles)
			{
				EnhancedInputComponent->RemoveBindingByHandle(BindingHandle);
			}

			if (bRemove)
			{
				InputBindings.Remove(InputTag);
			}
		}
	}
	BroadcastInputStateChangeEvent(InputTag);
}

void AAstroController::UnGrantAllInputs()
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (ensureMsgf(EnhancedInputComponent, TEXT("::ERROR: Unexpected Input Component class, make sure you've enabled EnhancedInput.")))
	{
		for (const TPair<FGameplayTag, FAstroInputBinding>& BindingHandlePair : InputBindings)
		{
			// We don't want to remove while looping, to avoid modifying the list during iteration
			constexpr bool bRemove = false;
			UnGrantInput(BindingHandlePair.Key, bRemove);
		}
		InputBindings.Empty();
	}
}

bool AAstroController::GetHitResultsUnderCursorForObjects(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, OUT TArray<FHitResult>& HitResults) const
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	bool bHit = false;
	if (LocalPlayer && LocalPlayer->ViewportClient)
	{
		FVector2D MousePosition;
		if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
		{
			bHit = GetHitResultsAtScreenPosition(MousePosition, ObjectTypes, bTraceComplex, OUT HitResults);
		}
	}

	if (!bHit)	// If there was no hit we reset the results. This is redundant but may help users.
	{
		HitResults.Empty();
	}

	return bHit;
}

bool AAstroController::GetHitResultsAtScreenPosition(const FVector2D ScreenPosition, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes, bool bTraceComplex, OUT TArray<FHitResult>& HitResults) const
{
	// Early out if we clicked on a HUD hitbox
	if (GetHUD() != NULL && GetHUD()->GetHitBoxAtCoordinates(ScreenPosition, true))
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (UGameplayStatics::DeprojectScreenToWorld(this, ScreenPosition, WorldOrigin, WorldDirection))
	{
		FCollisionObjectQueryParams const ObjParam(ObjectTypes);
		return GetWorld()->LineTraceMultiByObjectType(HitResults, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, ObjParam, FCollisionQueryParams(SCENE_QUERY_STAT(ClickableTrace), bTraceComplex));
	}

	return false;
}

void AAstroController::SelfToggleFocusInputBlock(const bool bBlock)
{
	// Early-returns if trying to block while already blocking, or trying to unblock while not blocking
	if (bIsInstigatingFocusBlock == bBlock)
	{
		return;
	}

	if (bBlock)
	{
		RegisterInputBlock(AstroGameplayTags::InputTag_Focus);
	}
	else
	{
		UnregisterInputBlock(AstroGameplayTags::InputTag_Focus);
	}

	bIsInstigatingFocusBlock = bBlock;
}

void AAstroController::RegisterInputBlock(const FGameplayTag& InputTag)
{
	ensure(InputTag.MatchesTag(AstroGameplayTags::InputTag));
	BlockedInputTags.UpdateTagCount(InputTag, 1);
	BroadcastInputStateChangeEvent(InputTag);
}

void AAstroController::UnregisterInputBlock(const FGameplayTag& InputTag)
{
	ensure(InputTag.MatchesTag(AstroGameplayTags::InputTag));
	BlockedInputTags.UpdateTagCount(InputTag, -1);
	BroadcastInputStateChangeEvent(InputTag);
}

void AAstroController::BroadcastInputStateChangeEvent(const FGameplayTag& InputTag)
{
	if (InputStateEvents.Contains(InputTag))
	{
		const bool bIsInputAvailable = IsInputAvailable(InputTag);
		InputStateEvents[InputTag].Broadcast(bIsInputAvailable);
	}
}

void AAstroController::OnMovePressed(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter && IsInputAvailable(AstroGameplayTags::InputTag_Move))
	{
		const FVector InputVector = { InputActionValue[0], InputActionValue[1], 0.f };
		CachedAstroCharacter->OnMoveAction(InputVector);
	}
}

void AAstroController::OnMoveReleased(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter)
	{
		CachedAstroCharacter->OnMoveReleasedAction();
	}
}

void AAstroController::OnMoveCanceled()
{
	if (CachedAstroCharacter)
	{
		CachedAstroCharacter->OnMoveCancelAction();
	}
}

void AAstroController::OnFocusPressed(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter && IsInputAvailable(AstroGameplayTags::InputTag_Focus))
	{
		CachedAstroCharacter->OnFocusAction();
	}
}

void AAstroController::OnCancelFocusPressed(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter && IsInputAvailable(AstroGameplayTags::InputTag_CancelFocus))
	{
		CachedAstroCharacter->OnCancelFocusAction();
	}
}

void AAstroController::OnAstroDashPressed(const FInputActionValue& InputActionValue)
{
	if (!CachedAstroCharacter || !IsInputAvailable(AstroGameplayTags::InputTag_AstroDash))
	{
		return;
	}

	if (!AstroDashAimComponent)
	{
		return;
	}

	const TOptional<FAstroDashAimResult>& CurrentDashAimResult = AstroDashAimComponent->GetAstroDashAimResult();
	if (CurrentDashAimResult.IsSet())
	{
		CachedAstroCharacter->OnAstroDashAction(CurrentDashAimResult.GetValue());
	}
}

void AAstroController::OnAstroThrowPressed(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter && IsInputAvailable(AstroGameplayTags::InputTag_AstroThrow))
	{
		if (AstroThrowAimComponent)
		{
			CachedAstroCharacter->OnAstroThrowAction(AstroThrowAimComponent->GetThrowAimProvider());
		}
	}
}

void AAstroController::OnAstroThrowReleased(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter)
	{
		if (AstroThrowAimComponent)
		{
			CachedAstroCharacter->OnAstroThrowReleasedAction();
		}
	}
}

void AAstroController::OnAstroThrowCanceled()
{
	if (CachedAstroCharacter)
	{
		CachedAstroCharacter->OnAstroThrowCanceledAction();
	}
}

void AAstroController::OnInteractPressed(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter && IsInputAvailable(AstroGameplayTags::InputTag_Interact))
	{
		CachedAstroCharacter->OnInteractAction();
	}
}

void AAstroController::OnExitPracticeModePressed(const FInputActionValue& InputActionValue)
{
	if (CachedAstroCharacter && IsInputAvailable(AstroGameplayTags::InputTag_ExitPracticeMode))
	{
		CachedAstroCharacter->OnExitPracticeModeAction();
	}
}

void AAstroController::OnAstroDashAimChanged(float AxisValue)
{
	// Mouse movement may get picked up while the game is paused. This check prevents it from doing anything.
	if (IsPaused())
	{
		return;
	}

	AstroDashAimComponent->UpdateAim();
}

void AAstroController::OnAstroThrowAimChanged(float AxisValue)
{
	// Mouse movement may get picked up while the game is paused. This check prevents it from doing anything.
	if (IsPaused())
	{
		return;
	}

	if (AstroThrowAimComponent)
	{
		AstroThrowAimComponent->UpdateAim();
	}
}

void AAstroController::OnInteractComponentEnabled(UActorComponent* Component, bool bReset)
{
	GrantInteractInput();
}

void AAstroController::OnInteractComponentDisabled(UActorComponent* Component)
{
	UnGrantInput(AstroGameplayTags::InputTag_Interact);
}

void AAstroController::OnBallPickedUp()
{
	// NOTE: Ideally we should always block/unblock dashing when throwing is assigned/unassigned, so this could go into Grant/UngrantThrow as well
	GrantThrowInput();
	RegisterInputBlock(AstroGameplayTags::InputTag_AstroDash);
}

void AAstroController::OnBallDropped()
{
	// NOTE: Ideally we should always block/unblock dashing when throwing is assigned/unassigned, so this could go into Grant/UngrantThrow as well
	UnGrantInput(AstroGameplayTags::InputTag_AstroThrow);
	UnregisterInputBlock(AstroGameplayTags::InputTag_AstroDash);
}

void AAstroController::OnMoveInputStateChanged(bool bIsInputAvailable)
{
	// Forces MoveRelease input if the move input was blocked while the player was pressing it.
	if (const bool bMoveBlocked = !bIsInputAvailable)
	{
		const UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
		if (ensureMsgf(EnhancedInputComponent, TEXT("::ERROR: Unexpected Input Component class, make sure you've enabled EnhancedInput.")))
		{
			const TObjectPtr<UInputAction> MoveInputAction = FindPlayerInputActionByTag(AstroGameplayTags::InputTag_Move);
			const FInputActionValue CurrentMoveValue = EnhancedInputComponent->GetBoundActionValue(MoveInputAction);
			if (CurrentMoveValue.GetMagnitudeSq() > 0.f)
			{
				OnMoveReleased({});
				OnMoveCanceled();
			}
		}
	}
}

void AAstroController::OnAstroDashInputStateChanged(bool bIsInputAvailable)
{
	if (bIsInputAvailable)
	{
		ActivateDashAim();
	}
	else
	{
		DeactivateDashAim();
	}
}

void AAstroController::OnAstroThrowInputStateChanged(bool bIsInputAvailable)
{
	if (bIsInputAvailable)
	{
		ActivateThrowAim();
	}
	else
	{
		DeactivateThrowAim();
	}

	// Cancels input if the throw input is blocked
	if (const bool bMoveBlocked = !bIsInputAvailable)
	{
		const UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
		if (ensureMsgf(EnhancedInputComponent, TEXT("::ERROR: Unexpected Input Component class, make sure you've enabled EnhancedInput.")))
		{
			OnAstroThrowCanceled();
		}
	}
}

void AAstroController::OnAstroGameActivated()
{
	SelfToggleFocusInputBlock(false);
}

void AAstroController::OnAstroGameResolved()
{
	SelfToggleFocusInputBlock(true);
}

void AAstroController::InitializeInputMappings(UInputComponent* InInputComponent)
{
	check(InInputComponent);

	const APawn* PossessedPawn = GetPawn<APawn>();
	if (!PossessedPawn)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	check(LocalPlayer);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	for (const TSoftObjectPtr<UInputMappingContext>& InputMappingContextRef : DefaultInputMappings)
	{
		if (UInputMappingContext* InputMappingContext = InputMappingContextRef.LoadSynchronous())
		{
			// Registers the mappings with UserSettings, which creates an initial mapping.
			// I'm guessing this is useful in case we want to support keybindings, as having these initial mappings would allow us
			// to restore the default values whenever we wanted to.
			if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
			{
				Settings->RegisterInputMappingContext(InputMappingContext);
			}

			// Actually adds the input mapping. For now, all inputs will have the same priority.
			FModifyContextOptions Options = {};
			Options.bIgnoreAllPressedKeysUntilRelease = false;

			constexpr int32 MappingPriority = 0;
			Subsystem->AddMappingContext(InputMappingContext, MappingPriority, Options);
		}
	}
}

TObjectPtr<class UInputAction> AAstroController::FindPlayerInputActionByTag(const FGameplayTag& InInputGameplayTag)
{
	for (const TPair<FGameplayTag, TObjectPtr<UInputAction>>& InputActionPair : DefaultInputActions)
	{
		if (InInputGameplayTag == InputActionPair.Key)
		{
			return InputActionPair.Value;
		}
	}

	ensureAlwaysMsgf(false, TEXT("::ERROR: Input action was not found for the given gameplay tag"));
	return nullptr;
}
