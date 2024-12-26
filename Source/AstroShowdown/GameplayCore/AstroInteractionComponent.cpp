/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroInteractionComponent.h"
#include "AstroCharacter.h"
#include "AstroGameplayTags.h"
#include "AstroIndicatorTypes.h"
#include "AstroInteractableInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"


DECLARE_LOG_CATEGORY_EXTERN(LogAstroInteract, Log, All);
DEFINE_LOG_CATEGORY(LogAstroInteract);

UAstroInteractionComponent::UAstroInteractionComponent(const FObjectInitializer& InObjectInitializer) : Super(InObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAstroInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentDeactivated.AddDynamic(this, &UAstroInteractionComponent::OnComponentDeactivatedEvent);
}

void UAstroInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterInteractionIndicator();
}

void UAstroInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// NOTE: For whatever reason TickInterval doesn't consider time dilation, so we'll have to do it manually
	InteractionIntervalCounter += DeltaTime;
	if (const bool bReachedInteractionInterval = InteractionIntervalCounter >= InteractionInterval)
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
		FindInteractionFocus();
		InteractionIntervalCounter = 0.f;
	}
}

namespace AstroInteractionComponentUtils
{
	static AActor* FindClosestInteractableOverlapResult(const TArray<FOverlapResult>& OverlapResults, const FVector& Origin)
	{
		AActor* ClosestInteractableActor = nullptr;
		float ClosestDistanceSqr = TNumericLimits<float>::Max();
		
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AActor* OverlappingActor = OverlapResult.GetActor();
			const UClass* OverlappingActorClass = OverlappingActor ? OverlappingActor->GetClass() : nullptr;
			if (OverlappingActor && OverlappingActorClass && OverlappingActorClass->ImplementsInterface(UAstroInteractableInterface::StaticClass()))
			{
				if (!IAstroInteractableInterface::Execute_IsInteractable(OverlappingActor))
				{
					continue;
				}

				const FVector InteractablePosition = OverlappingActor->GetActorLocation();
				const float DistSqr = FVector::DistSquared(Origin, InteractablePosition);
				if (DistSqr < ClosestDistanceSqr)
				{
					ClosestInteractableActor = OverlappingActor;
					ClosestDistanceSqr = DistSqr;
				}
			}
		}

		return ClosestInteractableActor;
	}
}

void UAstroInteractionComponent::Interact(AAstroCharacter* Instigator)
{
	if (!Instigator)
	{
		return;
	}

	bool bInteractFailed = true;
	if (UObject* InteractFocus = GetCurrentInteractionFocus().GetObject())
	{
		if (IAstroInteractableInterface::Execute_IsInteractable(InteractFocus))
		{
			IAstroInteractableInterface::Execute_Interact(InteractFocus, Instigator);
			bInteractFailed = false;
			OnInteract.Broadcast(InteractFocus);
			UnregisterInteractionIndicator();
		}
	}

	// If interact fails and we're holding a ball, drop it. This is essentially hard-coded event bubbling.
	if (bInteractFailed && Instigator->IsHoldingBall())
	{
		Instigator->OnDropBallAction();
	}

	UE_LOG(LogAstroInteract, Verbose, TEXT("[%hs] Interacted (status: %s)"), __FUNCTION__, (bInteractFailed ? "fail" : "success"));
}

void UAstroInteractionComponent::FindInteractionFocus()
{
	ensureMsgf(InteractionTargetChannel != ECollisionChannel::ECC_MAX, TEXT("You forgot to configure InteractionTargetChannel"));

	UWorld* World = GetWorld();
	if (!World)
	{
		SetInteractionFocus(nullptr);
		return;
	}

	AActor* ComponentOwner = GetOwner();
	if (!ComponentOwner)
	{
		SetInteractionFocus(nullptr);
		return;
	}

	// Finds all overlapping interactables
	TArray<FOverlapResult> OverlapResults;
	{
		const FVector OverlapPosition = ComponentOwner->GetActorLocation();
		const FQuat OverlapRotation = FQuat::Identity;
		const FCollisionShape OverlapShape = FCollisionShape::MakeSphere(InteractionRadius);
		World->OverlapMultiByChannel(OUT OverlapResults, OverlapPosition, OverlapRotation, InteractionTargetChannel, OverlapShape);
	}

	// Finds the closest overlapping interactable, and then sets it as the current InteractionFocus
	{
		const FVector InteractionOrigin = ComponentOwner->GetActorLocation();
		TScriptInterface<IAstroInteractableInterface> NewInteractionFocus = AstroInteractionComponentUtils::FindClosestInteractableOverlapResult(OverlapResults, InteractionOrigin);
		SetInteractionFocus(NewInteractionFocus);
	}
}

void UAstroInteractionComponent::SetInteractionFocus(TScriptInterface<IAstroInteractableInterface> NewInteractionFocus)
{
	if (NewInteractionFocus != CurrentInteractionFocus)
	{
		if (CurrentInteractionFocus.GetObject())
		{
			UnregisterInteractionIndicator();
			IAstroInteractableInterface::Execute_OnExitInteractionRange(CurrentInteractionFocus.GetObject());
		}

		CurrentInteractionFocus = NewInteractionFocus.GetObject() ? NewInteractionFocus : nullptr;

		if (NewInteractionFocus.GetObject())
		{
			RegisterInteractionIndicator();
			IAstroInteractableInterface::Execute_OnEnterInteractionRange(CurrentInteractionFocus.GetObject());
		}
	}
}

void UAstroInteractionComponent::RegisterInteractionIndicator()
{
	if (CurrentInteractionFocus.GetObject() && !IndicatorWidgetClass.GetAssetName().IsEmpty())
	{
		FAstroIndicatorRegisterRequestMessage RegisterRequestMessage;
		RegisterRequestMessage.IndicatorSettings.Owner = CastChecked<AActor>(CurrentInteractionFocus.GetObject());
		RegisterRequestMessage.IndicatorSettings.IndicatorWidgetClassOverride = IndicatorWidgetClass;
		RegisterRequestMessage.IndicatorSettings.bOffscreenActorOnly = false;
		RegisterRequestMessage.IndicatorSettings.SocketName = "IndicatorSocket";
		RegisterRequestMessage.IndicatorSettings.OwnerPositionOffset = FVector::UpVector * 100.f;		// WORKAROUND: We need this for AstroBalls, as they can't use sockets because they rotate
		RegisterRequestMessage.IndicatorSettings.bOptionalSocketName = true;							// Suppresses errors if the indicator socket is not found
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_Indicator_Register, RegisterRequestMessage);
	}
}

void UAstroInteractionComponent::UnregisterInteractionIndicator()
{
	if (CurrentInteractionFocus.GetObject() && !IndicatorWidgetClass.GetAssetName().IsEmpty())
	{
		FAstroIndicatorUnregisterRequestMessage UnregisterRequestMessage;
		UnregisterRequestMessage.Owner = CastChecked<AActor>(CurrentInteractionFocus.GetObject());
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_Indicator_Unregister, UnregisterRequestMessage);
	}
}

void UAstroInteractionComponent::OnComponentDeactivatedEvent(UActorComponent* Component)
{
	SetInteractionFocus(nullptr);
}
