/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "AstroIndicatorTypes.generated.h"

class AActor;
class UAstroIndicatorWidget;
class UTexture2D;

USTRUCT(BlueprintType)
struct FAstroIndicatorWidgetSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient)
	TWeakObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadWrite, Transient)
	TSoftClassPtr<UAstroIndicatorWidget> IndicatorWidgetClassOverride = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FVector OwnerPositionOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FName SocketName;

	/** When true, will supress error messages if the socket is not found. */
	UPROPERTY(BlueprintReadWrite)
	uint8 bOptionalSocketName : 1 = false;

	UPROPERTY(BlueprintReadWrite)
	uint8 bUseEnemyIndicator : 1 = false;

	UPROPERTY(BlueprintReadWrite)
	uint8 bOffscreenActorOnly : 1 = true;

	UPROPERTY(BlueprintReadWrite)
	uint8 bShouldCheckAliveState : 1 = true;
};

USTRUCT(BlueprintType)
struct FAstroIndicatorRegisterRequestMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FAstroIndicatorWidgetSettings IndicatorSettings;
};

USTRUCT(BlueprintType)
struct FAstroIndicatorUnregisterRequestMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Transient)
	TWeakObjectPtr<AActor> Owner = nullptr;
};
