/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroMissionComponent_InteractWithItem.h"
#include "AstroCharacter.h"
#include "AstroGameState.h"
#include "AstroInteractionComponent.h"
#include "AstroInteractableInterface.h"
#include "AstroUtilitiesBlueprintLibrary.h"
#include "ControlFlowManager.h"
#include "Kismet/GameplayStatics.h"


UAstroMissionComponent_InteractWithItem::UAstroMissionComponent_InteractWithItem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroMissionComponent_InteractWithItem::BeginPlay()
{
	Super::BeginPlay();

	ensure(InteractableActorClass.Get() && InteractableActorClass->ImplementsInterface(UAstroInteractableInterface::StaticClass()));
}

void UAstroMissionComponent_InteractWithItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CachedInteractionComponent.IsValid())
	{
		CachedInteractionComponent->OnInteract.RemoveAll(this);
	}
}

TSharedPtr<FControlFlow> UAstroMissionComponent_InteractWithItem::CreateMissionControlFlow()
{
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("InteractMissionFlow"));
	Flow.QueueStep(TEXT("Interact With Item Mission"), this, &UAstroMissionComponent_InteractWithItem::FlowStep_InteractWithObjectMission);
	Flow.QueueStep(TEXT("Resolve Mission"), this, &UAstroMissionComponent_InteractWithItem::FlowStep_ResolveMission);
	return Flow.AsShared();
}

void UAstroMissionComponent_InteractWithItem::FlowStep_InteractWithObjectMission(FControlFlowNodeRef SubFlow)
{
	// Shows indicator on top of the interactable actor
	if (AActor* InteractableActor = UGameplayStatics::GetActorOfClass(this, InteractableActorClass))
	{
		RegisterActorIndicator(InteractableActor);
	}

	// Listens for interaction event
	if (AAstroCharacter* AstroCharacter = UAstroUtilitiesBlueprintLibrary::FindLocalPlayerAstroCharacter(this))
	{
		if (UAstroInteractionComponent* InteractionComponent = AstroCharacter->GetInteractionComponent())
		{
			CachedInteractionComponent = InteractionComponent;
			CachedInteractionComponent->OnInteract.AddUObject(this, &UAstroMissionComponent_InteractWithItem::OnInteract, SubFlow.ToSharedPtr());
		}
	}

	ensure(CachedInteractionComponent.IsValid());
}

void UAstroMissionComponent_InteractWithItem::FlowStep_ResolveMission(FControlFlowNodeRef SubFlow)
{
	if (GameStateOwner.IsValid())
	{
		GameStateOwner->ResolveGame();
	}

	StartMissionDialog(SuccessDialog);
}

void UAstroMissionComponent_InteractWithItem::OnInteract(TScriptInterface<IAstroInteractableInterface> InteractObject, TSharedPtr<FControlFlowNode> SubFlowPtr)
{
	if (!InteractObject.GetObject() || !InteractableActorClass)
	{
		return;
	}

	UClass* InteractObjectClass = InteractObject.GetObject()->GetClass();
	if (!InteractObjectClass || !InteractObjectClass->IsChildOf(InteractableActorClass))
	{
		return;
	}

	if (SubFlowPtr.IsValid())
	{
		SubFlowPtr->ContinueFlow();
	}

	if (CachedInteractionComponent.IsValid())
	{
		CachedInteractionComponent->OnInteract.RemoveAll(this);
	}
}
