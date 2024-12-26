/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstroUtilitiesBlueprintLibrary.generated.h"

class AActor;
class AAstroCharacter;
class UActorComponent;
class USceneComponent;
class AGameStateBase;

UCLASS()
class UAstroUtilitiesBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Astro Showdown|Utils", meta = (WorldContext = "InWorldContextObject"))
	static UActorComponent* AddInstancedComponent(UObject* InWorldContextObject, AActor* Owner, TSubclassOf<UActorComponent> AddedComponentClass);
	
	UFUNCTION(BlueprintCallable, Category = "Astro Showdown|Utils", meta = (WorldContext = "InWorldContextObject"))
	static void ClearAllChildrenComponents(UObject* InWorldContextObject, USceneComponent* Owner);

	UFUNCTION(BlueprintCallable, Category = "Astro Showdown|Utils", meta = (WorldContext = "InWorldContextObject"))
	static void ClearAllActorChildrenComponents(UObject* InWorldContextObject, AActor* Owner, TSubclassOf<UActorComponent> ComponentClass);

	UFUNCTION(BlueprintCallable, Category = "Astro Showdown|Utils", meta = (WorldContext = "WorldContextObject"))
	static void RestartLevel(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Astro Showdown|Utils", meta = (WorldContext = "WorldContextObject"))
	static void GetPenetrationDepthFromHitResult(UObject* WorldContextObject, const FHitResult& HitResult, float& OutPenetrationDepth);

	UFUNCTION(BlueprintPure, Category = "Astro Showdown|Utils", meta = (WorldContext = "WorldContextObject"))
	static int32 GetDefaultDepthStencilOutlineValue(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Astro Showdown|Utils", meta = (WorldContext = "WorldContextObject"))
	static int32 GetInteractableDepthStencilOutlineValue(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Astro Showdown|Utils", meta = (ComponentClass = "/Script/Engine.GameStateBase"), meta = (DeterminesOutputType = "GameStateClass", WorldContext = "WorldContextObject"))
	static AGameStateBase* GetGameStateByClass(UObject* WorldContextObject, TSubclassOf<AGameStateBase> GameStateClass);

	static AAstroCharacter* FindLocalPlayerAstroCharacter(UObject* WorldContextObject);

};
