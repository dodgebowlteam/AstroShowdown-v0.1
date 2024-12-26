/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "GameFramework/Actor.h"
#include "AstroTilesetSpawner.generated.h"

class UAstroTilesetData;

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAstroTilesetCornerFilter : uint8
{
	None		= 0	UMETA(Hidden),
	IgnoreFirst		= 1 << 0,
	IgnoreLast		= 1 << 1,
	IgnoreMiddle	= 1 << 2,
	IgnoreAll		= IgnoreFirst | IgnoreLast | IgnoreMiddle,
};
ENUM_CLASS_FLAGS(EAstroTilesetCornerFilter);


USTRUCT(BlueprintType)
struct FAstroTilesetInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAstroTilesetData> TilesetData;

	UPROPERTY(EditAnywhere)
	FVector PositionOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere)
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** Specifies which corners should NOT spawn. */
	UPROPERTY(EditAnywhere, meta = (Bitmask, BitmaskEnum = "/Script/AstroShowdown.EAstroTilesetCornerFilter"))
	uint8 CornerFilter = 0;
};

UINTERFACE(MinimalAPI, BlueprintType)
class UAstroTilesetProxyInterface : public UInterface
{
	GENERATED_BODY()
};

class IAstroTilesetProxyInterface : public IInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void GetProxyMeshLength(float& Length) const;

};


class USplineComponent;

UCLASS(Blueprintable, BlueprintType)
class AAstroTilesetSpawner : public AActor
{
	GENERATED_BODY()


#pragma region AActor
public:
	AAstroTilesetSpawner();
protected:
	virtual void OnConstruction(const FTransform& Transform);
#pragma endregion

protected:
	/** Root for all components owned by this actor. */
	UPROPERTY(BlueprintReadOnly, Category = Components)
	TObjectPtr<USceneComponent> SceneRootComponent = nullptr;

	/** Root for all SMCs owned by this actor. */
	UPROPERTY(BlueprintReadOnly, Category = Components)
	TObjectPtr<USceneComponent> MeshContainerComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Components)
	TObjectPtr<USplineComponent> SplineComponent = nullptr;

protected:
	UPROPERTY(EditAnywhere, Category = Settings)
	TArray<FAstroTilesetInfo> Tilesets;

	/**
	* All objects of the given classes will used as proxy tileset meshes, meaning we'll treat them
	* as part of the tileset, and prevent spawning on top of them.
	*/
	UPROPERTY(EditAnywhere, Category = Settings)
	TArray<TSubclassOf<AActor>> ProxyMeshClasses;

	UPROPERTY(EditAnywhere, Category = Settings)
	uint8 bShouldFreezeGeneration : 1 = true;

	UPROPERTY(VisibleAnywhere)
	TArray<FBox> ProxyMeshBounds;


private:
	void Generate();
	void GenerateTileset(const FAstroTilesetInfo& InTileset);
	void ClearAllMeshes();
	void CacheProxyMeshBounds();

	/** @return True when @param DistanceAlongSpline is within any of the proxy mesh bounds. */
	bool IsSplinePointWithinProxyMeshBounds(const float DistanceAlongSpline, const TArray<FBox>& InProxyMeshBounds, OUT FBox& OutHitMask) const;

	/** Picks the largest available mesh that would fit the segment. Defaults to the first mesh if none would fit. */
	void PickGreedyMesh(const UAstroTilesetData* TilesetData, const float DistanceAlongSplineA, const float DistanceAlongSplineB, OUT int32& OutMeshDescriptorIndex);

	/** Picks mesh 0, then 1, then 2, ..., then back to 0. */
	void PickAlternatedMesh(const UAstroTilesetData* TilesetData, OUT int32& OutMeshDescriptorIndex);

};
