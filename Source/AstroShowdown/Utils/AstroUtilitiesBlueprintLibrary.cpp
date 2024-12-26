/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroUtilitiesBlueprintLibrary.h"
#include "AstroCharacter.h"
#include "AstroCoreDelegates.h"
#include "AstroCustomDepthStencilConstants.h"
#include "AsyncAction_RealTimeDelay.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

UActorComponent* UAstroUtilitiesBlueprintLibrary::AddInstancedComponent(UObject* InWorldContextObject, AActor* Owner, TSubclassOf<UActorComponent> AddedComponentClass)
{
	if (Owner && AddedComponentClass.Get())
	{
		UActorComponent* MyNewComponent = NewObject<UActorComponent>(Owner, AddedComponentClass.Get());
		Owner->AddInstanceComponent(MyNewComponent);
		Owner->AddOwnedComponent(MyNewComponent);
		MyNewComponent->RegisterComponent();
		return MyNewComponent;
	}

	return nullptr;
}

void UAstroUtilitiesBlueprintLibrary::ClearAllChildrenComponents(UObject* InWorldContextObject, USceneComponent* Owner)
{
	if (Owner)
	{
		TArray<USceneComponent*> OutChildren;
		constexpr bool bIncludeAllDescendants = true;
		Owner->GetChildrenComponents(bIncludeAllDescendants, OutChildren);

		for (USceneComponent* Child : OutChildren)
		{
			Child->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
			Child->DestroyComponent();
		}
	}
}

void UAstroUtilitiesBlueprintLibrary::ClearAllActorChildrenComponents(UObject* InWorldContextObject, AActor* Owner, TSubclassOf<UActorComponent> ComponentClass)
{
	if (!Owner || !ComponentClass.Get())
	{
		return;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(ComponentClass, Components);	
	Components.Append(Owner->GetInstanceComponents());

	for (UActorComponent* Component : Components)
	{
		if (!Component || Component->IsBeingDestroyed())
		{
			continue;
		}

		if (!Component->GetClass()->IsChildOf(ComponentClass.Get()))
		{
			continue;
		}

		Component->DestroyComponent();
	}
}

void UAstroUtilitiesBlueprintLibrary::RestartLevel(UObject* WorldContextObject)
{
	bool bShouldDefer = false;
	FAstroCoreDelegates::OnPreRestartCurrentLevel.Broadcast(OUT bShouldDefer);

	// Reloads the current level
	auto OpenRestartLevel = [WorldContextObject]()
	{
		if (WorldContextObject)
		{
			const FName CurrentLevelName = FName(UGameplayStatics::GetCurrentLevelName(WorldContextObject));
			UGameplayStatics::OpenLevel(WorldContextObject, CurrentLevelName);
		}
	};

	if (bShouldDefer)
	{
		constexpr float DeferredLoadDelay = 0.5f;
		constexpr bool bSuspendInputUntilComplete = false;
		UAsyncAction_RealTimeDelay* RealTimeDelayAction = UAsyncAction_RealTimeDelay::WaitForRealTime(WorldContextObject, DeferredLoadDelay);
		RealTimeDelayAction->OnCompleteDelegate.AddWeakLambda(WorldContextObject, OpenRestartLevel);
		RealTimeDelayAction->Activate();
	}
	else
	{
		OpenRestartLevel();
	}
}

void UAstroUtilitiesBlueprintLibrary::GetPenetrationDepthFromHitResult(UObject* WorldContextObject, const FHitResult& HitResult, OUT float& OutPenetrationDepth)
{
	OutPenetrationDepth = HitResult.PenetrationDepth;
}

int32 UAstroUtilitiesBlueprintLibrary::GetDefaultDepthStencilOutlineValue(UObject* WorldContextObject)
{
	return AstroCustomDepthStencilConstants::DefaultDepthStencilOutlineValue;
}

int32 UAstroUtilitiesBlueprintLibrary::GetInteractableDepthStencilOutlineValue(UObject* WorldContextObject)
{
	return AstroCustomDepthStencilConstants::InteractableDepthStencilOutlineValue;
}

AGameStateBase* UAstroUtilitiesBlueprintLibrary::GetGameStateByClass(UObject* WorldContextObject, TSubclassOf<AGameStateBase> GameStateClass)
{
	UWorld* const World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (AGameStateBase* GameState = World ? World->GetGameState() : nullptr)
	{
		if (GameState->GetClass()->IsChildOf(GameStateClass.Get()))
		{
			return GameState;
		}
	}
	return nullptr;
}

AAstroCharacter* UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject->GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	return Cast<AAstroCharacter>(PlayerPawn);
}
