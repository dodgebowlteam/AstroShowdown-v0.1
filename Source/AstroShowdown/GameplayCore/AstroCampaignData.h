/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/DataAsset.h"
#include "AstroCampaignData.generated.h"

class UAstroSectionData;

UCLASS(BlueprintType, Const)
class UAstroCampaignData : public UPrimaryDataAsset
{
	GENERATED_BODY()

#pragma region UPrimaryDataAsset
public:
	UAstroCampaignData();

	static const UAstroCampaignData* Get();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
#pragma endregion


public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<TObjectPtr<UAstroSectionData>> Sections;
};