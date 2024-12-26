/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroAIPawn.h"
#include "BallMachine.generated.h"

class AAstroBall;
class UAnimMontage;
class UNiagaraSystem;
class UFMODEvent;

UENUM(BlueprintType)
enum class EBallMachineTargetType : uint8
{
	Static,
	Dynamic,
	Forward
};

USTRUCT(BlueprintType)
struct FThrowAtTargetParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AAstroBall> ThrownBallClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> ShootMontage = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UFMODEvent> ShootSFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Meta = (UIMin = 100.0f, UIMax = 10000.f))
	float ThrowSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, Meta = (UIMin = 0.0f, UIMax = 1.f, ToolTip = "Determines how further the actor will try to throw at when shooting at a dynamic target."))
	float ThrowLookaheadStrength = 0.12f;

	UPROPERTY(EditDefaultsOnly)
	FName MuzzleSocketName = {};

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	EBallMachineTargetType ThrowTargetType = EBallMachineTargetType::Forward;
	
	UPROPERTY(EditInstanceOnly, Meta = (EditCondition = "ThrowTargetType == EBallMachineTargetType::Static", EditConditionHides, MakeEditWidget, ToolTip = "Targets that the ball machine will throw towards. These are all relative to the machine's current position."))
	TArray<FVector> ThrowTargets;

	UPROPERTY(EditInstanceOnly, Meta = (EditCondition = "ThrowTargetType == EBallMachineTargetType::Dynamic", EditConditionHides))
	TSubclassOf<AActor> ThrowTargetActorClass = nullptr;

	UPROPERTY(EditInstanceOnly, Meta = (UIMin = 0.0f, UIMax = 10.f, ToolTip = "Time (in seconds) that the machine will take to shoot subsequent balls."))
	float ThrowDelay = 0.5f;

	UPROPERTY(EditInstanceOnly, Meta = (UIMin = 0.0f, UIMax = 10.f, ToolTip = "Time (in seconds) that the machine will take to start shooting balls."))
	float ThrowStartDelay = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> AimPointerParticleTemplate;

};

UCLASS()
class ASTROSHOWDOWN_API ABallMachine : public AAstroAIPawn
{
	GENERATED_BODY()

#pragma region APawn
public:	
	ABallMachine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void PossessedBy(AController* NewController);
#pragma endregion


#pragma region ABallMachine
public:
	UPROPERTY(EditDefaultsOnly, Transient, Category = "Abilities")
	TSubclassOf<class UGameplayAbility> ThrowBallAbilityClass;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Targeting")
	FThrowAtTargetParameters ThrowParameters;

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	USkeletalMeshComponent* BallMachineMesh = nullptr;

protected:
	UPROPERTY()
	TOptional<FVector> CurrentTargetLocation;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetLocationUpdated, const FVector&)
	FOnTargetLocationUpdated OnTargetLocationUpdated;

protected:
	virtual void NativeDie() override;

protected:
	UFUNCTION(BlueprintCallable)
	void ActivateShootingAbility(const bool bInstant = false);

public:
	USkeletalMeshComponent* GetMeshComponent() const { return BallMachineMesh; }
	TOptional<FVector> GetCurrentTargetLocation() const { return CurrentTargetLocation; }
	void SetCurrentTargetLocation(const FVector& NewTargetLocation);

#pragma endregion

};
