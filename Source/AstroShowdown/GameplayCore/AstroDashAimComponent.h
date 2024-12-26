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
#include "AstroDashAimComponent.generated.h"

class AAstroController;
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FAstroDashAimResult
{
	GENERATED_BODY()

	UPROPERTY()
	FVector DashStartPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector DashEndPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	AActor* DashTargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FHitResult DeflectionTargetHit;

	UPROPERTY(BlueprintReadOnly)
	TArray<FHitResult> DeflectionTrajectoryHits;

	UPROPERTY()
	FVector LastValidDashEndPositionWithDeflectionTarget = FVector::ZeroVector;

	uint8 bIsValidTrajectory : 1 = false;

};

UCLASS(BlueprintType)
class UAstroDashPayloadData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FAstroDashAimResult AstroDashAimResult;

};

UCLASS()
class UAstroDashAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void ActivateAim();
	void DeactivateAimPreview();
	void UpdateAim();

private:
	AActor* FindDashTarget(const FVector& DashTargetWorldPosition);
	bool ValidateDashTargetLocation(OUT FVector& DashTargetWorldPosition);
	bool ValidateDashTrajectory(const FVector& DashStartPosition, const FVector& DashTargetPosition);
	void SimulateBallTrajectory(const FVector& DashTargetWorldPosition, class AAstroBall* Ball, OUT TArray<FHitResult>& Hits);
	bool ShouldAimAssistDeflection(const FVector& DashTargetWorldPosition);

private:
	void CreateDashAimDecalCursor();
	void CreateDeflectAimDecalCursor();

	TObjectPtr<UNiagaraComponent> GetOrCreateDeflectAimPointer(int32 AimPointerIndex);
	void EnableDeflectAimPointer(int32 AimPointerIndex, const FVector& AimStartPosition, const FVector& AimEndPosition, const bool bHasValidTarget);
	void DisableDeflectAimPointer(int32 AimPointerIndex);
	void DisableAllDeflectAimPointers();

	void UpdateAimPreview(const FVector& DashTargetWorldPosition);

public:
	FORCEINLINE const TOptional<FAstroDashAimResult>& GetAstroDashAimResult() const { return CurrentAimResult; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Aim", meta=(UIMin = 0.f, UIMax = 1000.f))
	float DashAimRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	TEnumAsByte<ETraceTypeQuery> DashTargetChannel;

	/** The smaller the value, the bigger the assist. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim Assist", meta = (UIMin = -1.f, UIMax = 1.f))
	float DeflectionAimAssistAngleThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> DeflectAimPointerParticle;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FLinearColor DeflectAimPointerTargetFoundColor;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FLinearColor DeflectAimPointerNoTargetColor;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor|Dash")
	TObjectPtr<UMaterialInterface> DashAimDecalCursorMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor|Dash")
	FLinearColor DashAimCursorValidColor;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor|Dash")
	FLinearColor DashAimCursorInvalidColor;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor|Deflect")
	TObjectPtr<UMaterialInterface> DeflectAimDecalCursorMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor|Deflect")
	FLinearColor DeflectAimCursorNoTargetColor;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor|Deflect")
	FLinearColor DeflectAimCursorYesTargetColor;

private:
	UPROPERTY(Transient)
	UDecalComponent* DashAimDecalCursor = nullptr;

	UPROPERTY(Transient)
	class UDecalComponent* DeflectAimDecalCursor = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AAstroController> CachedOwnerController = nullptr;

	UPROPERTY()
	TOptional<FAstroDashAimResult> CurrentAimResult;

	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> DeflectAimPointerParticleComponents;

	float CachedOwnerCharacterRadius = 0.f;
	float CachedOwnerCharacterHalfHeight = 0.f;

};
