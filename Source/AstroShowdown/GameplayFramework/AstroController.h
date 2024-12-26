/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "CommonPlayerController.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "AstroController.generated.h"

class AAstroCharacter;
class UAstroDashAimComponent;
class UAstroThrowAimComponent;
class UAstroTimeDilationSubsystem;

UCLASS()
class ASTROSHOWDOWN_API AAstroController : public ACommonPlayerController
{
	GENERATED_BODY()

#pragma region AActor
public:
	AAstroController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	virtual void UpdateCameraManager(float DeltaSeconds) override;
#pragma endregion


#pragma region AAstroController
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TSoftObjectPtr<class UInputMappingContext>> DefaultInputMappings;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TMap<FGameplayTag, TObjectPtr<class UInputAction>> DefaultInputActions;

	UPROPERTY(EditDefaultsOnly, Transient, Category = "Components")
	UAstroDashAimComponent* AstroDashAimComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Transient, Category = "Components")
	UAstroThrowAimComponent* AstroThrowAimComponent = nullptr;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInputStateChanged, bool /* bIsInputAvailable */)
	TMap<FGameplayTag, FOnInputStateChanged> InputStateEvents;

protected:
	UPROPERTY(Transient)
	AAstroCharacter* CachedAstroCharacter = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAstroTimeDilationSubsystem> CachedTimeDilationSubsystem = nullptr;

	/** WORKAROUND: Prevents self-registering the focus input block multiple times. This should be replaced by an instigator mechanism on the input block code. */
	uint8 bIsInstigatingFocusBlock : 1 = false;

protected:
	struct FAstroInputBinding
	{
		TArray<uint32> Handles;
	};
	TMap<FGameplayTag, FAstroInputBinding> InputBindings;
	FGameplayTagCountContainer BlockedInputTags;

public:
	void ActivateDashAim();
	void DeactivateDashAim();

	void ActivateThrowAim();
	void DeactivateThrowAim();

public:
	void GrantMoveInput();
	void GrantFocusInput();
	void GrantCancelFocusInput();
	void GrantDashInput();
	void GrantThrowInput();
	void GrantInteractInput();
	void GrantExitPracticeModeInput();

	void UnGrantInput(const FGameplayTag& InputTag, bool bRemove = true);

	void RegisterInputBlock(const FGameplayTag& InputTag);
	void UnregisterInputBlock(const FGameplayTag& InputTag);
	bool IsInputAvailable(const FGameplayTag& InputTag) const { return InputBindings.Contains(InputTag) && BlockedInputTags.GetTagCount(InputTag) == 0; }

private:
	template <typename InputHandlerFn>
	void GrantInput(const FGameplayTag& InputGameplayTag, ETriggerEvent TriggerEvent, InputHandlerFn InputFn, bool bUnique = false);
	void UnGrantAllInputs();

public:
	/** Same as APlayerController::GetHitResultUnderCursorForObjects, but returns multiple hits. */
	bool GetHitResultsUnderCursorForObjects(const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, OUT TArray<FHitResult>& HitResults) const;

private:
	/** Same as APlayerController::GetHitResultAtScreenPosition, but returns multiple hits. */
	bool GetHitResultsAtScreenPosition(const FVector2D ScreenPosition, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, OUT TArray<FHitResult>& HitResults) const;

private:
	/** WORKAROUND: Prevents self-registering the focus input block multiple times. This should be replaced by an instigator mechanism on the input block code. */
	void SelfToggleFocusInputBlock(const bool bBlock);

private:
	void BroadcastInputStateChangeEvent(const FGameplayTag& InputTag);

protected:
	UFUNCTION()
	void OnMovePressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnMoveReleased(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnMoveCanceled();

	UFUNCTION()
	void OnFocusPressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnCancelFocusPressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnAstroDashPressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnAstroThrowPressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnAstroThrowReleased(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnAstroThrowCanceled();

	UFUNCTION()
	void OnInteractPressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnExitPracticeModePressed(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnAstroDashAimChanged(float AxisValue);

	UFUNCTION()
	void OnAstroThrowAimChanged(float AxisValue);

protected:
	UFUNCTION()
	void OnInteractComponentEnabled(UActorComponent* Component, bool bReset);
	UFUNCTION()
	void OnInteractComponentDisabled(UActorComponent* Component);

	UFUNCTION()
	void OnBallPickedUp();
	UFUNCTION()
	void OnBallDropped();

protected:
	UFUNCTION()
	void OnMoveInputStateChanged(bool bIsInputAvailable);

	UFUNCTION()
	void OnAstroDashInputStateChanged(bool bIsInputAvailable);

	UFUNCTION()
	void OnAstroThrowInputStateChanged(bool bIsInputAvailable);

protected:
	UFUNCTION()
	void OnAstroGameActivated();
	UFUNCTION()
	void OnAstroGameResolved();

protected:
	void InitializeInputMappings(UInputComponent* InInputComponent);
	TObjectPtr<class UInputAction> FindPlayerInputActionByTag(const FGameplayTag& InInputGameplayTag);
#pragma endregion

};

template<typename InputHandlerFn>
inline void AAstroController::GrantInput(const FGameplayTag& InputGameplayTag, ETriggerEvent TriggerEvent, InputHandlerFn InputFn, bool bUnique/* = false*/)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (ensureMsgf(EnhancedInputComponent, TEXT("::ERROR: Unexpected Input Component class, make sure you've enabled EnhancedInput.")))
	{
		if (bUnique && InputBindings.Contains(InputGameplayTag))
		{
			return;
		}

		const TObjectPtr<UInputAction> InputAction = FindPlayerInputActionByTag(InputGameplayTag);
		const uint32 BindingHandle = EnhancedInputComponent->BindAction(InputAction, TriggerEvent, this, InputFn).GetHandle();
		InputBindings.FindOrAdd(InputGameplayTag).Handles.Add(BindingHandle);
	}

	BroadcastInputStateChangeEvent(InputGameplayTag);
}
