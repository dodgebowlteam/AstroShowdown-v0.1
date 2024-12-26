/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayAbility_BulletTime.generated.h"

/** Throws balls repeatedly at fixed target locations. */
UCLASS()
class ASTROSHOWDOWN_API UGameplayAbility_BulletTime : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGameplayAbility_BulletTime();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	struct FTransientResumeTimeRequest;
	void AddResumeTimeRequest(const FTransientResumeTimeRequest& NewResumeTimeRequest);
	void RemoveResumeTimeRequest(const FTransientResumeTimeRequest& RemovedResumeTimeRequest);
	void UpdateTimeDilation();

private:
	void OnInputEventReceived(FGameplayTag MatchingTag, const struct FGameplayEventData* Payload);

	// TODO (#cleanup): Move all of these to spreadsheets and turn them into FScalableFloats
protected:
	/** When enabled, the time dilation will change if the player moves while in Bullet Time. */
	UPROPERTY(EditAnywhere)
	bool bShouldResumeTimeWhileMoving = true;

	/** When enabled, the time dilation will change if the player throws while in Bullet Time. */
	UPROPERTY(EditAnywhere)
	bool bShouldResumeTimeWhileThrowing = true;

	/** Determines the custom time dilation that will be applied during bullet time. */
	UPROPERTY(EditAnywhere)
	float BulletTimeCustomTimeDilation = 0.0001f;

	/** Determines the custom time dilation that will be applied if bShouldResumeTimeWhileMoving is true. */
	UPROPERTY(EditAnywhere)
	float BulletTimeCustomTimeDilationWhileMoving = 0.5f;

	/** Determines the custom time dilation that will be applied if bShouldResumeTimeWhileThrowing is true. */
	UPROPERTY(EditAnywhere)
	float BulletTimeCustomTimeDilationWhileThrowing = 0.5f;

private:
	struct FTransientResumeTimeRequest
	{
		FGameplayTag ResumeReason;
		float ResumeTimeDilationValue = 1.f;

		FTransientResumeTimeRequest() = default;
		FTransientResumeTimeRequest(const FGameplayTag& InResumeReason) : ResumeReason(InResumeReason), ResumeTimeDilationValue(1.f) { }
		FTransientResumeTimeRequest(const FGameplayTag& InResumeReason, const float InResumeTimeDilationValue) : ResumeReason(InResumeReason), ResumeTimeDilationValue(InResumeTimeDilationValue) { }

		bool operator==(const FTransientResumeTimeRequest& Other) const
		{
			return ResumeReason == Other.ResumeReason;
		}
	};
	TArray<FTransientResumeTimeRequest> ResumeRequests;

	FDelegateHandle InputEventDelegateHandle;

};
