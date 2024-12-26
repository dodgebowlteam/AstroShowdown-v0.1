/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "AstroIndicatorTypes.h"
#include "Components/GameStateComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "AstroIndicatorWidgetManagerComponent.generated.h"

class UAstroIndicatorWidget;
class UAstroTimeDilationSubsystem;
class UCanvasPanel;
class UHealthAttributeSet;
class UMeshComponent;


UCLASS()
class UAstroIndicatorWidgetManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

#pragma region UGameStateComponent
public:
	UAstroIndicatorWidgetManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#pragma endregion


public:
	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UAstroIndicatorWidget> IndicatorWidgetClass;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UAstroIndicatorWidget> EnemyIndicatorWidgetClass;

private:
	/** Keeps track of all indicators in the world. We'll use this to update their positions and visibility. */
	struct FNPCIndicatorData
	{
		TWeakObjectPtr<AActor> Owner = nullptr;
		TWeakObjectPtr<UAstroIndicatorWidget> Widget = nullptr;
		TWeakObjectPtr<const UHealthAttributeSet> CachedOwnerHealthAttributeSet = nullptr;
		TWeakObjectPtr<const UMeshComponent> CachedOwnerMeshComponent = nullptr;
		FVector2D PreviousPosition = FVector2D::ZeroVector;
		FVector OwnerPositionOffset = FVector::ZeroVector;
		FName SocketName;
		uint8 bDead : 1 = false;
		uint8 bOffscreenActorOnly : 1 = true;

		bool operator==(const FNPCIndicatorData& Other)
		{
			return Owner == Other.Owner && Widget == Other.Widget;
		}
	};
	TArray<FNPCIndicatorData> Indicators;

	UPROPERTY(Transient)
	TWeakObjectPtr<UAstroTimeDilationSubsystem> CachedTimeDilationSubsystem = nullptr;

	UPROPERTY(Transient)
	UCanvasPanel* IndicatorRoot = nullptr;

	FGameplayMessageListenerHandle RegisterRequestMessageHandle;
	FGameplayMessageListenerHandle UnregisterRequestMessageHandle;

public:
	void RegisterActorIndicator(const FAstroIndicatorWidgetSettings& IndicatorSettings);
	void UnregisterActorIndicator(AActor* Actor);

	UFUNCTION(BlueprintCallable)
	void SetIndicatorRoot(UCanvasPanel* InRoot) { IndicatorRoot = InRoot; }

private:
	void CreateIndicatorFor(const FAstroIndicatorWidgetSettings& IndicatorSettings);
	void RemoveIndicatorAt(const int32 IndicatorIndex);
	void RemoveAllIndicators();

private:
	UFUNCTION()
	void OnIndicatorSpawned(UUserWidget* NewWidget, const FAstroIndicatorWidgetSettings IndicatorSettings);

	UFUNCTION()
	void OnIndicatorIconLoaded(UTexture2D* IndicatorIcon, UAstroIndicatorWidget* Widget);

	UFUNCTION()
	void OnGameResolved();

	UFUNCTION()
	void OnIndicatorRegisterRequestReceived(const FGameplayTag ChannelTag, const FAstroIndicatorRegisterRequestMessage& RegisterRequestMessage);

	UFUNCTION()
	void OnIndicatorUnregisterRequestReceived(const FGameplayTag ChannelTag, const FAstroIndicatorUnregisterRequestMessage& UnregisterRequestMessage);

};