/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroTilesetSpawner.h"
#include "AstroRoomDoor.h"
#include "AstroTilesetData.h"
#include "AstroUtilitiesBlueprintLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


AAstroTilesetSpawner::AAstroTilesetSpawner() : Super()
{
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>("SceneRoot");
	MeshContainerComponent = CreateDefaultSubobject<USceneComponent>("MeshContainer");
	SplineComponent = CreateDefaultSubobject<USplineComponent>("Spline");

	SceneRootComponent->SetMobility(EComponentMobility::Static);
	SplineComponent->SetMobility(EComponentMobility::Static);
	MeshContainerComponent->SetMobility(EComponentMobility::Static);

	SetRootComponent(SceneRootComponent);
	MeshContainerComponent->AttachToComponent(SceneRootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	SplineComponent->AttachToComponent(SceneRootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// Filters doors by default
	ProxyMeshClasses =
	{
		AAstroRoomDoor::StaticClass()
	};
}

void AAstroTilesetSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Generate();
}

namespace AstroUtils
{
	namespace Private
	{
		bool IsInPlayMode()
		{
			if (GEngine)
			{
				for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
				{
					if (WorldContext.WorldType == EWorldType::Game || WorldContext.WorldType == EWorldType::PIE)
					{
						return true;
					}
				}
			}

			return false;
		}
	}
}

void AAstroTilesetSpawner::Generate()
{
	// Prevents mesh regeneration at runtime
	if (AstroUtils::Private::IsInPlayMode())
	{
		return;
	}

	if (bShouldFreezeGeneration || !SplineComponent)
	{
		return;
	}

	// Clears everything before starting
	ClearAllMeshes();
	CacheProxyMeshBounds();

	// Reorigins the spline
	SplineComponent->SetRelativeLocation(FVector::ZeroVector);

	// Generates all tilesets
	for (const FAstroTilesetInfo& Tileset : Tilesets)
	{
		GenerateTileset(Tileset);
	}
}

namespace AstroUtils
{
	namespace Private
	{
		float GetMeshLengthX(const TInstancedStruct<FISMComponentDescriptor>& InMeshDescriptor)
		{
			if (!InMeshDescriptor.IsValid())
			{
				return 0.f;
			}

			UStaticMesh* Mesh = InMeshDescriptor.GetPtr()->StaticMesh;
			if (!Mesh)
			{
				return 0.f;
			}

			const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
			return FMath::RoundToFloat(MeshBounds.BoxExtent.X * 2.f);
		}
	}
}

void AAstroTilesetSpawner::GenerateTileset(const FAstroTilesetInfo& InTileset)
{
	if (!SplineComponent)
	{
		return;
	}

	UAstroTilesetData* TilesetData = InTileset.TilesetData;
	if (!TilesetData || TilesetData->BaseMeshDescriptors.IsEmpty())
	{
		return;
	}

	using FAstroTilesetInfoBatcher = TMap<const FISMComponentDescriptor*, TArray<FTransform>>;
	FAstroTilesetInfoBatcher Batcher;

	int32 PickedMeshIndex = -1;		// NOTE: We keep this at an outer scope because we reuse it when calculating the alternate fill method
	const int32 SplineSegmentCount = SplineComponent->GetNumberOfSplineSegments();
	const int32 SplinePointCount = SplineComponent->GetNumberOfSplinePoints();
	TArray<FBox> LocalProxyMeshBounds = ProxyMeshBounds;
	for (int32 CurrentPointIndex = 0; CurrentPointIndex < SplineSegmentCount; CurrentPointIndex++)
	{
		const int32 NextPointIndex = CurrentPointIndex + 1;
		if (CurrentPointIndex >= SplinePointCount || NextPointIndex >= SplinePointCount)
		{
			break;
		}

		// Generates mesh descriptors + instances for the line segment using the fill policy
		const float SegmentStartDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(CurrentPointIndex);
		const float SegmentEndDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(NextPointIndex);
		float CurrentSegmentDistance = SegmentStartDistance;
		while (CurrentSegmentDistance < SegmentEndDistance)
		{
			switch (TilesetData->FillPolicy)
			{
			case EAstroTilesetFillPolicy::Greedy:
				PickGreedyMesh(TilesetData, CurrentSegmentDistance, SegmentEndDistance, OUT PickedMeshIndex);
				break;
			case EAstroTilesetFillPolicy::Alternate:
				PickAlternatedMesh(TilesetData, OUT PickedMeshIndex);
				break;
			}

			if (!TilesetData->BaseMeshDescriptors.IsValidIndex(PickedMeshIndex))
			{
				break;
			}

			const TInstancedStruct<FISMComponentDescriptor>& MeshDescriptor = TilesetData->BaseMeshDescriptors[PickedMeshIndex];
			const FISMComponentDescriptor* MeshDescriptorPtr = MeshDescriptor.IsValid() ? MeshDescriptor.GetPtr() : nullptr;
			if (!MeshDescriptorPtr)
			{
				break;
			}

			const float MeshLength = AstroUtils::Private::GetMeshLengthX(MeshDescriptor);
			if (MeshLength == 0.f)
			{
				break;
			}

			// If the mesh is intersecting with any of the proxy meshes, then don't spawn it
			const float TargetSegmentDistance = FMath::Clamp(CurrentSegmentDistance + MeshLength, SegmentStartDistance, SegmentEndDistance);
			const float HalfMeshSegment = (CurrentSegmentDistance + TargetSegmentDistance) / 2.f;
			FBox HitSamplingFilterMask = FBox(ForceInitToZero);
			if (!TilesetData->bIgnoreProxyMeshes && (
				IsSplinePointWithinProxyMeshBounds(CurrentSegmentDistance, LocalProxyMeshBounds, OUT HitSamplingFilterMask)
				|| IsSplinePointWithinProxyMeshBounds(HalfMeshSegment, LocalProxyMeshBounds, OUT HitSamplingFilterMask)))
			{
				const float ProxyBoundSize = HitSamplingFilterMask.GetExtent().X * 2.f;
				CurrentSegmentDistance += ProxyBoundSize;

				LocalProxyMeshBounds.Remove(HitSamplingFilterMask);
				continue;
			}

			// Spawns the mesh at halfway along the distance between the mesh segment start and end
			// NOTE: This assumes that the meshes are pivoted around their center.
			const float CurrentInputKey = SplineComponent->GetInputKeyValueAtDistanceAlongSpline(HalfMeshSegment);
			const FVector CurrentSplinePosition = SplineComponent->GetLocationAtSplineInputKey(CurrentInputKey, ESplineCoordinateSpace::World);
			const FRotator CurrentSplineRotation = SplineComponent->GetRotationAtDistanceAlongSpline(HalfMeshSegment, ESplineCoordinateSpace::World);

			FTransform MeshTransform;
			MeshTransform.SetTranslation(CurrentSplinePosition);
			MeshTransform.SetRotation((CurrentSplineRotation + InTileset.RotationOffset).Quaternion());
			MeshTransform.SetScale3D(FVector::OneVector);

			Batcher.FindOrAdd(MeshDescriptorPtr).Add(MeshTransform);

			CurrentSegmentDistance = TargetSegmentDistance;
		}
	}
	
	// Generates one mesh descriptor for the corner mesh on top of each vertex
	for (int32 CurrentPointIndex = 0; CurrentPointIndex < SplinePointCount; CurrentPointIndex++)
	{
		const FISMComponentDescriptor* CornerMeshDescriptorPtr = TilesetData->CornerMeshDescriptor.IsValid() ? TilesetData->CornerMeshDescriptor.GetPtr() : nullptr;
		if (!CornerMeshDescriptorPtr)
		{
			break;
		}

		const uint8 IgnoreFirstFlag = static_cast<uint8>(EAstroTilesetCornerFilter::IgnoreFirst);
		if (CurrentPointIndex == 0 && (InTileset.CornerFilter & IgnoreFirstFlag))
		{
			continue;
		}

		const uint8 IgnoreLastFlag = static_cast<uint8>(EAstroTilesetCornerFilter::IgnoreLast);
		if (CurrentPointIndex == SplinePointCount - 1 && (InTileset.CornerFilter & IgnoreLastFlag))
		{
			continue;
		}

		const uint8 IgnoreMiddleFlag = static_cast<uint8>(EAstroTilesetCornerFilter::IgnoreMiddle);
		if (CurrentPointIndex != 0 && CurrentPointIndex != SplinePointCount - 1 && (InTileset.CornerFilter & IgnoreMiddleFlag))
		{
			continue;
		}

		const float SplinePointDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(CurrentPointIndex);
		const float CurrentInputKey = SplineComponent->GetInputKeyValueAtDistanceAlongSpline(SplinePointDistance);
		const FVector CurrentSplinePosition = SplineComponent->GetLocationAtSplineInputKey(CurrentInputKey, ESplineCoordinateSpace::World);
		const FRotator CurrentSplineRotation = SplineComponent->GetRotationAtDistanceAlongSpline(SplinePointDistance, ESplineCoordinateSpace::World);

		FTransform MeshTransform;
		MeshTransform.SetTranslation(CurrentSplinePosition);
		MeshTransform.SetRotation((CurrentSplineRotation + InTileset.RotationOffset).Quaternion());
		MeshTransform.SetScale3D(FVector::OneVector);
		Batcher.FindOrAdd(CornerMeshDescriptorPtr).Add(MeshTransform);
	}

	// Spawns all mesh descriptors and their instances
	TSet<const FISMComponentDescriptor*> BatchedMeshes;
	Batcher.GetKeys(OUT BatchedMeshes);
	for (const FISMComponentDescriptor* MeshDescriptorPtr : BatchedMeshes)
	{
		if (!MeshDescriptorPtr)
		{
			continue;
		}

		const TArray<FTransform>& InstanceTransforms = Batcher[MeshDescriptorPtr];
		if (InstanceTransforms.IsEmpty())
		{
			continue;
		}

		// NOTE: We need to use this method to ensure that the component is serialized and shows up on the editor
		UInstancedStaticMeshComponent* MeshComponent = CastChecked<UInstancedStaticMeshComponent>(UAstroUtilitiesBlueprintLibrary::AddInstancedComponent(this, this, MeshDescriptorPtr->ComponentClass));
		MeshDescriptorPtr->InitComponent(MeshComponent);
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->AttachToComponent(MeshContainerComponent, FAttachmentTransformRules::KeepRelativeTransform);

		// Initializes all instances
		constexpr bool bReturnIndices = false;
		constexpr bool bWorldSpace = true;
		MeshComponent->AddInstances(InstanceTransforms, bReturnIndices, bWorldSpace);
	}
}

void AAstroTilesetSpawner::ClearAllMeshes()
{
	UAstroUtilitiesBlueprintLibrary::ClearAllChildrenComponents(this, MeshContainerComponent);
	UAstroUtilitiesBlueprintLibrary::ClearAllActorChildrenComponents(this, this, UStaticMeshComponent::StaticClass());
}

void AAstroTilesetSpawner::CacheProxyMeshBounds()
{
	ProxyMeshBounds.Empty();

	for (const TSubclassOf<AActor>& SamplingFilterClass : ProxyMeshClasses)
	{
		TArray<AActor*> ProxyMeshActors;
		UGameplayStatics::GetAllActorsOfClass(this, SamplingFilterClass, OUT ProxyMeshActors);

		for (AActor* ProxyMeshActor : ProxyMeshActors)
		{
			UClass* ProxyMeshActorClass = ProxyMeshActor ? ProxyMeshActor->GetClass() : nullptr;
			if (ProxyMeshActorClass && ProxyMeshActorClass->ImplementsInterface(UAstroTilesetProxyInterface::StaticClass()))
			{
				float ProxyMeshLength;
				IAstroTilesetProxyInterface::Execute_GetProxyMeshLength(ProxyMeshActor, OUT ProxyMeshLength);

				const float HalfProxyMeshLength = ProxyMeshLength / 2.f;
				const FVector ActorPosition = ProxyMeshActor->GetActorLocation();
				const FVector BoxExtents = FVector(HalfProxyMeshLength, HalfProxyMeshLength, ProxyMeshLength);
				const FBox ProxyBounds = UKismetMathLibrary::MakeBoxWithOrigin(ActorPosition, BoxExtents);
				ProxyMeshBounds.Add(ProxyBounds);
			}
		}
	}
}

bool AAstroTilesetSpawner::IsSplinePointWithinProxyMeshBounds(const float DistanceAlongSpline, const TArray<FBox>& InProxyMeshBounds, OUT FBox& OutHitMask) const
{
	if (!SplineComponent)
	{
		return false;
	}

	const FVector SplinePosition = SplineComponent->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);
	for (const FBox& ProxyMeshBox : InProxyMeshBounds)
	{
		if (FMath::PointBoxIntersection(SplinePosition, ProxyMeshBox))
		{
			OutHitMask = ProxyMeshBox;
			return true;
		}
	}

	return false;
}

