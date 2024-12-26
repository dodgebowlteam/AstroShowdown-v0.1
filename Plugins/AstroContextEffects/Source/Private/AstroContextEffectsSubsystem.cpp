/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's Context Effects System, though we modified it to suit our needs.
*/

#include "AstroContextEffectsSubsystem.h"

#include "AstroContextEffectsLibrary.h"
#include "AstroContextEffectsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroContextEffectsSubsystem)

class AActor;
class UAudioComponent;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;

void UAstroContextEffectsSubsystem::SpawnContextEffects(AActor* SpawnInstigator, const FAstroContextEffectsParameters& ContextEffectsParameters, OUT TArray<UAudioComponent*>& OutAudios, OUT TArray<UNiagaraComponent*>& OutNiagaraEffects)
{
	// First determine if this Actor has a matching Set of Libraries
	if (TObjectPtr<UAstroContextEffectsSet>* EffectsLibrariesSetPtr = ActiveActorEffectsMap.Find(SpawnInstigator))
	{
		// Validate the pointers from the Map Find
		if (UAstroContextEffectsSet* EffectsLibraries = *EffectsLibrariesSetPtr)
		{
			// Prepare Arrays for Sounds and Niagara Systems
			TArray<USoundBase*> TotalSounds;
			TArray<UNiagaraSystem*> TotalNiagaraSystems;

			// Cycle through Effect Libraries
			for (UAstroContextEffectsLibrary* EffectLibrary : EffectsLibraries->AstroContextEffectsLibraries)
			{
				// Check if the Effect Library is valid and data Loaded
				if (EffectLibrary && EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Loaded)
				{
					// Set up local list of Sounds and Niagara Systems
					TArray<USoundBase*> Sounds;
					TArray<UNiagaraSystem*> NiagaraSystems;

					// Get Sounds and Niagara Systems
					EffectLibrary->GetEffects(ContextEffectsParameters.MotionEffect, ContextEffectsParameters.Contexts, Sounds, NiagaraSystems);

					// Append to accumulating array
					TotalSounds.Append(Sounds);
					TotalNiagaraSystems.Append(NiagaraSystems);
				}
				else if (EffectLibrary && EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Unloaded)
				{
					// Else load effects
					EffectLibrary->LoadEffects();
				}
			}

			// Cycle through found Sounds
			for (USoundBase* Sound : TotalSounds)
			{
				// Spawn Sounds Attached, add Audio Component to List of ACs
				UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(Sound,
					ContextEffectsParameters.StaticMeshComponent,
					ContextEffectsParameters.Bone,
					ContextEffectsParameters.LocationOffset,
					ContextEffectsParameters.RotationOffset,
					EAttachLocation::KeepRelativeOffset,
					false, ContextEffectsParameters.AudioVolume, ContextEffectsParameters.AudioPitch,
					0.0f, nullptr, nullptr, true);

				OutAudios.Add(AudioComponent);
			}

			// Cycle through found Niagara Systems
			for (UNiagaraSystem* NiagaraSystem : TotalNiagaraSystems)
			{
				// Spawn Niagara Systems Attached, add Niagara Component to List of NCs
				constexpr bool bAutoDestroy = true;
				constexpr bool bAutoActivate = true;
				constexpr bool bPreCullCheck = true;
				UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem,
					ContextEffectsParameters.StaticMeshComponent,
					ContextEffectsParameters.Bone,
					ContextEffectsParameters.LocationOffset,
					ContextEffectsParameters.RotationOffset,
					ContextEffectsParameters.VFXScale,
					EAttachLocation::KeepRelativeOffset, bAutoDestroy, ENCPoolMethod::None, bAutoActivate, bPreCullCheck);

				OutNiagaraEffects.Add(NiagaraComponent);
			}
		}
	}
}

bool UAstroContextEffectsSubsystem::GetContextFromSurfaceType(
	TEnumAsByte<EPhysicalSurface> PhysicalSurface, FGameplayTag& Context)
{
	// Get Project Settings
	if (const UAstroContextEffectsSettings* ProjectSettings = GetDefault<UAstroContextEffectsSettings>())
	{
		// Find which Gameplay Tag the Surface Type is mapped to
		if (const FGameplayTag* GameplayTagPtr = ProjectSettings->SurfaceTypeToContextMap.Find(PhysicalSurface))
		{
			Context = *GameplayTagPtr;
		}
	}

	// Return true if Context is Valid
	return Context.IsValid();
}

void UAstroContextEffectsSubsystem::LoadAndAddContextEffectsLibraries(AActor* OwningActor,
	TSet<TSoftObjectPtr<UAstroContextEffectsLibrary>> ContextEffectsLibraries)
{
	// Early out if Owning Actor is invalid or if the associated Libraries is 0 (or less)
	if (OwningActor == nullptr || ContextEffectsLibraries.Num() <= 0)
	{
		return;
	}

	// Create new Context Effect Set
	UAstroContextEffectsSet* EffectsLibrariesSet = NewObject<UAstroContextEffectsSet>(this);

	// Cycle through Libraries getting Soft Obj Refs
	for (const TSoftObjectPtr<UAstroContextEffectsLibrary>& ContextEffectSoftObj : ContextEffectsLibraries)
	{
		// Load Library Assets from Soft Obj refs
		// TODO Support Async Loading of Asset Data
		if (UAstroContextEffectsLibrary* EffectsLibrary = ContextEffectSoftObj.LoadSynchronous())
		{
			// Call load on valid Libraries
			EffectsLibrary->LoadEffects();

			// Add new library to Set
			EffectsLibrariesSet->AstroContextEffectsLibraries.Add(EffectsLibrary);
		}
	}

	// Update Active Actor Effects Map
	ActiveActorEffectsMap.Emplace(OwningActor, EffectsLibrariesSet);
}

void UAstroContextEffectsSubsystem::UnloadAndRemoveContextEffectsLibraries(AActor* OwningActor)
{
	// Early out if Owning Actor is invalid
	if (OwningActor == nullptr)
	{
		return;
	}

	// Remove ref from Active Actor/Effects Set Map
	ActiveActorEffectsMap.Remove(OwningActor);
}

