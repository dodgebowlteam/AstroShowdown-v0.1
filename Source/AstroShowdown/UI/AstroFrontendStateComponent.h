/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Components/GameStateComponent.h"
#include "ControlFlowNode.h"
#include "Engine/SoftWorldReference.h"
#include "LoadingProcessInterface.h"
#include "AstroFrontendStateComponent.generated.h"

class FControlFlow;
class FString;
class FText;
class UObject;
struct FFrame;

class UCommonActivatableWidget;
class UAstroGameContextData;

UCLASS(Abstract)
class UAstroFrontendStateComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroFrontendStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion


#pragma region ILoadingProcessInterface
public:
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
#pragma endregion


public:
	/** Forces the loading/transition screen to show up or hide. */
	UFUNCTION(BlueprintCallable)
	void SetShowLoadingScreen(const bool bShow);

private:
	void OnGameContextLoaded(const UAstroGameContextData* GameContext);

	void FlowStep_TryShowSponsorsSplashScreen(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowTeamSplashScreenClass(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowMainMenuScreen(FControlFlowNodeRef SubFlow);

	bool bShouldShowLoadingScreen = true;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> SponsorsSplashScreenClass;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> TeamSplashScreenClass;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> MainMenuScreenClass;

	TSharedPtr<FControlFlow> FrontEndFlow;

};
