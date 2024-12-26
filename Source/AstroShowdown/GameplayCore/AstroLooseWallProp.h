/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroPropActor.h"
#include "AstroLooseWallProp.generated.h"

class UInstancedStaticMeshComponent;
class UAnimToTextureDataAsset;

UCLASS(BlueprintType)
class AAstroLooseWallProp : public AAstroPropActor
{
	GENERATED_BODY()

#pragma region AstroPropActor
public:
	AAstroLooseWallProp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void OnConstruction(const FTransform& Transform);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
	virtual void OnPropDied_Implementation() override;
#pragma endregion

private:
	UPROPERTY(EditDefaultsOnly, Category = Settings)
	TObjectPtr<UAnimToTextureDataAsset> AnimToTextureData = nullptr;

	UPROPERTY(EditAnywhere, Category = Settings)
	int32 LooseWallHeight = 0;


private:
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> CachedInstancedStaticMeshComponent = nullptr;

	FTimerHandle DeathAnimationTimerHandle;
	float DeathAnimationStartTimestamp = -1.f;

private:
	void PlayIdleAnimation();

	void PlayDeathAnimation();
	void StopDeathAnimation();

private:
	/** @return Length in seconds of the death animation. */
	float GetDeathAnimationDuration() const;

	/** @return Time in seconds that the death animation has been playing. */
	float GetDeathAnimationElapsedTime() const;

	/**
	* @return Index of the animation. This could be either a hit or idle animation.
	* 
	* NOTE: This works due to a convention in the anim sequence order on the UAnimToTextureDataAsset. If the order ever changes, we'll have to modify this method.
	*/
	int32 GetAnimationSequenceIndex(const bool bIsHitAnimation) const;

};