void AAstroTilesetSpawner::PickGreedyMesh(const UAstroTilesetData* TilesetData, const float DistanceAlongSplineA, const float DistanceAlongSplineB, OUT int32& OutMeshDescriptorIndex)
{
	if (!TilesetData)
	{
		return;
	}

	ensure(TilesetData->BaseMeshDescriptors.Num() > 0);

	float CurrentMeshLength = -1.f;

	const float SegmentLength = DistanceAlongSplineB - DistanceAlongSplineA;
	const int32 BaseMeshCount = TilesetData->BaseMeshDescriptors.Num();
	for (int32 BaseMeshIndex = 0; BaseMeshIndex < BaseMeshCount; BaseMeshIndex++)
	{
		TInstancedStruct<FISMComponentDescriptor> CurrentMeshDescriptor = TilesetData->BaseMeshDescriptors[BaseMeshIndex];
		if (!CurrentMeshDescriptor.IsValid())
		{
			continue;
		}

		// Picks the largest mesh, or uses the first one as default if none would fit
		const float MeshSize = AstroUtils::Private::GetMeshLengthX(CurrentMeshDescriptor);
		if (BaseMeshIndex == 0 || (MeshSize > CurrentMeshLength && MeshSize <= SegmentLength) || (CurrentMeshLength > SegmentLength && MeshSize <= SegmentLength))
		{
			OutMeshDescriptorIndex = BaseMeshIndex;
			CurrentMeshLength = MeshSize;
			continue;
		}
	}
}

void AAstroTilesetSpawner::PickAlternatedMesh(const UAstroTilesetData* TilesetData, OUT int32& OutMeshDescriptorIndex)
{
	OutMeshDescriptorIndex = -1;

	if (TilesetData)
	{
		OutMeshDescriptorIndex = ((OutMeshDescriptorIndex + 1) % TilesetData->BaseMeshDescriptors.Num());
	}
}
