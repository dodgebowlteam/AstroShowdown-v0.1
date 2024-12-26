/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Editor/UnrealEdEngine.h"
#include "AstroEditorEngine.generated.h"

class IEngineLoop;
class UObject;

UCLASS()
class UAstroEditorEngine : public UUnrealEdEngine
{
	GENERATED_BODY()

public:
	UAstroEditorEngine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void Init(IEngineLoop* InEngineLoop) override;
	virtual void Start() override;
	virtual void Tick(float DeltaSeconds, bool bIdleMode) override;

	virtual FGameInstancePIEResult PreCreatePIEInstances(const bool bAnyBlueprintErrors, const bool bStartInSpectatorMode, const float PIEStartTime, const bool bSupportsOnlinePIE, int32& InNumOnlinePIEInstances) override;

};
