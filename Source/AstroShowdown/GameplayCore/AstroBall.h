/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroInteractableInterface.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

#include "AstroBall.generated.h"

class UFMODEvent;
class UNiagaraComponent;

UENUM()
enum class EBallMovementType : uint8
{
	Regular,
	Ricochet,
};

UENUM(BlueprintType)
enum class EBallPhysicsState : uint8
{
	Inactive,			// Turns the ball into a mesh with no physics
	Projectile,			// Turns the ball into a projectile that can be thrown around
	Ragdoll,			// Makes the ball physically simulated
	Attachment,			// Turns the ball into a static mesh, that has no collision and can be attached to a bone
};

USTRUCT()
struct FAstroBallGrabInput
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVector DeflectStartPosition = FVector::ZeroVector;

	UPROPERTY()
	float DeflectSpeed = 0.f;
};

UCLASS()
class ASTROSHOWDOWN_API AAstroBall : public AActor, public IAstroInteractableInterface
{
	GENERATED_BODY()
	
public:	
	AAstroBall();

#pragma region AActor
protected:
	virtual void OnConstruction(const FTransform& Transform);
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void LifeSpanExpired() override;

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
#pragma endregion


#pragma region IAstroInteractableInterface
public:
	virtual void Interact_Implementation(AAstroCharacter* InteractionInstigator) override;
	virtual bool IsInteractable_Implementation() const override;
	virtual void OnEnterInteractionRange_Implementation() override;
	virtual void OnExitInteractionRange_Implementation() override;
#pragma endregion


#pragma region AAstroBall
public:
	/** Throws the ball towards at a given direction and speed. */
	void Throw(FGameplayEffectSpecHandle InDamageGameplayEffectSpecHandle, const FVector& InDirection, const float InSpeed);

	/** Disables a bunch of components, allowing the 'deflector' to grab the ball. */
	UFUNCTION(BlueprintCallable)
	void StartGrab(USceneComponent* GrabParent, FName GrabParentSocketName);

	/** Releases a grabbed ball, and then deflects it. Should always be paired with a StartGrab call.  */
	UFUNCTION(BlueprintCallable)
	void ReleaseGrabDeflect(FGameplayEffectSpecHandle DeflectDamageEffect, const FVector& DeflectDirection);

	/**
	* Releases a grabbed ball, and then throws it. Should always be paired with a StartGrab call.
	* 
	* @param bSweepHit When true, will overlap check the ball at the origin, to see if it's hit anything. Useful to
	* avoid missing a throw whose origin is on top of the target.
	*/
	UFUNCTION(BlueprintCallable)
	void ReleaseGrabThrow(FGameplayEffectSpecHandle ThrowDamageEffect, const FVector& ThrowOrigin, const FVector& ThrowDirection, const float ThrowSpeed);

	/** Drops a grabbed ball. Should always be paired with a StartGrab call.  */
	UFUNCTION(BlueprintCallable)
	void DropGrab(const bool bSilent = false);
	
	/* Effectively destroys the ball and plays its destruction FX. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Die();
	virtual void Die_Implementation();

	/** Sets the ball's physics state. Use this before throwing it around, attaching it to something, or to make it simulate physics.  */
	UFUNCTION(BlueprintCallable)
	void SetBallPhysicsState(const EBallPhysicsState BallPhysicsState);

	/*
	* Simulates the trajectory of the ball if it was following a given direction, and returns all bounces the ball would make if going at this direction.
	*/
	void SimulateTrajectory(const FVector& StartDirection, OUT TArray<FHitResult>& OutBounces, const float RadiusMultiplier = 1.f);
	void SimulateTrajectory(const FVector& StartPosition, const FVector& StartDirection, OUT TArray<FHitResult>& OutBounces, const float RadiusMultiplier = 1.f);
	
	/*
	* Simulates how the ball would bounce had it hit a certain object.
	* NOTE: This is a rough copy of what's in UProjectileMovementComponent::ComputeBounceResult.
	* 
	* @returns deflection direction
	*/
	FVector SimulateDeflection(FHitResult& Hit, const FVector& CurrentVelocity);

