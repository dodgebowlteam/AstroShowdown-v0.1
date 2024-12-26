/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "Engine/DataAsset.h"
#include "InstancedStruct.h"
#include "ISMPartition/ISMComponentDescriptor.h"
#include "AstroTilesetData.generated.h"

// TODO (#cleanup): Make this editor-only

UENUM(BlueprintType)
enum class EAstroTilesetFillPolicy : uint8
{
	Greedy,			// Always picks the largest mesh to use
	Alternate,		// Will alternate between all meshes in order
};

UCLASS(BlueprintType)
class UAstroTilesetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

#pragma region UPrimaryDataAsset
public:
	UAstroTilesetData();
#pragma endregion

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<TInstancedStruct<FISMComponentDescriptor>> BaseMeshDescriptors;

	UPROPERTY(EditDefaultsOnly)
	TInstancedStruct<FISMComponentDescriptor> CornerMeshDescriptor;

	UPROPERTY(EditDefaultsOnly)
	EAstroTilesetFillPolicy FillPolicy = EAstroTilesetFillPolicy::Greedy;

	UPROPERTY(EditDefaultsOnly)
	uint8 bIgnoreProxyMeshes : 1 = false;

};