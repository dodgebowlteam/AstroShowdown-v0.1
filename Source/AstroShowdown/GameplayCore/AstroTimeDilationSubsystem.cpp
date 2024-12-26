/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroTimeDilationSubsystem.h"
#include "AstroBall.h"
#include "Engine/Engine.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraComponent.h"

namespace AstroTimeDilationSubsystemVars
{
	static bool bEnableUpdateIgnoredCallsDispatch = false;
	static FAutoConsoleVariableRef CVarEnableUpdateIgnoredCallsDispatch(
		TEXT("AstroTimeDilation.EnableUpdateIgnoredCallsDispatch"),
		bEnableUpdateIgnoredCallsDispatch,
		TEXT("When enabled, time dilation ignore calls are delayed until the end of the GT frame, instead of being called immediately."),
		ECVF_Default);

	static float TimeDilationIncrementStep = 3.f;
	static FAutoConsoleVariableRef CVarTimeDilationIncrementStep(
		TEXT("AstroTimeDilation.IncrementStep"),
		TimeDilationIncrementStep,
		TEXT("Determines the increment step for time dilation changes."),
		ECVF_Default);
}

namespace AstroStatics
{
	static const FName RealTimeMaterialParameterName = "RealTimeSeconds";
	static const FName PreviousRealTimeMaterialParameterName = "PreviousRealTimeSeconds";
}

void UAstroTimeDilationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UAstroTimeDilationSettings* TimeDilationSettings = GetDefault<UAstroTimeDilationSettings>())
	{
		RealTimeMPC = TimeDilationSettings->RealTimeMPC.LoadSynchronous();
	}
}


// UAstroTimeDilationSubsystem is implemented as an FTickableGameObject, so its tick comes after physics and TimerManager.
// All time dilation changes are polled in this subsystem's tick, so they're not instant, and other ticks may take place
// with old time dilation values, causing inconsistent behavior.
// 
// One prime example of this is physics simulation. Physics tick DeltaTime is calculated during TG_PrePhysics (check FChaosScene
// ::SetUpForFrame), and iirc physics ticking happens on a separate thread, so there's no one-to-one relation between GT and PT ticks.
// This implies that time dilation changes may not be picked accurately by the PT.
// 
// Tick order goes: (source = https://dev.epicgames.com/community/learning/tutorials/D7P8/unreal-engine-advanced-tick-functionality-tickables-multi-tick)
// ==============================================
// RunTickGroup(TG_PrePhysics);
// RunTickGroup(TG_StartPhysics);
// RunTickGroup(TG_DuringPhysics);
// RunTickGroup(TG_EndPhysics);
// RunTickGroup(TG_PostPhysics);
// > tick timers (delays, latent actions...)
// GetTimerManager().Tick(DeltaSeconds);
// > tick tickables that inherit from FTickableGameObject
// FTickableGameObject::TickObjects(this, TickType, bIsPaused, DeltaSeconds);			<< We're here!!!!
// RunTickGroup(TG_PostUpdateWork);
// RunTickGroup(TG_LastDemotable);
// ==============================================
// 
void UAstroTimeDilationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PreviousRealTimeSeconds = CurrentRealTimeSeconds;
	CurrentRealTimeSeconds = GetWorld()->GetRealTimeSeconds();

	const UWorld* World = GetWorld();
	FPhysScene* PhysScene = World ? World->GetPhysicsScene() : nullptr;
	if (!World || !PhysScene)
	{
		return;
	}

	// Updates the real time MPC value, which we may use to do time-based animation on materials while ignoring time dilation
	if (RealTimeMPC)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, RealTimeMPC, AstroStatics::RealTimeMaterialParameterName, CurrentRealTimeSeconds);
		UKismetMaterialLibrary::SetScalarParameterValue(this, RealTimeMPC, AstroStatics::PreviousRealTimeMaterialParameterName, PreviousRealTimeSeconds);
	}

	// Steps from the current time dilation value to the target value.
	// NOTE: We do this primarily to avoid physics simulation {over/under}ticks with big time dilation deltas.
	if (TargetTimeDilationValue.IsSet())
	{
		const float TargetTimeDilation = TargetTimeDilationValue->TargetTimeDilation;
		const float CustomTimeDilationStep = TargetTimeDilationValue->CustomTimeDilationIncrementStep;
		const float CurrentGlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);

		const float TimeDilationStep = (CustomTimeDilationStep > 0.f ? CustomTimeDilationStep : AstroTimeDilationSubsystemVars::TimeDilationIncrementStep) * GetRealTimeDeltaSeconds();
		const float CurrentGlobalTimeDilationStep = CurrentGlobalTimeDilation > TargetTimeDilation ? -TimeDilationStep : TimeDilationStep;
		
		// If the next step is different than the current, we can assume that we've stepped over the target value, and
		// round the new time dilation to the target value.
		float NewGlobalTimeDilation = CurrentGlobalTimeDilation + CurrentGlobalTimeDilationStep;
		const float NextGlobalTimeDilationStep = NewGlobalTimeDilation > TargetTimeDilation ? -TimeDilationStep : TimeDilationStep;
		if (const bool bChangedDirection = CurrentGlobalTimeDilationStep != NextGlobalTimeDilationStep)
		{
			NewGlobalTimeDilation = TargetTimeDilation;
			TargetTimeDilationValue.Reset();
		}

		UGameplayStatics::SetGlobalTimeDilation(this, NewGlobalTimeDilation);

		bAreTimeDilationIgnoredActorsDirty = true;
	}

	// NOTE: We're delaying this to the end of the frame to prevent sudden animation/movement/etc {over/under}ticks
	if (bAreTimeDilationIgnoredActorsDirty)
	{
		UpdateIgnoredObjectsTimeDilation();
		bAreTimeDilationIgnoredActorsDirty = false;
	}
}

