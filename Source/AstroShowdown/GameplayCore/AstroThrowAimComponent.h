/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "AstroThrowAimComponent.generated.h"

class AAstroCharacter;
class AAstroController;
class UNiagaraComponent;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FAstroThrowAimData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FHitResult TargetHitResult;

	UPROPERTY(BlueprintReadOnly)
	TArray<FHitResult> TrajectoryHits;

	UPROPERTY(BlueprintReadOnly)
	FVector CachedThrowDirection = FVector::ZeroVector;

};

UCLASS(BlueprintType)
class UAstroThrowAimProvider : public UObject
{
	GENERATED_BODY()

public:
	TFunction<FAstroThrowAimData ()> AimDataProviderCallback = nullptr;

public:
	UFUNCTION(BlueprintCallable)
	void GetAimThrowData(FAstroThrowAimData& OutAimThrowData, bool& bSuccess);

};

UCLASS()
class UAstroThrowAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void ActivateAim();
	void DeactivateAim();
	void UpdateAim();

private:
	FHitResult FindThrowTarget();
	void SimulateBallTrajectory(const FVector& TargetPosition, class AAstroBall* Ball, OUT TArray<FHitResult>& Hits);

private:
	TObjectPtr<UNiagaraComponent> GetOrCreateThrowAimPointer(int32 AimPointerIndex);
	TObjectPtr<UNiagaraComponent> GetOrCreateThrowAimCursor();
	void EnableThrowAimPointer(int32 AimPointerIndex, const FVector& AimStartPosition, const FVector& AimEndPosition);
	void DisableThrowAimPointer(int32 AimPointerIndex);
	void DisableAllThrowAimPointers();

	void EnableThrowAimCursor(const FVector& AimTargetPosition);
	void DisableThrowAimCursor();

	void UpdateAimPreview();

public:
	FORCEINLINE const FAstroThrowAimData& GetAstroThrowAimData() const { return CurrentAimResult; }
	FORCEINLINE UAstroThrowAimProvider* GetThrowAimProvider() const { return ThrowAimProvider; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Throw|Aiming")
	TEnumAsByte<ETraceTypeQuery> ThrowAimTargetChannel;

	UPROPERTY(EditDefaultsOnly, Category = "Throw|VFX")
	TObjectPtr<UNiagaraSystem> AimPointerParticleTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "Throw|VFX")
	TObjectPtr<UNiagaraSystem> AimCursorParticleTemplate;


private:
	UPROPERTY()
	TWeakObjectPtr<AAstroCharacter> CachedOwnerCharacter = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AAstroController> CachedOwnerController = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> AimPointerParticleComponents;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> AimCursorParticleComponent = nullptr;

	UPROPERTY()
	FAstroThrowAimData CurrentAimResult;

	UPROPERTY(Transient)
	TObjectPtr<UAstroThrowAimProvider> ThrowAimProvider = nullptr;

};
