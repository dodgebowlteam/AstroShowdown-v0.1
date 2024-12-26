/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroLooseWallProp.h"
#include "AnimToTextureDataAsset.h"
#include "AnimToTextureInstancePlaybackHelpers.h"
#include "AstroTimeDilationSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AAstroLooseWallProp::AAstroLooseWallProp(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass("PropStaticMeshComponent", UInstancedStaticMeshComponent::StaticClass()))	// Spawns an ISM instead of a regular StaticMesh
{
	PrimaryActorTick.bCanEverTick = false;

	bShouldAutoDestroyOnDeath = false;
}

void AAstroLooseWallProp::OnConstruction(const FTransform& Transform)
{
	CachedInstancedStaticMeshComponent = CastChecked<UInstancedStaticMeshComponent>(PropStaticMeshComponent);

	if (CachedInstancedStaticMeshComponent && AnimToTextureData)
	{
		CachedInstancedStaticMeshComponent->SetStaticMesh(AnimToTextureData->GetStaticMesh());
	}
}

void AAstroLooseWallProp::BeginPlay()
{
	Super::BeginPlay();

	PlayIdleAnimation();
}

void AAstroLooseWallProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	StopDeathAnimation();
}

void AAstroLooseWallProp::OnPropDied_Implementation()
{
	Super::OnPropDied_Implementation();

	PlayDeathAnimation();
}

void AAstroLooseWallProp::PlayIdleAnimation()
{
	// Gets the autoplay data for the idle animation
	FAnimToTextureAutoPlayData IdleAnimationFrameData;
	const int32 IdleAnimationIndex = GetAnimationSequenceIndex(false /*bIsHitAnimation*/);
	const bool bSuccess = UAnimToTextureInstancePlaybackLibrary::GetAutoPlayDataFromDataAsset(AnimToTextureData, IdleAnimationIndex, OUT IdleAnimationFrameData);
	if (!bSuccess)
	{
		return;
	}

	// Applies the frame data to all instances in the loose wall ISM
	// NOTE: Animates using VAT, with the ML_BoneAnimation file. This only works because of InstancedStaticMesh > Details > Instances > Advanced > Custom Data Floats.
	const int32 InstanceCount = CachedInstancedStaticMeshComponent->GetInstanceCount();
	const float AnimationOffset = FMath::FRandRange(0.f, 1.f);
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; InstanceIndex++)
	{
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 0 /*CustomDataIndex*/, IdleAnimationFrameData.TimeOffset + AnimationOffset);
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 1 /*CustomDataIndex*/, IdleAnimationFrameData.PlayRate);
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 2 /*CustomDataIndex*/, IdleAnimationFrameData.StartFrame);
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 3 /*CustomDataIndex*/, IdleAnimationFrameData.EndFrame);
	}
}

void AAstroLooseWallProp::PlayDeathAnimation()
{
	// Updates the animation start timestamp, if it's not been initialized yet
	const UWorld* World = GetWorld();
	if (DeathAnimationStartTimestamp <= 0.f)
	{
		DeathAnimationStartTimestamp = World ? World->GetTimeSeconds() : 0.f;
	}

	// Self-destroys if the animation has ended OR there's any invalid object
	const float AnimationElapsedTime = GetDeathAnimationElapsedTime();
	const float AnimationLength = GetDeathAnimationDuration();
	if (AnimationElapsedTime >= AnimationLength || !CachedInstancedStaticMeshComponent || !World)
	{
		Destroy();
		return;
	}

	// Gets the data for the current animation frame
	const int32 DeathAnimationIndex = GetAnimationSequenceIndex(true /*bIsHitAnimation*/);
	FAnimToTextureFrameData DeathAnimationFrameData;
	const bool bSuccess = UAnimToTextureInstancePlaybackLibrary::GetFrameDataFromDataAsset(AnimToTextureData, DeathAnimationIndex, AnimationElapsedTime, OUT DeathAnimationFrameData);
	if (!bSuccess)
	{
		Destroy();
		return;
	}

	// Applies the frame data to all instances in the loose wall ISM
	// NOTE (1): Animates using VAT, with the ML_BoneAnimation file. This only works because of InstancedStaticMesh > Details > Instances > Advanced > Custom Data Floats.
	// NOTE (2): AutoPlay is enabled for this VAT, but we're want to play this frame-by-frame, which is not allowed. We're working around this limitation by setting TimeOffset
	// and PlayRate to 0, so we can avoid AutoPlay and display only the frame that we want to.
	const int32 InstanceCount = CachedInstancedStaticMeshComponent->GetInstanceCount();
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount; InstanceIndex++)
	{
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 0 /*CustomDataIndex*/, 0.f /*VAT.TimeOffset*/);
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 1 /*CustomDataIndex*/, 0.f /*VAT.PlayRate*/);
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 2 /*CustomDataIndex*/, DeathAnimationFrameData.Frame /*VAT.StartFrame*/);
		CachedInstancedStaticMeshComponent->SetCustomDataValue(InstanceIndex, 3 /*CustomDataIndex*/, DeathAnimationFrameData.Frame /*VAT.EndFrame*/);
	}

	// Schedules the next frame
	const float FrameRate = 1.0f / AnimToTextureData->SampleRate;
	World->GetTimerManager().SetTimer(DeathAnimationTimerHandle, this, &AAstroLooseWallProp::PlayDeathAnimation, FrameRate);
}

void AAstroLooseWallProp::StopDeathAnimation()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathAnimationTimerHandle);
	}
}

float AAstroLooseWallProp::GetDeathAnimationDuration() const
{
	if (!AnimToTextureData)
	{
		return 0.f;
	}

	const TArray<FAnimToTextureAnimSequenceInfo>& AnimSequences = AnimToTextureData->AnimSequences;
	const int32 DeathAnimationIndex = GetAnimationSequenceIndex(true /*bIsHitAnimation*/);
	if (AnimSequences.IsValidIndex(DeathAnimationIndex) && AnimSequences[DeathAnimationIndex].AnimSequence)
	{
		return AnimSequences[DeathAnimationIndex].AnimSequence->GetPlayLength();
	}

	return 0.f;
}

float AAstroLooseWallProp::GetDeathAnimationElapsedTime() const
{
	const UWorld* World = GetWorld();
	const float CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.f;
	return (CurrentTimeSeconds - DeathAnimationStartTimestamp);
}

int32 AAstroLooseWallProp::GetAnimationSequenceIndex(const bool bIsHitAnimation) const
{
	const int32 HitMultiplier = bIsHitAnimation ? 1 : 0;
	return LooseWallHeight + HitMultiplier * 6;
}
