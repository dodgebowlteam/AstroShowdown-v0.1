/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's Context Effects System, though we modified it to suit our needs.
*/

#include "AnimNotify_AstroContextEffects.h"
#include "AstroContextEffectsLibrary.h"
#include "AstroContextEffectsInterface.h"
#include "AstroContextEffectsSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_AstroContextEffects)



UAnimNotify_AstroContextEffects::UAnimNotify_AstroContextEffects()
{
}

void UAnimNotify_AstroContextEffects::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR
void UAnimNotify_AstroContextEffects::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FString UAnimNotify_AstroContextEffects::GetNotifyName_Implementation() const
{
	// If the Effect Tag is valid, pass the string name to the notify name
	if (Effect.IsValid())
	{
		return Effect.ToString();
	}

	return Super::GetNotifyName_Implementation();
}

void UAnimNotify_AstroContextEffects::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* OwningActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!OwningActor)
	{
		return;
	}

	// Prepare Trace Data
	bool bHitSuccess = false;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;

	if (TraceProperties.bIgnoreActor)
	{
		QueryParams.AddIgnoredActor(OwningActor);
	}

	QueryParams.bReturnPhysicalMaterial = true;

	if (bPerformTrace)
	{
		// If trace is needed, set up Start Location to Attached
		FVector TraceStart = bAttached ? MeshComp->GetSocketLocation(SocketName) : MeshComp->GetComponentLocation();

		// Make sure World is valid
		if (UWorld* World = OwningActor->GetWorld())
		{
			// Call Line Trace, Pass in relevant properties
			bHitSuccess = World->LineTraceSingleByChannel(HitResult, TraceStart, (TraceStart + TraceProperties.EndTraceLocationOffset),
				TraceProperties.TraceChannel, QueryParams, FCollisionResponseParams::DefaultResponseParam);
		}
	}

	// Prepare Contexts in advance
	FGameplayTagContainer Contexts;

	// Set up Array of Objects that implement the Context Effects Interface
	TArray<UObject*> AstroContextEffectImplementingObjects;

	// Determine if the Owning Actor is one of the Objects that implements the Context Effects Interface
	if (OwningActor->Implements<UAstroContextEffectsInterface>())
	{
		// If so, add it to the Array
		AstroContextEffectImplementingObjects.Add(OwningActor);
	}

	// Cycle through Owning Actor's Components and determine if any of them is a Component implementing the Context Effect Interface
	for (UActorComponent* Component : OwningActor->GetComponents())
	{
		if (Component)
		{
			// If the Component implements the Context Effects Interface, add it to the list
			if (Component->Implements<UAstroContextEffectsInterface>())
			{
				AstroContextEffectImplementingObjects.Add(Component);
			}
		}
	}

	// Cycle through all objects implementing the Context Effect Interface
	for (UObject* AstroContextEffectImplementingObject : AstroContextEffectImplementingObjects)
	{
		if (AstroContextEffectImplementingObject)
		{
			// If the object is still valid, Execute the AnimMotionEffect Event on it, passing in relevant data
			FAstroContextEffectsParameters ContextEffectsParameters;
			ContextEffectsParameters.Bone = bAttached ? SocketName : FName("None");
			ContextEffectsParameters.MotionEffect = Effect;
			ContextEffectsParameters.StaticMeshComponent = MeshComp;
			ContextEffectsParameters.LocationOffset = LocationOffset;
			ContextEffectsParameters.RotationOffset = RotationOffset;
			ContextEffectsParameters.AnimationSequence = Animation;
			ContextEffectsParameters.bHitSuccess = bHitSuccess;
			ContextEffectsParameters.HitResult = HitResult;
			ContextEffectsParameters.Contexts = Contexts;
			ContextEffectsParameters.VFXScale = VFXProperties.Scale;
			ContextEffectsParameters.AudioVolume = AudioProperties.VolumeMultiplier;
			ContextEffectsParameters.AudioPitch = AudioProperties.PitchMultiplier;

			IAstroContextEffectsInterface::Execute_AnimMotionEffect(AstroContextEffectImplementingObject, ContextEffectsParameters);
		}
	}

