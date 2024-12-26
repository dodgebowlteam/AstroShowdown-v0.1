/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's Context Effects System, though we modified it to suit our needs.
*/

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstroContextEffectsSubsystem.generated.h"

enum EPhysicalSurface : int;

class AActor;
class UAudioComponent;
class UAstroContextEffectsLibrary;
class UNiagaraComponent;
class USceneComponent;
struct FFrame;
struct FGameplayTag;
struct FGameplayTagContainer;
struct FAstroContextEffectsParameters;


UCLASS(config = Game, defaultconfig, meta = (DisplayName = "AstroContextEffects"))
class UAstroContextEffectsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere)
	TMap<TEnumAsByte<EPhysicalSurface>, FGameplayTag> SurfaceTypeToContextMap;
};


UCLASS()
class UAstroContextEffectsSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TSet<TObjectPtr<UAstroContextEffectsLibrary>> AstroContextEffectsLibraries;
};


UCLASS()
class UAstroContextEffectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	void SpawnContextEffects(AActor* SpawnInstigator, const FAstroContextEffectsParameters& ContextEffectsParameters, TArray<UAudioComponent*>& OutAudios, TArray<UNiagaraComponent*>& OutNiagaraEffects);

	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	bool GetContextFromSurfaceType(TEnumAsByte<EPhysicalSurface> PhysicalSurface, FGameplayTag& Context);

	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	void LoadAndAddContextEffectsLibraries(AActor* OwningActor, TSet<TSoftObjectPtr<UAstroContextEffectsLibrary>> ContextEffectsLibraries);

	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	void UnloadAndRemoveContextEffectsLibraries(AActor* OwningActor);

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, TObjectPtr<UAstroContextEffectsSet>> ActiveActorEffectsMap;

};
