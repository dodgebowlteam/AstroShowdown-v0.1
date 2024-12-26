/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "AstroPropActor.generated.h"

class UHealthAttributeSet;
class UStaticMeshComponent;

UCLASS()
class ASTROSHOWDOWN_API AAstroPropActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

#pragma region AActor
public:
	AAstroPropActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
#pragma endregion


#pragma region IAbilitySystemInterface
protected:
	UPROPERTY(Transient)
	UHealthAttributeSet* HealthAttributeSet = nullptr;

	UPROPERTY(Transient, VisibleAnywhere)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;

public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}
#pragma endregion


#pragma region AAstroPropActor
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient)
	UStaticMeshComponent* PropStaticMeshComponent = nullptr;

	UPROPERTY(EditDefaultsOnly)
	uint8 bShouldAutoDestroyOnDeath : 1 = true;

protected:
	UFUNCTION(BlueprintNativeEvent)
	void OnDamaged(float OldHealthValue, float NewHealthValue, const FHitResult& HitResult);
	virtual void OnDamaged_Implementation(float OldHealthValue, float NewHealthValue, const FHitResult& HitResult);

	UFUNCTION(BlueprintNativeEvent)
	void OnPropDied();
	virtual void OnPropDied_Implementation() {}

#pragma endregion


};
