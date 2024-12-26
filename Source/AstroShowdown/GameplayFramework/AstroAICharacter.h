/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstroAICharacter.generated.h"

class UGameplayEffect;

UCLASS()
class ASTROSHOWDOWN_API AAstroAICharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


#pragma region AActor
public:
	// Sets default values for this character's properties
	AAstroAICharacter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
#pragma endregion


#pragma region IAbilitySystemInterface
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

protected:
	UPROPERTY(Transient)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;

	UPROPERTY(Transient)
	class UHealthAttributeSet* HealthAttributeSet = nullptr;
#pragma endregion


	// TODO (#cleanup): Move this entire region to a modular component that can be shared with AAstroAIPawn
#pragma region AAstroAICharacter
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> InitialStatsGE = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> ReviveGE = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> IndicatorIcon = nullptr;

public:
	UPROPERTY(BlueprintReadOnly)
	bool bDead = false;

protected:
	UPROPERTY()
	FActiveGameplayEffectHandle ReviveEffectHandle;

protected:
	/** Applies GE with the default values for all attributes */
	UFUNCTION(BlueprintCallable)
	void ResetAbilitySystemProperties();

protected:
	UFUNCTION(BlueprintCallable)
	void StartReviveCounter();
	void StopReviveCounter();

	UFUNCTION(BlueprintNativeEvent)
	void Die(const FHitResult& DeathHitResult);
	virtual void Die_Implementation(const FHitResult& DeathHitResult);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Revive();
	virtual void Revive_Implementation();

protected:
	UFUNCTION()
	void OnDamaged(const float OldHealthValue, const float NewHealthValue, const FHitResult& HitResult);

	UFUNCTION()
	void OnReviveCounterChanged(const float OldReviveCounterValue, const float NewReviveCounterValue);
	UFUNCTION(BlueprintImplementableEvent)
	void OnReviveCounterChanged_BP(const float OldReviveCounterValue, const float NewReviveCounterValue);

	UFUNCTION()
	void OnReviveCounterFinished();
	UFUNCTION(BlueprintImplementableEvent)
	void OnReviveCounterFinished_BP();

	UFUNCTION(BlueprintImplementableEvent)
	void OnReviveCounterStopped_BP();

	UFUNCTION(BlueprintNativeEvent)
	void OnAstroWorldInitialized();
	virtual void OnAstroWorldInitialized_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void OnAstroWorldActivated();
	virtual void OnAstroWorldActivated_Implementation();

	UFUNCTION()
	void OnAstroWorldResolved();

public:
	UFUNCTION(BlueprintCallable)
	float GetReviveDuration() const;
#pragma endregion

};
