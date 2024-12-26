/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroIndicatorWidgetManagerComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Actions/AsyncAction_CreateWidgetAsync.h"
#include "AstroGameplayTags.h"
#include "AstroGameState.h"
#include "AstroIndicatorWidget.h"
#include "AstroTimeDilationSubsystem.h"
#include "AsyncAction_LoadTexture.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/MeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HealthAttributeSet.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"

UAstroIndicatorWidgetManagerComponent::UAstroIndicatorWidgetManagerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAstroIndicatorWidgetManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedTimeDilationSubsystem = SubsystemUtils::GetWorldSubsystem<UAstroTimeDilationSubsystem>(this);

	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameResolved.AddDynamic(this, &UAstroIndicatorWidgetManagerComponent::OnGameResolved);
	}
	
	// Starts listening to indicator register requests
	{
		FGameplayMessageListenerParams<FAstroIndicatorRegisterRequestMessage> RegisterListenerParams;
		RegisterListenerParams.SetMessageReceivedCallback(this, &UAstroIndicatorWidgetManagerComponent::OnIndicatorRegisterRequestReceived);
		RegisterRequestMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_Indicator_Register, RegisterListenerParams);
	}

	// Starts listening to indicator unregister requests
	{
		FGameplayMessageListenerParams<FAstroIndicatorUnregisterRequestMessage> UnregisterListenerParams;
		UnregisterListenerParams.SetMessageReceivedCallback(this, &UAstroIndicatorWidgetManagerComponent::OnIndicatorUnregisterRequestReceived);
		UnregisterRequestMessageHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(AstroGameplayTags::Gameplay_Message_Indicator_Unregister, UnregisterListenerParams);
	}
}

void UAstroIndicatorWidgetManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	const UWorld* World = GetWorld();
	if (AAstroGameState* AstroGameState = World ? World->GetGameState<AAstroGameState>() : nullptr)
	{
		AstroGameState->OnAstroGameResolved.RemoveDynamic(this, &UAstroIndicatorWidgetManagerComponent::OnGameResolved);
	}

	RemoveAllIndicators();

	// Stops listening to indicator register/unregister requests
	UGameplayMessageSubsystem::Get(this).UnregisterListener(RegisterRequestMessageHandle);
	UGameplayMessageSubsystem::Get(this).UnregisterListener(UnregisterRequestMessageHandle);
}

void UAstroIndicatorWidgetManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Finds the local player controller. We'll need it to project indicator owners world locations to the viewport.
	APlayerController* LocalPlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController<APlayerController>() : nullptr;
	if (!LocalPlayerController)
	{
		return;
	}

	// Reverse iterates indicators, removing invalids, and updating the visibility of valids
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	for (int32 Index = Indicators.Num() - 1; Index >= 0; Index--)
	{
		FNPCIndicatorData& Indicator = Indicators[Index];
		if (!Indicator.Owner.IsValid() || !Indicator.Widget.IsValid() || Indicator.Owner->IsHidden())
		{
			RemoveIndicatorAt(Index);
			continue;
		}

		FVector IndicatorOwnerWorldPosition = Indicator.Owner->GetActorLocation() + Indicator.OwnerPositionOffset;

		// Overrides the owner's position with a socket's position if one was specified
		if (!Indicator.SocketName.IsNone())
		{
			if (Indicator.CachedOwnerMeshComponent.IsValid() && Indicator.CachedOwnerMeshComponent->DoesSocketExist(Indicator.SocketName))
			{
				IndicatorOwnerWorldPosition = Indicator.CachedOwnerMeshComponent->GetSocketLocation(Indicator.SocketName);
			}
		}

		FVector2D OwnerScreenLocation;
		constexpr bool bPlayerViewportRelative = true;		// Ensures that the calculation will work in all aspect ratios
		UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(LocalPlayerController, IndicatorOwnerWorldPosition, OUT OwnerScreenLocation, bPlayerViewportRelative);
		const FVector2D ClampedOwnerScreenLocation = FVector2D::Clamp(OwnerScreenLocation, FVector2D::ZeroVector, ViewportSize);

		// We consider the widget to be visible if the enemy's screen location is out of the [0, ViewportSize] bounds.
		// NOTE: We can't use Owner->WasRecentlyRendered here because the owner's LastRenderTime is being updated on the shadow pass
		// even when objects are offscreen. We should investigate this though, and revisit later.
		bool bIsWidgetVisible = true;
		const bool bIsOwnerBeingRendered = OwnerScreenLocation == ClampedOwnerScreenLocation;
		if (Indicator.bOffscreenActorOnly)
		{
			bIsWidgetVisible = !bIsOwnerBeingRendered;
		}

		const ESlateVisibility NewWidgetVisibility = bIsWidgetVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		Indicator.Widget->SetVisibility(NewWidgetVisibility);

		// If the indicator is visible, its position is updated
		if (bIsWidgetVisible)
		{
			const FVector2D ViewportPadding = Indicator.Widget->GetDesiredSize() * 0.5f;
			const FVector2D OriginPadding = ViewportPadding;
			const FVector2D PaddedViewportSize = ViewportSize - ViewportPadding;
			const FVector2D PaddedOwnerScreenLocation = FVector2D::Clamp(OwnerScreenLocation, OriginPadding, PaddedViewportSize);
			const FVector2D ToWidgetOrigin = (OwnerScreenLocation - PaddedOwnerScreenLocation);
			const FVector2D WidgetDirection = ToWidgetOrigin.GetSafeNormal();
			const UHealthAttributeSet* HealthAttributeSet = Indicator.CachedOwnerHealthAttributeSet.Get();

			const float Angle = FMath::Atan2(WidgetDirection.Y, WidgetDirection.X);
			const float Distance = ToWidgetOrigin.Length();
			const float ReviveProgress = HealthAttributeSet ? HealthAttributeSet->GetReviveCounter() / HealthAttributeSet->GetReviveDuration() : 0.f;
			const bool bNewDead = HealthAttributeSet ? HealthAttributeSet->GetCurrentHealth() == 0.f : false;
			Indicator.Widget->SetIndicatorAngle(bIsOwnerBeingRendered ? 0.f : FMath::RadiansToDegrees(Angle));
			Indicator.Widget->SetIndicatorDistance(Distance);
			Indicator.Widget->SetIndicatorReviveProgress(ReviveProgress);
			Indicator.Widget->SetIndicatorOwnerBeingRendered(bIsOwnerBeingRendered);

			// Only sets IndicatorDead if it's changed
			if (bNewDead != Indicator.bDead)
			{
				Indicator.Widget->SetIndicatorDead(bNewDead);
				Indicator.bDead = bNewDead;
			}

			// Updates the indicator's position
			constexpr float PositionUpdateDistSqrThreshold = 1.f;
			if (FVector2D::DistSquared(PaddedOwnerScreenLocation, Indicator.PreviousPosition) > PositionUpdateDistSqrThreshold)
			{
				// Assumes the widget is attached to a canvas panel
				if (UCanvasPanelSlot* PanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Indicator.Widget.Get()))
				{
					PanelSlot->SetPosition(PaddedOwnerScreenLocation);
					Indicator.PreviousPosition = PaddedOwnerScreenLocation;
				}
			}
		}
	}
}