TStatId UAstroTimeDilationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAstroTimeDilationSubsystem, STATGROUP_Tickables);
}

namespace AstroTimeDilationSubsystemUtils
{
	namespace Private
	{
		void ExecuteRealTimeTicker(TWeakObjectPtr<UAstroTimeDilationSubsystem> TimeDilationSubsystem, FTimerHandle& TimerHandle, const FTimerDelegate& TimerDelegate, float Duration, float CurrentCounter)
		{
			if (!TimeDilationSubsystem.IsValid())
			{
				return;
			}

			CurrentCounter += TimeDilationSubsystem->GetRealTimeDeltaSeconds();
			if (CurrentCounter >= Duration)
			{
				TimerDelegate.ExecuteIfBound();
			}
			else
			{
				UWorld* World = TimeDilationSubsystem->GetWorld();
				TimerHandle = World->GetTimerManager().SetTimerForNextTick([&TimerHandle, TimeDilationSubsystem, TimerDelegate, Duration, CurrentCounter]()
				{
					ExecuteRealTimeTicker(TimeDilationSubsystem, TimerHandle, TimerDelegate, Duration, CurrentCounter);
				});
			}
		}

		void StopRealTimeTicker(FTimerHandle& TimerHandle, UWorld* World)
		{
			if (World)
			{
				World->GetTimerManager().ClearTimer(TimerHandle);
			}
		}
	}
}

void UAstroTimeDilationSubsystem::SetAbsoluteTimer(FTimerHandle& TimerHandle, const FTimerDelegate& TimerDelegate, float Duration)
{
	UWorld* World = GetWorld();
	AstroTimeDilationSubsystemUtils::Private::ExecuteRealTimeTicker(this, TimerHandle, TimerDelegate, Duration, 0.f);
}

void UAstroTimeDilationSubsystem::ClearAbsoluteTimer(FTimerHandle& TimerHandle)
{
	UWorld* World = GetWorld();
	AstroTimeDilationSubsystemUtils::Private::StopRealTimeTicker(TimerHandle, World);
}

void UAstroTimeDilationSubsystem::RegisterIgnoreTimeDilation(AActor* Actor)
{
	if (Actor && !IgnoreTimeDilationActors.Contains(Actor))
	{
		IgnoreTimeDilationActors.Add(Actor);
		DispatchUpdateIgnoredObjectsTimeDilation();
	}
}

void UAstroTimeDilationSubsystem::UnregisterIgnoreTimeDilation(AActor* Actor)
{
	if (Actor && IgnoreTimeDilationActors.Contains(Actor) && !RemovedIgnoreTimeDilationActors.Contains(Actor))
	{
		RemovedIgnoreTimeDilationActors.Add(Actor);
		DispatchUpdateIgnoredObjectsTimeDilation();
	}
}

void UAstroTimeDilationSubsystem::RegisterIgnoreTimeDilationParticle(UNiagaraComponent* ParticleComponent)
{
	if (ParticleComponent && !IgnoreTimeDilationParticles.Contains(ParticleComponent))
	{
		IgnoreTimeDilationParticles.Add(ParticleComponent);
		DispatchUpdateIgnoredObjectsTimeDilation();
	}
}

void UAstroTimeDilationSubsystem::UnregisterIgnoreTimeDilationParticle(UNiagaraComponent* ParticleComponent)
{
	if (ParticleComponent && IgnoreTimeDilationParticles.Contains(ParticleComponent) && !RemovedIgnoreTimeDilationParticles.Contains(ParticleComponent))
	{
		RemovedIgnoreTimeDilationParticles.Add(ParticleComponent);
		DispatchUpdateIgnoredObjectsTimeDilation();
	}
}

