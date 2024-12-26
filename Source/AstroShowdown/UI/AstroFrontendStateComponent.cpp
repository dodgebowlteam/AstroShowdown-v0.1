/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroFrontendStateComponent.h"
#include "AstroGameContextManagerComponent.h"
#include "AstroGameplayTags.h"
#include "CommonGameInstance.h"
#include "ControlFlowManager.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AstroFrontendStateComponent)

UAstroFrontendStateComponent::UAstroFrontendStateComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroFrontendStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the game context load to complete
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	UAstroGameContextManagerComponent* GameContextComponent = GameState->FindComponentByClass<UAstroGameContextManagerComponent>();
	check(GameContextComponent);

	// This delegate is on a component with the same lifetime as this one, so no need to unhook it in 
	GameContextComponent->CallOrRegister_OnGameContextLoaded_HighPriority(FOnAstroGameContextLoaded::FDelegate::CreateUObject(this, &UAstroFrontendStateComponent::OnGameContextLoaded));
}

void UAstroFrontendStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool UAstroFrontendStateComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (bShouldShowLoadingScreen)
	{
		OutReason = TEXT("Frontend Flow Pending...");

		if (FrontEndFlow.IsValid())
		{
			const TOptional<FString> StepDebugName = FrontEndFlow->GetCurrentStepDebugName();
			if (StepDebugName.IsSet())
			{
				OutReason = StepDebugName.GetValue();
			}
		}

		return true;
	}

	return false;
}

void UAstroFrontendStateComponent::SetShowLoadingScreen(const bool bShow)
{
	bShouldShowLoadingScreen = bShow;
}

void UAstroFrontendStateComponent::OnGameContextLoaded(const UAstroGameContextData* Experience)
{
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("FrontendFlow"))
		.QueueStep(TEXT("Try Show Sponsors Splash Screen"), this, &UAstroFrontendStateComponent::FlowStep_TryShowSponsorsSplashScreen)
		.QueueStep(TEXT("Try Show Team Splash Screen"), this, &UAstroFrontendStateComponent::FlowStep_TryShowTeamSplashScreenClass)
		.QueueStep(TEXT("Try Show Main Menu Screen"), this, &UAstroFrontendStateComponent::FlowStep_TryShowMainMenuScreen);

	Flow.ExecuteFlow();

	FrontEndFlow = Flow.AsShared();
}

void UAstroFrontendStateComponent::FlowStep_TryShowSponsorsSplashScreen(FControlFlowNodeRef SubFlow)
{
	// Stops showing the loading screen
	bShouldShowLoadingScreen = false;

	// Adds the Sponsors Splash Screen, and moves to the next flow when it deactivates.
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
	{
		auto HandleSponsorsScreenStateChange = [this, SubFlow](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen)
		{
			switch (State)
			{
			case EAsyncWidgetLayerState::AfterPush:
				bShouldShowLoadingScreen = false;
				Screen->OnDeactivated().AddWeakLambda(this, [this, SubFlow]() {
					SubFlow->ContinueFlow();
				});
				break;
			case EAsyncWidgetLayerState::Canceled:
				bShouldShowLoadingScreen = false;
				SubFlow->ContinueFlow();
				return;
			}
		};

		constexpr bool bSuspendInputUntilComplete = true;
		RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Menu, bSuspendInputUntilComplete, SponsorsSplashScreenClass, HandleSponsorsScreenStateChange);
	}
}

void UAstroFrontendStateComponent::FlowStep_TryShowTeamSplashScreenClass(FControlFlowNodeRef SubFlow)
{
	// Adds the Team Splash Screen, and moves to the next flow when it deactivates.
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
	{
		auto HandleTeamScreenStateChange = [this, SubFlow](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen)
		{
			switch (State)
			{
			case EAsyncWidgetLayerState::AfterPush:
				bShouldShowLoadingScreen = false;
				Screen->OnDeactivated().AddWeakLambda(this, [this, SubFlow]()
				{
					SubFlow->ContinueFlow();
				});
				break;
			case EAsyncWidgetLayerState::Canceled:
				bShouldShowLoadingScreen = false;
				SubFlow->ContinueFlow();
				return;
			}
		};

		constexpr bool bSuspendInputUntilComplete = true;
		RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Menu, bSuspendInputUntilComplete, TeamSplashScreenClass, HandleTeamScreenStateChange);
	}
}

void UAstroFrontendStateComponent::FlowStep_TryShowMainMenuScreen(FControlFlowNodeRef SubFlow)
{
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
	{
		auto HandleMainMenuStateChange = [this, SubFlow](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen)
		{
			switch (State)
			{
			case EAsyncWidgetLayerState::AfterPush:
				bShouldShowLoadingScreen = false;
				SubFlow->ContinueFlow();
				return;
			case EAsyncWidgetLayerState::Canceled:
				bShouldShowLoadingScreen = false;
				SubFlow->ContinueFlow();
				return;
			}
		};

		constexpr bool bSuspendInputUntilComplete = true;
		RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Menu, bSuspendInputUntilComplete, MainMenuScreenClass, HandleMainMenuStateChange);
	}
}