#if WITH_EDITORONLY_DATA
	NotifyEditor(MeshComp, Animation, EventReference, OUT Contexts);
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITORONLY_DATA
void UAnimNotify_AstroContextEffects::NotifyEditor(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference, OUT FGameplayTagContainer& OutContexts)
{
	AActor* OwningActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!OwningActor)
	{
		return;
	}

	// This is for Anim Editor previewing, it is a deconstruction of the calls made by the Interface and the Subsystem
	if (bPreviewInEditor)
	{
		UWorld* World = OwningActor->GetWorld();

		// Get the world, make sure it's an Editor Preview World
		if (World && World->WorldType == EWorldType::EditorPreview)
		{
			// Add Preview contexts if necessary
			OutContexts.AppendTags(PreviewProperties.PreviewContexts);

			// Convert given Surface Type to Context and Add it to the Contexts for this Preview
			if (PreviewProperties.bPreviewPhysicalSurfaceAsContext)
			{
				TEnumAsByte<EPhysicalSurface> PhysicalSurfaceType = PreviewProperties.PreviewPhysicalSurface;

				if (const UAstroContextEffectsSettings* AstroContextEffectsSettings = GetDefault<UAstroContextEffectsSettings>())
				{
					if (const FGameplayTag* SurfaceContextPtr = AstroContextEffectsSettings->SurfaceTypeToContextMap.Find(PhysicalSurfaceType))
					{
						FGameplayTag SurfaceContext = *SurfaceContextPtr;

						OutContexts.AddTag(SurfaceContext);
					}
				}
			}

			// Libraries are soft referenced, so you will want to try to load them now
			// TODO Async Asset Loading
			if (UObject* EffectsLibrariesObj = PreviewProperties.PreviewContextEffectsLibrary.TryLoad())
			{
				// Check if it is in fact a UAstroContextEffectLibrary type
				if (UAstroContextEffectsLibrary* EffectLibrary = Cast<UAstroContextEffectsLibrary>(EffectsLibrariesObj))
				{
					// Prepare Sounds and Niagara System Arrays
					TArray<USoundBase*> TotalSounds;
					TArray<UNiagaraSystem*> TotalNiagaraSystems;

					// Attempt to load the Effect Library content (will cache in Transient data on the Effect Library Asset)
					EffectLibrary->LoadEffects();

					// If the Effect Library is valid and marked as Loaded, Get Effects from it
					if (EffectLibrary && EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Loaded)
					{
						// Prepare local arrays
						TArray<USoundBase*> Sounds;
						TArray<UNiagaraSystem*> NiagaraSystems;

						// Get the Effects
						EffectLibrary->GetEffects(Effect, OutContexts, Sounds, NiagaraSystems);

						// Append to the accumulating arrays
						TotalSounds.Append(Sounds);
						TotalNiagaraSystems.Append(NiagaraSystems);
					}

					// Cycle through Sounds and call Spawn Sound Attached, passing in relevant data
					for (USoundBase* Sound : TotalSounds)
					{
						UGameplayStatics::SpawnSoundAttached(Sound, MeshComp, (bAttached ? SocketName : FName("None")), LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset,
							false, AudioProperties.VolumeMultiplier, AudioProperties.PitchMultiplier, 0.0f, nullptr, nullptr, true);
					}

					// Cycle through Niagara Systems and call Spawn System Attached, passing in relevant data
					for (UNiagaraSystem* NiagaraSystem : TotalNiagaraSystems)
					{
						UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, MeshComp, (bAttached ? SocketName : FName("None")), LocationOffset,
							RotationOffset, VFXProperties.Scale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, true, true);
					}
				}
			}

		}
	}
}
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
void UAnimNotify_AstroContextEffects::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();
}

void UAnimNotify_AstroContextEffects::SetParameters(FGameplayTag EffectIn, FVector LocationOffsetIn, FRotator RotationOffsetIn,
	FAstroContextEffectAnimNotifyVFXSettings VFXPropertiesIn, FAstroContextEffectAnimNotifyAudioSettings AudioPropertiesIn,
	bool bAttachedIn, FName SocketNameIn, bool bPerformTraceIn, FAstroContextEffectAnimNotifyTraceSettings TracePropertiesIn)
{
	Effect = EffectIn;
	LocationOffset = LocationOffsetIn;
	RotationOffset = RotationOffsetIn;
	VFXProperties.Scale = VFXPropertiesIn.Scale;
	AudioProperties.PitchMultiplier = AudioPropertiesIn.PitchMultiplier;
	AudioProperties.VolumeMultiplier = AudioPropertiesIn.VolumeMultiplier;
	bAttached = bAttachedIn;
	SocketName = SocketNameIn;
	bPerformTrace = bPerformTraceIn;
	TraceProperties.EndTraceLocationOffset = TracePropertiesIn.EndTraceLocationOffset;
	TraceProperties.TraceChannel = TracePropertiesIn.TraceChannel;
	TraceProperties.bIgnoreActor = TracePropertiesIn.bIgnoreActor;

}
#endif

