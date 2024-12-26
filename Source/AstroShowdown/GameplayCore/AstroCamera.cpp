/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroCamera.h"
#include "AstroTimeDilationSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "SubsystemUtils.h"

AAstroCamera::AAstroCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;		// Lazily enables tick

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>("CameraRoot");
	SetRootComponent(SceneRootComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("CameraSpringArm");
	SpringArmComponent->TargetArmLength = 2000.f;
	SpringArmComponent->bDoCollisionTest = false;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 3.f;
	SpringArmComponent->SetUsingAbsoluteRotation(true);
	SpringArmComponent->SetWorldRotation(FRotator(-40.f, 45.f, 0.f));		// Sets up isometric rotation
	SpringArmComponent->AttachToComponent(SceneRootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera");
	CameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	CameraComponent->OrthoWidth = 3500.f;
	CameraComponent->bUpdateOrthoPlanes = true;				// Required to shadow properly, bUseCameraHeightAsViewTarget is disabled without it
	CameraComponent->bAutoCalculateOrthoPlanes = false;		// Required to shadow properly, CameraLag doesn't work without it
	CameraComponent->bUseCameraHeightAsViewTarget = true;	// Required to shadow properly
	CameraComponent->OrthoNearClipPlane = 0.f;
	CameraComponent->bConstrainAspectRatio = true;
	CameraComponent->AttachToComponent(SpringArmComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

void AAstroCamera::BeginPlay()
{
	Super::BeginPlay();

	if (CameraComponent)
	{
		TargetOrthoWidth = CameraComponent->OrthoWidth;
	}

	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		TimeDilationSubsystem->RegisterIgnoreTimeDilation(this);
	}

	RecalculateSpringArmLength();
}

void AAstroCamera::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UAstroTimeDilationSubsystem* TimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this))
	{
		TimeDilationSubsystem->UnregisterIgnoreTimeDilation(this);
	}
}

void AAstroCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Snaps to the target's position
	if (CameraTarget.IsValid())
	{
		SetActorLocation(CameraTarget->GetActorLocation(), false /*bSweep*/, nullptr /*OutSweepHitResult*/, ETeleportType::TeleportPhysics);
	}

	// Animates to TargetOrthoWidth
	if (CameraComponent && CameraComponent->OrthoWidth != TargetOrthoWidth)
	{
		const float CorrectedDeltaTime = DeltaTime * UAstroTimeDilationSubsystem::GetGlobalTimeDilationInverse(this);	// Ignores dilation
		const float OrthoWidthDelta = (TargetOrthoWidth - CameraComponent->OrthoWidth);
		const float OrthoWidthDeltaFrame = FMath::Sign(OrthoWidthDelta) * OrthoWidthInterpolationSpeed * CorrectedDeltaTime;
		const float NewOrthoWidth = FMath::Clamp(CameraComponent->OrthoWidth + OrthoWidthDeltaFrame, 0.f, TargetOrthoWidth);
		CameraComponent->SetOrthoWidth(NewOrthoWidth);
		RecalculateSpringArmLength();
	}
}

void AAstroCamera::SetCameraTarget(AActor* InCameraTarget)
{
	CameraTarget = InCameraTarget;

	if (InCameraTarget)
	{
		SetActorTickEnabled(true);
	}
}

void AAstroCamera::SetTargetOrthoWidth(const float InTargetOrthoWidth)
{
	TargetOrthoWidth = InTargetOrthoWidth;

	if (CameraComponent && InTargetOrthoWidth != CameraComponent->OrthoWidth)
	{
		SetActorTickEnabled(true);
	}
}

void AAstroCamera::RecalculateSpringArmLength()
{
	if (!CameraComponent || !SpringArmComponent)
	{
		return;
	}

	constexpr float SpringArmLengthRatio = 0.8f;		// Magic number that we use to calculate the spring arm length relative to the ortho width
	const float CurrentOrthoWidth = CameraComponent->OrthoWidth;
	const float NewSpringArmLength = CurrentOrthoWidth * SpringArmLengthRatio;
	SpringArmComponent->TargetArmLength = NewSpringArmLength;
}
