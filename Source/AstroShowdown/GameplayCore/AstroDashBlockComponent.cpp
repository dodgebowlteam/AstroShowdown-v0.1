/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroDashBlockComponent.h"
#include "AstroCharacter.h"
#include "AstroController.h"
#include "AstroGameplayTags.h"
#include "AstroRoomData.h"
#include "AstroRoomNavigationTypes.h"
#include "Components/SkeletalMeshComponent.h"

void UAstroDashBlockComponent::BeginPlay()
{
	Super::BeginPlay();

	// Starts listening to room enter messages
	FGameplayMessageListenerParams<FAstroRoomGenericMessage> RoomEnterListenerParams;
	RoomEnterListenerParams.SetMessageReceivedCallback(this, &UAstroDashBlockComponent::OnRoomEntered);
	RoomEnterMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_Room_Enter, RoomEnterListenerParams);
}

void UAstroDashBlockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnlockDash();

	UGameplayMessageSubsystem::Get(this).UnregisterListener(RoomEnterMessageHandle);
}

void UAstroDashBlockComponent::LockDash()
{
	check(CharacterMeshOverride);
	if (bIsDashLocked || !CharacterMeshOverride)
	{
		return;
	}

	if (AAstroController* Controller = GetController<AAstroController>())
	{
		Controller->RegisterInputBlock(AstroGameplayTags::InputTag_AstroDash);
	}

	if (AAstroCharacter* Character = GetPawn<AAstroCharacter>())
	{
		if (USkeletalMeshComponent* CharacterMeshComponent = Character->GetMesh())
		{
			OriginalCharacterMesh = CharacterMeshComponent->GetSkeletalMeshAsset();
			CharacterMeshComponent->SetSkeletalMeshAsset(CharacterMeshOverride);
		}
	}

	bIsDashLocked = true;
}

void UAstroDashBlockComponent::UnlockDash()
{
	if (!bIsDashLocked)
	{
		return;
	}

	if (AAstroController* Controller = GetController<AAstroController>())
	{
		Controller->UnregisterInputBlock(AstroGameplayTags::InputTag_AstroDash);
	}

	if (AAstroCharacter* Character = GetPawn<AAstroCharacter>())
	{
		if (USkeletalMeshComponent* CharacterMeshComponent = Character->GetMesh())
		{
			check(OriginalCharacterMesh);
			CharacterMeshComponent->SetSkeletalMeshAsset(OriginalCharacterMesh);
		}
	}

	bIsDashLocked = false;
}

void UAstroDashBlockComponent::OnRoomEntered(const FGameplayTag GameplayTag, const FAstroRoomGenericMessage& EnterRoomPayload)
{
	if (EnterRoomPayload.Room && EnterRoomPayload.Room->bShouldLockDash)
	{
		LockDash();
	}
	else
	{
		UnlockDash();
	}
}
