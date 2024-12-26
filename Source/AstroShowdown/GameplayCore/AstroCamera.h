/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "GameFramework/Actor.h"
#include "AstroCamera.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;

/**
* We're using orthographic rendering, which as of UE 5.5 is still beta, so there may be bugs.
*/
UCLASS()
class AAstroCamera : public AActor
{
	GENERATED_BODY()

#pragma region AActor
public:
	AAstroCamera();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
#pragma endregion

protected:
	UPROPERTY(EditDefaultsOnly)
	float OrthoWidthInterpolationSpeed = 400.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRootComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USpringArmComponent> SpringArmComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> CameraComponent = nullptr;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CameraTarget = nullptr;

	float TargetOrthoWidth = 0.f;

public:
	UFUNCTION(BlueprintCallable)
	void SetCameraTarget(AActor* InCameraTarget);

	UFUNCTION(BlueprintCallable)
	void SetTargetOrthoWidth(const float InTargetOrthoWidth);

private:
	/**
	* Recalculates the spring arm's length based on the ortho width. This is useful to avoid near clipping
	* due to ortho width sizes, and also to keep the camera at a reasonable height for shadow calculations.
	*/
	void RecalculateSpringArmLength();

};