void UAstroIndicatorWidgetManagerComponent::RegisterActorIndicator(const FAstroIndicatorWidgetSettings& IndicatorSettings)
{
	UnregisterActorIndicator(IndicatorSettings.Owner.Get());		// Prevents multiple indicators for the same actor
	CreateIndicatorFor(IndicatorSettings);
}

void UAstroIndicatorWidgetManagerComponent::UnregisterActorIndicator(AActor* Actor)
{
	for (int32 Index = Indicators.Num() - 1; Index >= 0; Index--)
	{
		FNPCIndicatorData& Indicator = Indicators[Index];
		if (Indicator.Owner.Get() == Actor)
		{
			RemoveIndicatorAt(Index);
			continue;
		}
	}
}

void UAstroIndicatorWidgetManagerComponent::CreateIndicatorFor(const FAstroIndicatorWidgetSettings& IndicatorSettings)
{	
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* LocalPlayerController = World->GetFirstPlayerController<APlayerController>())
			{
				// Checks which indicator to use
				TSoftClassPtr<UAstroIndicatorWidget> IndicatorClass = IndicatorSettings.bUseEnemyIndicator ? EnemyIndicatorWidgetClass : IndicatorWidgetClass;
				IndicatorClass = IndicatorSettings.IndicatorWidgetClassOverride.GetAssetName().IsEmpty() ? IndicatorClass : IndicatorSettings.IndicatorWidgetClassOverride;

				// Spawns the indicator widget asynchronously
				constexpr bool bSuspendInputUntilComplete = false;
				UAsyncAction_CreateWidgetAsync* CreateWidgetAsyncAction = UAsyncAction_CreateWidgetAsync::CreateWidgetAsync(this, IndicatorClass, LocalPlayerController, bSuspendInputUntilComplete);
				CreateWidgetAsyncAction->OnCompleteDelegate.AddUObject(this, &UAstroIndicatorWidgetManagerComponent::OnIndicatorSpawned, IndicatorSettings);
				CreateWidgetAsyncAction->Activate();
			}
		}
	}
}

void UAstroIndicatorWidgetManagerComponent::RemoveIndicatorAt(const int32 Index)
{
	if (Indicators.IsValidIndex(Index))
	{
		// Destroys the widget and removes it from Indicators
		Indicators[Index].Widget->SetVisibility(ESlateVisibility::Collapsed);
		Indicators[Index].Widget->RemoveFromParent();
		Indicators.RemoveAt(Index);
	}
}

void UAstroIndicatorWidgetManagerComponent::RemoveAllIndicators()
{
	while (!Indicators.IsEmpty())
	{
		RemoveIndicatorAt(0);
	}
}


