/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ModularPlayerState.h"
#include "AstroPlayerState.generated.h"

class UAstroPlayerAttributeSet;
class UHealthAttributeSet;

/**
 * A PlayerState is created for each client/player connected to the server (works the same in standalone games).
 * PlayerStates are replicated to all clients, and should contain game relevant information about the player that should be networked, such as playername, score, etc.
 */
UCLASS()
class ASTROSHOWDOWN_API AAstroPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
#pragma region AActor
public:
	// Sets default values for this actor's properties
	AAstroPlayerState();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
#pragma endregion


#pragma region IAbilitySystemInterface
public:

	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

	FORCEINLINE UHealthAttributeSet* GetHealthAttributeSet() const
	{
		return HealthAttributeSet;
	}

	FORCEINLINE UAstroPlayerAttributeSet* GetPlayerAttributeSet() const
	{
		return PlayerAttributeSet;
	}

protected:
	UPROPERTY(Transient)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;

	UPROPERTY(Transient)
	UHealthAttributeSet* HealthAttributeSet = nullptr;

	UPROPERTY(Transient)
	UAstroPlayerAttributeSet* PlayerAttributeSet = nullptr;
#pragma endregion


};
