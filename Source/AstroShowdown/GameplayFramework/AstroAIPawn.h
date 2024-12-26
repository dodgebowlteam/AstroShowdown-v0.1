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
#include "GameFramework/Pawn.h"
#include "AstroAIPawn.generated.h"

class UGameplayEffect;
class UHealthAttributeSet;

UCLASS()
class ASTROSHOWDOWN_API AAstroAIPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

#pragma region APawn
public:
	AAstroAIPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController);
#pragma endregion


#pragma region IAbilitySystemInterface
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

protected:
	UPROPERTY(Transient)
	UHealthAttributeSet* HealthAttributeSet = nullptr;

	UPROPERTY(Transient, VisibleAnywhere)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;
#pragma endregion


protected:
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> PawnInitialStatsGE = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UGameplayEffect> ReviveGE = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> IndicatorIcon = nullptr;

public:
	UPROPERTY(BlueprintReadWrite)
	bool bDead = false;

protected:
	UPROPERTY()
	FActiveGameplayEffectHandle ReviveEffectHandle;

protected:
	/** Applies GE with the default values for all attributes */
	UFUNCTION(BlueprintCallable)
	void ResetAbilitySystemProperties();

protected:
	virtual void NativeDie();
	void NativeRevive();

	UFUNCTION(BlueprintImplementableEvent)
	void Die_BP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Revive_BP();

	UFUNCTION(BlueprintCallable)
	void StartReviveCounter();
	void StopReviveCounter();

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

	UFUNCTION()
	void OnAstroGameReset();

	UFUNCTION(BlueprintNativeEvent)
	void OnAstroWorldInitialized();
	virtual void OnAstroWorldInitialized_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void OnAstroWorldActivated();
	virtual void OnAstroWorldActivated_Implementation();

	UFUNCTION()
	void OnAstroGameResolved();

public:
	UFUNCTION(BlueprintCallable)
	float GetReviveDuration() const;

};