namespace AstroUtils
{
	namespace Private
	{
		const UHealthAttributeSet* FindActorHealthAttributeSet(AActor* Actor)
		{
			if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor))
			{
				if (UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent())
				{
					if (const UHealthAttributeSet* HealthAttributeSet = Cast<UHealthAttributeSet>(ASC->GetAttributeSet(UHealthAttributeSet::StaticClass())))
					{
						return HealthAttributeSet;
					}
				}
			}

			return nullptr;
		}
	}
}

namespace AstroUtils
{
	namespace Private
	{
		static const UMeshComponent* FindMeshComponentWithSocket(AActor* Owner, const FName& SocketName)
		{
			if (SocketName.IsNone())
			{
				return nullptr;
			}

			const TSet<UActorComponent*> Components = Owner->GetComponents();
			for (TSet<UActorComponent*>::TConstIterator It = Components.CreateConstIterator(); It; ++It)
			{
				if (const UMeshComponent* MeshComponent = Cast<UMeshComponent>(*It))
				{
					if (MeshComponent->DoesSocketExist(SocketName))
					{
						return MeshComponent;
					}
				}
			}

			return nullptr;
		}
	}
}

void UAstroIndicatorWidgetManagerComponent::OnIndicatorSpawned(UUserWidget* NewWidget, const FAstroIndicatorWidgetSettings IndicatorSettings)
{
	if (!NewWidget || !IndicatorRoot || !IndicatorSettings.Owner.IsValid())
	{
		NewWidget->RemoveFromRoot();
		return;
	}

	NewWidget->SetVisibility(ESlateVisibility::Collapsed);
	IndicatorRoot->AddChild(NewWidget);

	FNPCIndicatorData NPCIndicatorData;
	NPCIndicatorData.Owner = IndicatorSettings.Owner;
	NPCIndicatorData.Widget = Cast<UAstroIndicatorWidget>(NewWidget);
	NPCIndicatorData.CachedOwnerHealthAttributeSet = IndicatorSettings.bShouldCheckAliveState ? AstroUtils::Private::FindActorHealthAttributeSet(IndicatorSettings.Owner.Get()) : nullptr;
	NPCIndicatorData.CachedOwnerMeshComponent = AstroUtils::Private::FindMeshComponentWithSocket(IndicatorSettings.Owner.Get(), IndicatorSettings.SocketName);
	NPCIndicatorData.OwnerPositionOffset = IndicatorSettings.OwnerPositionOffset;
	NPCIndicatorData.SocketName = IndicatorSettings.SocketName;
	NPCIndicatorData.bOffscreenActorOnly = IndicatorSettings.bOffscreenActorOnly;
	Indicators.Add(NPCIndicatorData);

	if (!IndicatorSettings.bOptionalSocketName && !NPCIndicatorData.SocketName.IsNone())
	{
		ensureMsgf(NPCIndicatorData.CachedOwnerMeshComponent.IsValid(), TEXT("Could not find MeshComponent with the specified SocketName"));
	}

	// Assigns indicator icon, OR loads it asynchronously if not loaded yet
	const TSoftObjectPtr<UTexture2D> IndicatorIcon = IndicatorSettings.Icon;
	if (IndicatorIcon.IsValid())
	{
		NPCIndicatorData.Widget->SetIndicatorIcon(IndicatorIcon.Get());
	}
	else if (const bool bHasIndicatorPath = !IndicatorIcon.GetAssetName().IsEmpty())
	{
		UAsyncAction_LoadTexture* LoadTextureAsyncAction = UAsyncAction_LoadTexture::LoadTexture(this, IndicatorIcon);
		LoadTextureAsyncAction->OnCompleteDelegate.AddUObject(this, &UAstroIndicatorWidgetManagerComponent::OnIndicatorIconLoaded, NPCIndicatorData.Widget.Get());
		LoadTextureAsyncAction->Activate();
	}
}

void UAstroIndicatorWidgetManagerComponent::OnIndicatorIconLoaded(UTexture2D* IndicatorIcon, UAstroIndicatorWidget* Widget)
{
	if (Widget && IndicatorIcon)
	{
		Widget->SetIndicatorIcon(IndicatorIcon);
	}
}

void UAstroIndicatorWidgetManagerComponent::OnGameResolved()
{
	RemoveAllIndicators();
}

void UAstroIndicatorWidgetManagerComponent::OnIndicatorRegisterRequestReceived(const FGameplayTag ChannelTag, const FAstroIndicatorRegisterRequestMessage& RegisterRequestMessage)
{
	RegisterActorIndicator(RegisterRequestMessage.IndicatorSettings);
}

void UAstroIndicatorWidgetManagerComponent::OnIndicatorUnregisterRequestReceived(const FGameplayTag ChannelTag, const FAstroIndicatorUnregisterRequestMessage& UnregisterRequestMessage)
{
	UnregisterActorIndicator(UnregisterRequestMessage.Owner.Get());
}