	/** Activates a ball that was spawned from the pool. */
	void ActivateFromPool();
	/** Returns a ball to the pool. Should be paired with a call to ActivateFromPool. */
	void ReturnToPool();

private:
	void SetBallPhysicsStateInactive();
	void SetBallPhysicsStateProjectile();
	void SetBallPhysicsStateAttachment();
	void SetBallPhysicsStateRagdoll();

public:
	FORCEINLINE bool IsDead() const { return bDead; }

	/** Get the height at which all balls should travel */
	static float GetBallTravelHeight();

	/** Applies the ball's travel height to a position. */
	static FVector ApplyTravelHeightFixupToPosition(const FVector& InPosition);

protected:
	UFUNCTION()
	void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult);
	void OnActorHit(AActor* HitActor, const FHitResult& HitResult);

	/** Called when the ball is spawned. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpawn_BP();

	bool ShouldFilterTarget(UAbilitySystemComponent* TargetASC);

	void PlayHitFX();

	void PlayBallHueAnimation();
	void ResetBallHueAnimation();

	void SetBallRotationAnimation(const bool bEnabled);

	void UpdateBallMovementProperties();

public:
	/* Returns true if the ball may damage the actor. */
	static bool CanDamageActor(const AActor* TargetActor);


protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ball Movement")
	EBallMovementType BallMovementType = EBallMovementType::Regular;

	UPROPERTY(EditDefaultsOnly, Category = "Ball Movement", Meta = (EditCondition = "BallMovementType == EBallMovementType::Ricochet"))
	int32 MaxRicochetBounces = 4;

	UPROPERTY(EditDefaultsOnly, Category = "Deflect", Meta = (UIMin = 0.1f, UIMax = 10000.f))
	float DeflectSpeed = 6000.f;

	/** How long it takes for this object to become interactable again once it's been interacted with. */
	UPROPERTY(EditDefaultsOnly, Category = "Deflect", Meta = (UIMin = 0.f, UIMax = 10.f))
	float InteractionCooldown = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<class UNiagaraSystem> BallHitVFX;

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	TObjectPtr<UFMODEvent> BallHitSFX;

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	TObjectPtr<UFMODEvent> BallInteractSFX;

	UPROPERTY(EditDefaultsOnly, Category = "SFX")
	TObjectPtr<UFMODEvent> BallDropSFX;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	UPrimitiveComponent* CollisionComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Transient)
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient, Category = "VFX")
	UNiagaraComponent* BallTrailComponent = nullptr;

protected:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstroBallGenericDynamicDelegate);
	UPROPERTY(BlueprintAssignable)
	FAstroBallGenericDynamicDelegate OnAstroBallHit;

	UPROPERTY(BlueprintAssignable)
	FAstroBallGenericDynamicDelegate OnAstroBallBounce;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstroBallPhysicsStateChanged, EBallPhysicsState, OldState, EBallPhysicsState, NewState);
 	UPROPERTY(BlueprintAssignable)
	FAstroBallPhysicsStateChanged OnAstroBallPhysicsStateChanged;

public:
	DECLARE_MULTICAST_DELEGATE(FAstroBallGenericDelegate);
	FAstroBallGenericDelegate OnAstroBallDropped;
	FAstroBallGenericDelegate OnAstroBallThrown;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAstroBallActivated, AAstroBall*);
	FOnAstroBallActivated OnAstroBallActivated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstroBallDeactivated, AAstroBall*, Ball);
	UPROPERTY(BlueprintAssignable)
	FOnAstroBallDeactivated OnAstroBallDeactivated;


protected:
	/**
	* If true, this ball has hit something and is in the process of dying. This does NOT mean that the ball has been despawned yet.
	*/
	UPROPERTY(BlueprintReadWrite, Category = "VFX")
	bool bDead = false;

protected:
	UPROPERTY(BlueprintReadWrite)
	FGameplayEffectSpecHandle DamageGameplayEffectSpecHandle;

	UPROPERTY()
	FTimerHandle BallHueAnimationTimerHandle;

	/** This value is cached when the player is grabbing the ball. We also use it to verify if the player is grabbing it. */
	TOptional<FAstroBallGrabInput> GrabInput;

private:
	int32 CurrentBounceCount = 0;

	FVector PreviousVelocity = FVector::ZeroVector;

	float LastInteractionTimestamp = 0.f;

	EBallPhysicsState CurrentBallPhysicsState = EBallPhysicsState::Inactive;

#pragma endregion

};
