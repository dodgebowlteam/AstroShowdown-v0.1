/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*
* NOTE: Based on Epic Games Lyra's Context Effects System, though we modified it to suit our needs.
*/

#pragma once

#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "AstroContextEffectsInterface.generated.h"


class UAnimSequenceBase;
class UObject;
class USceneComponent;
struct FFrame;


UENUM()
enum EEffectsContextMatchType : int
{
	ExactMatch,
	BestMatch
};


USTRUCT(BlueprintType)
struct FAstroContextEffectsParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName Bone = FName("None");

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag MotionEffect;

	UPROPERTY(BlueprintReadWrite, Transient)
	USceneComponent* StaticMeshComponent = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, Transient)
	UAnimSequenceBase* AnimationSequence = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bHitSuccess = false;

	UPROPERTY(BlueprintReadWrite)
	FHitResult HitResult = {};

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer Contexts = {};

	UPROPERTY(BlueprintReadWrite)
	FVector VFXScale = FVector(1.f);

	UPROPERTY(BlueprintReadWrite)
	float AudioVolume = 1.f;

	UPROPERTY(BlueprintReadWrite)
	float AudioPitch = 1.f;
};


UINTERFACE(Blueprintable)
class UAstroContextEffectsInterface : public UInterface
{
	GENERATED_BODY()
};

class IAstroContextEffectsInterface : public IInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnimMotionEffect(const FAstroContextEffectsParameters& ContextEffectsParameters);
};

