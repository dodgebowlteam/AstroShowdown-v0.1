/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/DeveloperSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "AstroTimeDilationSubsystem.generated.h"


class UNiagaraComponent;
class UMaterialParameterCollection;


UCLASS(config = Game, defaultconfig, meta = (DisplayName = "AstroTimeDilationSettings"))
class UAstroTimeDilationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UMaterialParameterCollection> RealTimeMPC = nullptr;

};

/**
* AstroTimeDilationSubsystem contains Astro-specific utility methods for time dilation.
*/
UCLASS()
class UAstroTimeDilationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

#pragma region UWorldSubsystem
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
#pragma endregion


#pragma region FTickableGameObject
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
#pragma endregion


#pragma region UAstroTimeDilationSubsystem
public:
	/*
	* Sets a timer that will ignore time dilation and execute for a real-world duration.
	* 
	* NOTE: This method expects TimerHandle to be a valid reference throughout the duration of the timer.
	*/
	void SetAbsoluteTimer(FTimerHandle& TimerHandle, const FTimerDelegate& TimerDelegate, float Duration);

	/*
	* Clears an absolute timer given a TimerHandle.
	*/
	void ClearAbsoluteTimer(FTimerHandle& TimerHandle);

	/*
	* Forces an actor to ignore custom time dilation values.
	*/
	UFUNCTION(BlueprintCallable)
	void RegisterIgnoreTimeDilation(AActor* Actor);
	UFUNCTION(BlueprintCallable)
	void UnregisterIgnoreTimeDilation(AActor* Actor);

	UFUNCTION(BlueprintCallable)
	void RegisterIgnoreTimeDilationParticle(UNiagaraComponent* ParticleComponent);
	UFUNCTION(BlueprintCallable)
	void UnregisterIgnoreTimeDilationParticle(UNiagaraComponent* ParticleComponent);

	/*
	* Sets the global time dilation.
	*/
	UFUNCTION(BlueprintCallable)
	void SetGlobalTimeDilation(float TimeDilation, const bool bSmooth = false, const float CustomTimeDilationIncrementStep = -1.f);

	/*
	* Returns undilated duration of the game.
	*/
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	float GetRealTimeSeconds() const;

	/*
	* Returns undilated DeltaSeconds.
	*/
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	float GetRealTimeDeltaSeconds() const;

	/*
	* Applies global time dilation to a given Duration value.
	*/
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static float GetDilatedTime(UObject* WorldContextObject, float Duration);

	/*
	* Removes global time dilation from a given Duration value.
	*/
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static float GetUndilatedTime(UObject* WorldContextObject, float Duration);

	/*
	* Returns the current time dilation value.
	*/
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static float GetGlobalTimeDilation(UObject* WorldContextObject);

	/*
	* Returns the inverse of the current time dilation value. This value is useful when we want some object to ignore a custom time dilation value.
	*/
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static float GetGlobalTimeDilationInverse(UObject* WorldContextObject);

private:
	/*
	* Updates the custom time dilation for all objects that want to ignore time dilation.
	*/
	void UpdateIgnoredObjectsTimeDilation();
	template <typename T>
	void UpdateIgnoredObjectsTimeDilationImpl(TArray<TWeakObjectPtr<T>>& IgnoreTimeDilationObjects, TArray<TWeakObjectPtr<T>>& RemovedIgnoreTimeDilationObjects);

	/*
	* Dispatches an UpdateIgnoredObjectsTimeDilation call to the next frame.
	*/
	void DispatchUpdateIgnoredObjectsTimeDilation();

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> IgnoreTimeDilationActors;
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> RemovedIgnoreTimeDilationActors;

	UPROPERTY()
	TArray<TWeakObjectPtr<UNiagaraComponent>> IgnoreTimeDilationParticles;
	UPROPERTY()
	TArray<TWeakObjectPtr<UNiagaraComponent>> RemovedIgnoreTimeDilationParticles;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> RealTimeMPC = nullptr;

	float PreviousRealTimeSeconds = 0.f;
	float CurrentRealTimeSeconds = 0.f;

	struct FTargetTimeDilationValue
	{
		float TargetTimeDilation = 0.f;
		float CustomTimeDilationIncrementStep = -1.f;
	};
	TOptional<FTargetTimeDilationValue> TargetTimeDilationValue;
	bool bAreTimeDilationIgnoredActorsDirty;

#pragma endregion
};