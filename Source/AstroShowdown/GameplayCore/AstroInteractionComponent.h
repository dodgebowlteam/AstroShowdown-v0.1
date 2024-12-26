/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/ActorComponent.h"
#include "AstroInteractionComponent.generated.h"

class AAstroCharacter;
class IAstroInteractableInterface;
class UAstroIndicatorWidget;

UCLASS()
class UAstroInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

#pragma region UActorComponent
public:
	UAstroInteractionComponent(const FObjectInitializer& InObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#pragma endregion


public:
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float InteractionRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	float InteractionInterval = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	TEnumAsByte<ECollisionChannel> InteractionTargetChannel = ECollisionChannel::ECC_MAX;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|UI")
	TSoftClassPtr<UAstroIndicatorWidget> IndicatorWidgetClass = nullptr;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInteract, TScriptInterface<IAstroInteractableInterface>);
	FOnInteract OnInteract;

private:
	float InteractionIntervalCounter = 0.f;

private:
	UPROPERTY(Transient)
	TScriptInterface<IAstroInteractableInterface> CurrentInteractionFocus = nullptr;

public:
	void Interact(AAstroCharacter* Instigator);

private:
	void FindInteractionFocus();
	void SetInteractionFocus(TScriptInterface<IAstroInteractableInterface> NewInteractionFocus);

	void RegisterInteractionIndicator();
	void UnregisterInteractionIndicator();

protected:
	UFUNCTION()
	void OnComponentDeactivatedEvent(UActorComponent* Component);

public:
	TScriptInterface<IAstroInteractableInterface> GetCurrentInteractionFocus() const { return CurrentInteractionFocus; }

};