void UAstroTimeDilationSubsystem::SetGlobalTimeDilation(float TimeDilation, const bool bSmooth, const float CustomTimeDilationIncrementStep)
{
	const float CurrentTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	if (TimeDilation != CurrentTimeDilation)
	{
		// If bSmooth is disabled, we'll use a huge increment step to ensure that time dilation will reach its target in a single increment.
		constexpr float MaxTimeDilationIncrementStep = 99999.f;
		TargetTimeDilationValue = FTargetTimeDilationValue();
		TargetTimeDilationValue->TargetTimeDilation = TimeDilation;
		TargetTimeDilationValue->CustomTimeDilationIncrementStep = bSmooth ? CustomTimeDilationIncrementStep : MaxTimeDilationIncrementStep;

		DispatchUpdateIgnoredObjectsTimeDilation();
	}
}

float UAstroTimeDilationSubsystem::GetRealTimeSeconds() const
{
	return CurrentRealTimeSeconds;
}

float UAstroTimeDilationSubsystem::GetRealTimeDeltaSeconds() const
{
	return CurrentRealTimeSeconds - PreviousRealTimeSeconds;
}

float UAstroTimeDilationSubsystem::GetDilatedTime(UObject* WorldContextObject, float Duration)
{
	return Duration * UGameplayStatics::GetGlobalTimeDilation(WorldContextObject);
}

float UAstroTimeDilationSubsystem::GetUndilatedTime(UObject* WorldContextObject, float Duration)
{
	return Duration / UGameplayStatics::GetGlobalTimeDilation(WorldContextObject);
}

float UAstroTimeDilationSubsystem::GetGlobalTimeDilation(UObject* WorldContextObject)
{
	return UGameplayStatics::GetGlobalTimeDilation(WorldContextObject);
}

float UAstroTimeDilationSubsystem::GetGlobalTimeDilationInverse(UObject* WorldContextObject)
{
	return 1.f / UGameplayStatics::GetGlobalTimeDilation(WorldContextObject);
}

template <typename T>
struct FIgnoreTimeDilationImpl
{
	static void Ignore(T*)
	{
		check(false);
	}

	static void RemoveIgnore(T*)
	{
		check(false);
	}
};

template <>
struct FIgnoreTimeDilationImpl<AActor>
{
	static void Ignore(AActor* Actor)
	{
		if (Actor)
		{
			Actor->CustomTimeDilation = UAstroTimeDilationSubsystem::GetGlobalTimeDilationInverse(Actor);
		}
	}

	static void RemoveIgnore(AActor* Actor)
	{
		if (Actor)
		{
			Actor->CustomTimeDilation = 1.f;
		}
	}
};

template <>
struct FIgnoreTimeDilationImpl<UNiagaraComponent>
{
	static void Ignore(UNiagaraComponent* ParticleComponent)
	{
		if (ParticleComponent)
		{
			ParticleComponent->SetCustomTimeDilation(UAstroTimeDilationSubsystem::GetGlobalTimeDilationInverse(ParticleComponent));
		}
	}

	static void RemoveIgnore(UNiagaraComponent* ParticleComponent)
	{
		if (ParticleComponent)
		{
			ParticleComponent->SetCustomTimeDilation(1.f);
		}
	}
};

void UAstroTimeDilationSubsystem::UpdateIgnoredObjectsTimeDilation()
{
	UpdateIgnoredObjectsTimeDilationImpl(IgnoreTimeDilationActors, RemovedIgnoreTimeDilationActors);
	UpdateIgnoredObjectsTimeDilationImpl(IgnoreTimeDilationParticles, RemovedIgnoreTimeDilationParticles);
}

template <typename T>
void UAstroTimeDilationSubsystem::UpdateIgnoredObjectsTimeDilationImpl(TArray<TWeakObjectPtr<T>>& IgnoreTimeDilationObjects, TArray<TWeakObjectPtr<T>>& RemovedIgnoreTimeDilationObjects)
{
	for (const TWeakObjectPtr<T>& RemovedIgnoreTimeDilationObject : RemovedIgnoreTimeDilationObjects)
	{
		if (!RemovedIgnoreTimeDilationObject.IsValid())
		{
			continue;
		}

		FIgnoreTimeDilationImpl<T>::RemoveIgnore(RemovedIgnoreTimeDilationObject.Get());
		IgnoreTimeDilationObjects.Remove(RemovedIgnoreTimeDilationObject);
	}
	RemovedIgnoreTimeDilationObjects.Empty();

	for (const TWeakObjectPtr<T>& IgnoreTimeDilationObject : IgnoreTimeDilationObjects)
	{
		if (!IgnoreTimeDilationObject.IsValid())
		{
			continue;
		}

		FIgnoreTimeDilationImpl<T>::Ignore(IgnoreTimeDilationObject.Get());
	}
}

void UAstroTimeDilationSubsystem::DispatchUpdateIgnoredObjectsTimeDilation()
{
	if (AstroTimeDilationSubsystemVars::bEnableUpdateIgnoredCallsDispatch)
	{
		bAreTimeDilationIgnoredActorsDirty = true;
		return;
	}

	UpdateIgnoredObjectsTimeDilation();
}
