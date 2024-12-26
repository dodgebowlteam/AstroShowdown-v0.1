/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroRoomNavigationComponent.h"
#include "AstroCampaignData.h"
#include "AstroCampaignDataSubsystem.h"
#include "AstroCampaignPersistenceSubsystem.h"
#include "AstroCoreDelegates.h"
#include "AstroGameplayTags.h"
#include "AstroInterstitialWidget.h"
#include "AstroRoomData.h"
#include "AstroRoomDoor.h"
#include "AstroRoomNavigationSubsystem.h"
#include "AstroRoomNavigationTypes.h"
#include "AstroSectionData.h"
#include "ControlFlowManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LevelBounds.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "EngineUtils.h"
#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "PrimaryGameLayout.h"
#include "SubsystemUtils.h"
#include "WorldPartition/WorldPartitionLevelStreamingDynamic.h"


DECLARE_LOG_CATEGORY_EXTERN(LogAstroLevelStreaming, Log, All);
DEFINE_LOG_CATEGORY(LogAstroLevelStreaming);

namespace AstroRoomNavigationVars
{
	static bool bForcePlayInterstitials = false;
	static FAutoConsoleVariableRef CVarForcePlayInterstitials(
		TEXT("RoomNavigation.ForcePlayInterstitials"),
		bForcePlayInterstitials,
		TEXT("When enabled, will forcibly play all interstitials."),
		ECVF_Default);
}


namespace AstroRoomNavigationUtils
{
	static FSoftWorldReference GetWorldAssetByLevelStreaming(const ULevelStreamingDynamic* DynamicLevelStreaming)
	{
		FSoftWorldReference SoftWorldReference;
		if (DynamicLevelStreaming)
		{
			if (const ULevelStreamingLevelInstance* LevelStreamingLevelInstance = Cast<ULevelStreamingLevelInstance>(DynamicLevelStreaming))
			{
				if (const ILevelInstanceInterface* LevelInstance = LevelStreamingLevelInstance->GetLevelInstance())
				{
					SoftWorldReference.WorldAsset = LevelInstance->GetWorldAsset();
				}
			}
			else
			{
				const FString LevelAssetName = FPackageName::GetShortName(DynamicLevelStreaming->PackageNameToLoad);
				const FString SubPathString = "";
				SoftWorldReference.WorldAsset = FSoftObjectPath(DynamicLevelStreaming->PackageNameToLoad, FName(LevelAssetName), SubPathString);
			}
		}

		return SoftWorldReference;
	}

	static UAstroRoomData* GetNeighborRoomByDirection(const UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData& RoomNavigationData, const EAstroRoomDoorDirection DoorDirection)
	{
		switch (DoorDirection)
		{
		case EAstroRoomDoorDirection::South:
			return RoomNavigationData.SouthNeighbor;
		case EAstroRoomDoorDirection::North:
			return RoomNavigationData.NorthNeighbor;
		}

		ensureMsgf(false, TEXT("This direction is not supported yet."));
		return nullptr;
	}
}

UAstroRoomNavigationComponent::UAstroRoomNavigationComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAstroRoomNavigationComponent::BeginPlay()
{
	Super::BeginPlay();

	FAstroCoreDelegates::OnPreRestartCurrentLevel.AddUObject(this, &UAstroRoomNavigationComponent::OnPreRestartCurrentLevel);

	LoadStartingLevel();
}

void UAstroRoomNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UE_LOG(LogAstroLevelStreaming, Display, TEXT("[%hs] AstroRoomNavigationComponent destroyed."), __FUNCTION__);

	FWorldDelegates::LevelAddedToWorld.Remove(WaitForMainLevelLoadDelegateHandle);
	FAstroCoreDelegates::OnPreRestartCurrentLevel.RemoveAll(this);
}

bool UAstroRoomNavigationComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (bShouldShowLoadingScreen)
	{
		OutReason = TEXT("Loading Room...");
		return true;
	}

	return false;
}

void UAstroRoomNavigationComponent::MoveTo(const FSoftWorldReference& TargetWorld, float InTransitionDurationOverride/* = -1.f*/)
{
	static FRoomLoadFlowStepSharedState RoomLoadFlowStepSharedState;

	if (RoomWorldLoadFlow.IsValid() && RoomWorldLoadFlow->IsRunning())
	{
		ensureAlwaysMsgf(false, TEXT("Invalid MoveTo. We're already trying to load another Room."));
		return;
	}

	if (TargetWorld.WorldAsset.GetLongPackageName().IsEmpty())
	{
		UE_LOG(LogAstroLevelStreaming, Error, TEXT("[%hs] Invalid TargetWorld."), __FUNCTION__);
		return;
	}

	// Triggers the loading screen
	bShouldShowLoadingScreen = true;

	// Resets the shared load flow state
	RoomLoadFlowStepSharedState = FRoomLoadFlowStepSharedState();

	// Starts the MoveTo flow
	const float TransitionDuration = InTransitionDurationOverride < 0.f ? MoveToTransitionDuration : InTransitionDurationOverride;
	FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("RoomWorldLoadFlow"));
	Flow.QueueDelay(TransitionDuration, "Wait For Transition");
	Flow.QueueWait(TEXT("Unload Room")).BindUObject(this, &UAstroRoomNavigationComponent::RoomLoadFlowStep_UnloadCurrentRoom, &RoomLoadFlowStepSharedState);
	Flow.QueueWait(TEXT("Play Interstitial Screen")).BindUObject(this, &UAstroRoomNavigationComponent::RoomLoadFlowStep_PlayInterstitialScreen, &RoomLoadFlowStepSharedState, TargetWorld);
	Flow.QueueWait(TEXT("Start Loading Room")).BindUObject(this, &UAstroRoomNavigationComponent::RoomLoadFlowStep_StartLoadingRoom, &RoomLoadFlowStepSharedState, TargetWorld);
	Flow.QueueWait(TEXT("Wait For Room With Content To Load")).BindUObject(this, &UAstroRoomNavigationComponent::RoomLoadFlowStep_WaitForMainLevelLoad, &RoomLoadFlowStepSharedState);
	Flow.QueueWait(TEXT("Process Room Entry Points")).BindUObject(this, &UAstroRoomNavigationComponent::RoomLoadFlowStep_ProcessMainLevel, &RoomLoadFlowStepSharedState);
	Flow.QueueWait(TEXT("Post-Process Loaded Room")).BindUObject(this, &UAstroRoomNavigationComponent::RoomLoadFlowStep_FinishMainLevelLoad, &RoomLoadFlowStepSharedState);

	Flow.ExecuteFlow();

	RoomWorldLoadFlow = Flow.AsShared();
}

void UAstroRoomNavigationComponent::MoveToDirection(const EAstroRoomDoorDirection RoomDoorDirection)
{
	const FSoftWorldReference CurrentRoomWorld = GetCurrentRoomWorldAsset();

	const UAstroRoomNavigationSubsystem* RoomNavigationSubsystem = UAstroRoomNavigationSubsystem::Get(this);
	const UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData RoomNavigationData = RoomNavigationSubsystem ? RoomNavigationSubsystem->GetRoomRuntimeNavigationData(CurrentRoomWorld) : UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData();

	const UAstroRoomData* RoomInDirectionData = AstroRoomNavigationUtils::GetNeighborRoomByDirection(RoomNavigationData, RoomDoorDirection);
	const FSoftWorldReference TargetWorld = RoomInDirectionData ? RoomInDirectionData->RoomLevel : RootLevel;

	UE_CLOG(!RoomInDirectionData, LogAstroLevelStreaming, Warning, TEXT("[%hs] Invalid move direction. Defaulting to RootLevel."), __FUNCTION__);

	MoveTo(TargetWorld);
}

void UAstroRoomNavigationComponent::RoomLoadFlowStep_UnloadCurrentRoom(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState)
{
	// Unloads the current room
	const FSoftWorldReference UnloadedRoomWorldAsset = GetCurrentRoomWorldAsset();
	UnloadCurrentLevel();

	// Stops the character's movement while waiting for the room world to load.
	// Prevent the character from falling when the current level is unloaded.
	// NOTE: This is reverted in OnRoomLevelAdded, once we find an entry point for the room.
	const APlayerController* FirstController = UGameplayStatics::GetPlayerController(this, 0);
	TWeakObjectPtr<ACharacter> PlayerCharacter = FirstController ? Cast<ACharacter>(FirstController->GetPawn()) : nullptr;
	UCharacterMovementComponent* MovementComponent = PlayerCharacter.IsValid() ? PlayerCharacter->GetCharacterMovement() : nullptr;
	if (ensureMsgf(MovementComponent, TEXT("Failed to find player character.")))
	{
		MovementComponent->SetMovementMode(EMovementMode::MOVE_None);
		MovementComponent->SetComponentTickEnabled(false);
	}

	// Caches the instigator and previous room to the shared state
	// Caches the room that's being unloaded
	if (SharedLoadFlowState)
	{
		SharedLoadFlowState->PreviousRoomWorld = UnloadedRoomWorldAsset;
		SharedLoadFlowState->Instigator = PlayerCharacter;
	}

	// Triggers the loading screen
	bShouldShowLoadingScreen = true;

	// Moves on with the flow
	SubFlow->ContinueFlow();
}

void UAstroRoomNavigationComponent::RoomLoadFlowStep_PlayInterstitialScreen(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState, const FSoftWorldReference TargetWorld)
{
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroCampaignDataSubsystem>(this);
	const UAstroCampaignPersistenceSubsystem* CampaignPersistenceSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroCampaignPersistenceSubsystem>(this);
	if (!CampaignDataSubsystem || !CampaignPersistenceSubsystem)
	{
		SubFlow->ContinueFlow();
		return;
	}

	// Finds the target room's data, and checks for intro interstitial screens
	const UAstroRoomData* TargetRoomData = CampaignDataSubsystem->GetRoomDataByWorld(TargetWorld);
	if (!TargetRoomData || TargetRoomData->IntroInterstitialScreen.GetAssetName().IsEmpty())
	{
		SubFlow->ContinueFlow();
		return;
	}

	// Checks if it's already been visited before.
	const bool bWasAlreadyVisited = CampaignPersistenceSubsystem->WasRoomVisited(TargetRoomData);
	if (!AstroRoomNavigationVars::bForcePlayInterstitials && bWasAlreadyVisited)
	{
		SubFlow->ContinueFlow();
		return;
	}

	// If the room's not been visited yet, then we play its interstitial screen and wait for it to complete.
	UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this);
	if (!RootLayout)
	{
		SubFlow->ContinueFlow();
		return;
	}

	auto HandleInterstitialScreenState = [this, SubFlowPtr = SubFlow.ToSharedPtr()](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen)
	{
		switch (State)
		{
		case EAsyncWidgetLayerState::AfterPush:
			bShouldShowLoadingScreen = false;

			if (UAstroInterstitialWidget* InterstitialScreen = CastChecked<UAstroInterstitialWidget>(Screen))
			{
				InterstitialScreen->OnInterstitialEndEvent.AddWeakLambda(this, [this, SubFlowPtr]()
				{
					if (SubFlowPtr.IsValid())
					{
						SubFlowPtr->ContinueFlow();
					}
				});
			}

			break;
		case EAsyncWidgetLayerState::Canceled:
			if (SubFlowPtr.IsValid())
			{
				SubFlowPtr->ContinueFlow();
			}
			break;
		}
	};

	constexpr bool bSuspendInputUntilComplete = true;
	RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(AstroGameplayTags::UI_Layer_Menu, bSuspendInputUntilComplete, TargetRoomData->IntroInterstitialScreen, HandleInterstitialScreenState);
}

void UAstroRoomNavigationComponent::RoomLoadFlowStep_StartLoadingRoom(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState, const FSoftWorldReference TargetWorld)
{
	if (TargetWorld.WorldAsset.GetLongPackageName().IsEmpty())
	{
		ensureMsgf(false, TEXT("Room load failed"));
		SubFlow->CancelFlow();
		return;
	}

	// NOTE: This is async, so we have to wait until the level is loaded AND shown before moving the player
	bool bSuccess = false;
	ULevelStreamingDynamic* NextRoomWorldStreaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, TargetWorld.WorldAsset, FTransform::Identity, bSuccess);

	if (ensure(bSuccess))
	{
		bShouldShowLoadingScreen = bSuccess;		// Triggers the loading screen
		SharedLoadFlowState->NextRoomWorldStreaming = NextRoomWorldStreaming;
		SubFlow->ContinueFlow();
		return;
	}

	ensureMsgf(false, TEXT("Room load failed"));
	SubFlow->CancelFlow();
}

void UAstroRoomNavigationComponent::RoomLoadFlowStep_WaitForMainLevelLoad(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState)
{
	UWorld* World = GetWorld();
	if (!World || !ensure(SharedLoadFlowState->NextRoomWorldStreaming.IsValid()))
	{
		return;
	}

	// If the world was already loaded, we can call the OnRoomLevelAdded event directly.
	// Otherwise, we need to wait until WP Subsystem loads each Level in the World (check UWorldPartitionLevelStreamingDynamic::IssueLoadRequests)
	if (SharedLoadFlowState->NextRoomWorldStreaming->HasLoadedLevel())
	{
		SubFlow->ContinueFlow();
	}
	else
	{
		FWorldDelegates::LevelAddedToWorld.Remove(WaitForMainLevelLoadDelegateHandle);

		TWeakObjectPtr<UAstroRoomNavigationComponent> WeakThis = this;
		TSharedPtr<FControlFlowNode> CurrentSubFlow = SubFlow;
		WaitForMainLevelLoadDelegateHandle = FWorldDelegates::LevelAddedToWorld.AddLambda([WeakThis, CurrentSubFlow](ULevel* NewLevel, UWorld* InWorld)
		{
			if (!WeakThis->RoomWorldLoadFlow.IsValid())
			{
				ensureMsgf(false, TEXT("Invalid RoomWorldLoadFlow object"));
				return;
			}

			if (!CurrentSubFlow.IsValid())
			{
				ensureMsgf(false, TEXT("Invalid SubFlow object"));
				return;
			}

			if (!WeakThis.IsValid() || !IsValid(WeakThis.Get()))
			{
				ensureMsgf(false, TEXT("Invalid AstroRoomNavigationComponent"));
				return;
			}

			TArray<AAstroRoomDoor*> OutDoors;
			TArray<APlayerStart*> OutPlayerStarts;
			if (WeakThis->FindLevelEntryPoints(NewLevel, OUT OutDoors, OUT OutPlayerStarts))
			{
				CurrentSubFlow->ContinueFlow();
			}
		});
	}
}

void UAstroRoomNavigationComponent::RoomLoadFlowStep_ProcessMainLevel(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState)
{
	FWorldDelegates::LevelAddedToWorld.Remove(WaitForMainLevelLoadDelegateHandle);		// Stops listening to room level loads

	const UWorld* World = GetWorld();
	const ULevel* RoomLevel = World ? World->GetLevel(World->GetNumLevels() - 1) : nullptr;	// Assumes the last level will contain the actual room props
	const UAstroRoomNavigationSubsystem* RoomNavigationSubsystem = UAstroRoomNavigationSubsystem::Get(this);
	if (!World || !RoomLevel || !RoomNavigationSubsystem || !SharedLoadFlowState->NextRoomWorldStreaming.IsValid() || !SharedLoadFlowState->Instigator.IsValid())
	{
		return;
	}

	TArray<AAstroRoomDoor*> OutDoors;
	TArray<APlayerStart*> OutPlayerStarts;
	if (!FindLevelEntryPoints(RoomLevel, OUT OutDoors, OUT OutPlayerStarts))
	{
		SubFlow->CancelFlow();
		return;
	}

	// Activates the current section
	ActivateSection();

	// Activates all actions for the current room
	ActivateRoom();

	// Sets up the navigation direction for each door
	AAstroRoomDoor* EntryDoor = nullptr;
	AAstroRoomDoor* UnlockedDoor = nullptr;
	TMap<EAstroRoomDoorDirection, AAstroRoomDoor*> RoomDoorsByDirection;
	if (OutDoors.Num() > 0)
	{
		const ALevelBounds* LevelBoundsActor = RoomLevel ? RoomLevel->LevelBoundsActor.Get() : nullptr;
		const FVector LevelBoundsCenter = LevelBoundsActor ? LevelBoundsActor->GetActorLocation() : FVector::ZeroVector;
		ensureAlwaysMsgf(LevelBoundsActor, TEXT("ALevelBounds not found on this level, please add one."));

		// Finds neighbor rooms, which we'll use to assign the doors to the rooms they lead to
		const FSoftWorldReference CurrentRoomWorld = AstroRoomNavigationUtils::GetWorldAssetByLevelStreaming(SharedLoadFlowState->NextRoomWorldStreaming.Get());
		const UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData RoomNavigationData = RoomNavigationSubsystem ? RoomNavigationSubsystem->GetRoomRuntimeNavigationData(CurrentRoomWorld) : UAstroRoomNavigationSubsystem::FAstroRoomRuntimeNavigationData();
		const UAstroCampaignPersistenceSubsystem* CampaignPersistenceSubsystem = SubsystemUtils::GetGameInstanceSubsystem<UAstroCampaignPersistenceSubsystem>(this);
		for (AAstroRoomDoor* RoomDoor : OutDoors)
		{
			const bool bIsSouthDoor = RoomDoor->GetActorLocation().X <= LevelBoundsCenter.X;
			const EAstroRoomDoorDirection NewDoorDirection = bIsSouthDoor ? EAstroRoomDoorDirection::South : EAstroRoomDoorDirection::North;

			// Sets the door's direction
			ensureAlwaysMsgf(!RoomDoorsByDirection.Contains(NewDoorDirection), TEXT("Door setup is wrong"));
			RoomDoorsByDirection.Add(NewDoorDirection, RoomDoor);
			RoomDoor->SetDoorDirection(NewDoorDirection);

			// Sets the room which the door leads to
			const UAstroRoomData* RoomInDirectionData = AstroRoomNavigationUtils::GetNeighborRoomByDirection(RoomNavigationData, NewDoorDirection);
			RoomDoor->SetNeighborRoomWorld(RoomInDirectionData ? RoomInDirectionData->RoomLevel : FSoftWorldReference());

			// Caches the door if we entered through it, and uses it as an entry point for the player character
			if (const bool bEnteredThroughThisDoor = RoomInDirectionData && SharedLoadFlowState->PreviousRoomWorld.WorldAsset == RoomInDirectionData->RoomLevel.WorldAsset)
			{
				EntryDoor = RoomDoor;
			}

			// Uses any unlocked door as the fallback entry door, in case no entry is found
			if (CampaignPersistenceSubsystem && CampaignPersistenceSubsystem->IsRoomUnlocked(RoomInDirectionData))
			{
				UnlockedDoor = RoomDoor;
			}
		}
	}

	// Teleports the character to one of the entry points, ordered by priority:
	// 1. Door that leads to the player's previous room;
	// 2. Any of the unlocked doors (useful for LevelSelection);
	// 3. Any of the entry points (useful for first level);
	// 4. Any door at all (fallback);
	if (EntryDoor)
	{
		EntryDoor->OnEnterRoom(SharedLoadFlowState->Instigator.Get());
		UE_LOG(LogAstroLevelStreaming, Verbose, TEXT("[%hs] Door found (%s)."), __FUNCTION__, *EntryDoor->GetFName().ToString());
	}
	else if (UnlockedDoor)
	{
		UnlockedDoor->OnEnterRoom(SharedLoadFlowState->Instigator.Get());
		UE_LOG(LogAstroLevelStreaming, Verbose, TEXT("[%hs] Fallback door found (%s)."), __FUNCTION__, *EntryDoor->GetFName().ToString());
	}
	else if (OutPlayerStarts.Num() > 0)
	{
		const APlayerStart* PlayerStartActor = OutPlayerStarts[0];
		SharedLoadFlowState->Instigator->TeleportTo(PlayerStartActor->GetActorLocation(), PlayerStartActor->GetActorRotation());
		UE_LOG(LogAstroLevelStreaming, Verbose, TEXT("[%hs] Could not find a door. Entering through player start."), __FUNCTION__);
	}
	else if (OutDoors.IsValidIndex(0) && OutDoors[0])
	{
		OutDoors[0]->OnEnterRoom(SharedLoadFlowState->Instigator.Get());
		UE_LOG(LogAstroLevelStreaming, Warning, TEXT("[%hs] Could not find any valid entry. Entering through whatever door."), __FUNCTION__);
	}
	else
	{
		ensureMsgf(false, TEXT("[%hs] Could not find an entry point to the level."));
	}

	// Restores the character movement taken away in ProcessRoomWorldLoaded
	if (UCharacterMovementComponent* MovementComponent = SharedLoadFlowState->Instigator->GetCharacterMovement(); ensureMsgf(MovementComponent, TEXT("Failed to find player character.")))
	{
		MovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
		MovementComponent->SetComponentTickEnabled(true);
	}

	// Stops the loading screen
	bShouldShowLoadingScreen = false;

	SubFlow->ContinueFlow();
}

void UAstroRoomNavigationComponent::RoomLoadFlowStep_FinishMainLevelLoad(FControlFlowNodeRef SubFlow, FRoomLoadFlowStepSharedState* SharedLoadFlowState)
{
	const FSoftWorldReference CurrentRoomWorld = GetCurrentRoomWorldAsset();
	OnRoomLoaded.Broadcast(CurrentRoomWorld);

	SubFlow->ContinueFlow();
}

void UAstroRoomNavigationComponent::LoadStartingLevel()
{
	FSoftWorldReference TargetWorld = RootLevel;

	// If a RootLevelOverride is specified, we'll load it instead.
	UAstroRoomNavigationSubsystem* RoomNavigationSubsystem = UAstroRoomNavigationSubsystem::Get(this);
	const FSoftWorldReference RootLevelOverride = RoomNavigationSubsystem ? RoomNavigationSubsystem->GetRootLevelOverride() : FSoftWorldReference();
	if (!RootLevelOverride.WorldAsset.GetLongPackageName().IsEmpty())
	{
		TargetWorld = RootLevelOverride;
		RoomNavigationSubsystem->SetRootLevelOverride({});
	}
	// If a level is already loaded, we'll load it instead of RootLevel. This is useful on PIE,
	// in case the user has manually placed a level in the map.
	else if (ULevelStreamingDynamic* LoadedRoom = FindCurrentRoomWorldStreaming())
	{
		TargetWorld = GetCurrentRoomWorldAsset();
	}

	constexpr float TransitionDuration = 0.f;
	MoveTo(TargetWorld, TransitionDuration);
}

void UAstroRoomNavigationComponent::UnloadCurrentLevel()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Deactivates all GFAs for the currently loaded room
	DeactivateRoom();

	// We want to handle LevelInstances separately because there's a chance a level
	// may have been manually placed on the scene for testing as a LevelInstanceActor, and in this case, we want
	// to make sure it's properly unloaded by running it through the LevelInstance unload flow.
	ULevelStreamingDynamic* RoomWorldStreaming = FindCurrentRoomWorldStreaming();
	if (ULevelStreamingLevelInstance* LevelStreamingLevelInstance = Cast<ULevelStreamingLevelInstance>(RoomWorldStreaming))
	{
		if (ILevelInstanceInterface* LevelInstanceActor = LevelStreamingLevelInstance->GetLevelInstance())
		{
			LevelInstanceActor->UnloadLevelInstance();
		}
	}
	else if (ULevelStreamingDynamic* DynamicStreamingLevel = Cast<ULevelStreamingDynamic>(RoomWorldStreaming))
	{
		if (const TSoftObjectPtr<UWorld>& Level = DynamicStreamingLevel->GetWorldAsset(); !Level.GetLongPackageName().IsEmpty())
		{
			DynamicStreamingLevel->SetIsRequestingUnloadAndRemoval(true);		// Forces removal from World::StreamingLevels when unloading

			FLatentActionInfo LatentInfo;
			constexpr bool bShouldBlockOnUnload = false;
			UGameplayStatics::UnloadStreamLevelBySoftObjectPtr(this, Level, LatentInfo, bShouldBlockOnUnload);
		}
	}
}

ULevelStreamingDynamic* UAstroRoomNavigationComponent::FindCurrentRoomWorldStreaming() const
{
	uint32 RoomWorldCount = 0;
	ULevelStreamingDynamic* FoundRoomWorld = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
		{
			if (const bool bIsUnloaded = !StreamingLevel->IsLevelLoaded() || StreamingLevel->GetLevelStreamingState() == ELevelStreamingState::MakingInvisible)
			{
				continue;
			}

			if (ULevelStreamingLevelInstance* LevelStreamingLevelInstance = Cast<ULevelStreamingLevelInstance>(StreamingLevel))
			{
				FoundRoomWorld = LevelStreamingLevelInstance;
				RoomWorldCount++;
				continue;
			}

			if (ULevelStreamingDynamic* DynamicStreamingLevel = Cast<ULevelStreamingDynamic>(StreamingLevel))
			{
				// Skips WP streaming
				if (DynamicStreamingLevel->IsA<UWorldPartitionLevelStreamingDynamic>())
				{
					continue;
				}

				FoundRoomWorld = DynamicStreamingLevel;
				RoomWorldCount++;
				continue;
			}
		}
	}

	ensureAlwaysMsgf(RoomWorldCount <= 1, TEXT("We're breaking the assumption that there's only dynamic level per world. Please, review this logic."));
	return FoundRoomWorld;
}

bool UAstroRoomNavigationComponent::FindLevelEntryPoints(const ULevel* InLevel, OUT TArray<AAstroRoomDoor*>& OutDoors, OUT TArray<APlayerStart*>& OutPlayerStarts)
{
	OutDoors.Empty();
	OutPlayerStarts.Empty();
	if (InLevel)
	{
		for (AActor* Actor : InLevel->Actors)
		{
			if (AAstroRoomDoor* DoorActor = Cast<AAstroRoomDoor>(Actor))
			{
				OutDoors.Add(DoorActor);
			}
			if (APlayerStart* PlayerStartActor = Cast<APlayerStart>(Actor))
			{
				OutPlayerStarts.Add(PlayerStartActor);
			}
		}
	}

	return OutDoors.Num() > 0 || OutPlayerStarts.Num() > 0;
}

void UAstroRoomNavigationComponent::ActivateSection()
{
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = UAstroCampaignDataSubsystem::Get(this);
	if (!CampaignDataSubsystem)
	{
		return;
	}

	const FSoftWorldReference CurrentRoomWorld = GetCurrentRoomWorldAsset();
	const UAstroRoomData* CurrentRoom = CampaignDataSubsystem->GetRoomDataByWorld(CurrentRoomWorld);
	UAstroSectionData* CurrentSection = CampaignDataSubsystem->GetSectionDataByRoomId(CurrentRoom ? CurrentRoom->RoomId : FGuid());
	if (!CurrentSection)
	{
		return;
	}

	// Broadcasts section enter message
	FAstroSectionGenericMessage EnterSectionPayload;
	EnterSectionPayload.Section = CurrentSection;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_Section_Enter, EnterSectionPayload);
}

void UAstroRoomNavigationComponent::ActivateRoom()
{
	const FSoftWorldReference CurrentRoomWorld = GetCurrentRoomWorldAsset();
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = UAstroCampaignDataSubsystem::Get(this);
	const UAstroRoomData* CurrentRoom = CampaignDataSubsystem ? CampaignDataSubsystem->GetRoomDataByWorld(CurrentRoomWorld) : nullptr;
	if (!CurrentRoom)
	{
		return;
	}

	// Broadcasts room enter message
	FAstroRoomGenericMessage EnterRoomPayload;
	EnterRoomPayload.Room = CurrentRoom;
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(AstroGameplayTags::Gameplay_Message_Room_Enter, EnterRoomPayload);

	// Activates all room actions
	FGameFeatureActivatingContext ActivationContext;
	if (const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld()))
	{
		ActivationContext.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}

	for (TObjectPtr<UGameFeatureAction> RoomAction : CurrentRoom->Actions)
	{
		if (RoomAction)
		{
			RoomAction->OnGameFeatureRegistering();
			RoomAction->OnGameFeatureLoading();
			RoomAction->OnGameFeatureActivating(ActivationContext);
		}
	}
}

void UAstroRoomNavigationComponent::DeactivateRoom()
{
	const FSoftWorldReference CurrentRoomWorld = GetCurrentRoomWorldAsset();
	const UAstroCampaignDataSubsystem* CampaignDataSubsystem = UAstroCampaignDataSubsystem::Get(this);
	const UAstroRoomData* CurrentRoom = CampaignDataSubsystem ? CampaignDataSubsystem->GetRoomDataByWorld(CurrentRoomWorld) : nullptr;
	if (!CurrentRoom)
	{
		return;
	}

	auto DeactivationCompletionCallback = [](FStringView InPauserTag) {};
	FGameFeatureDeactivatingContext DeactivationContext(TEXT(""), DeactivationCompletionCallback);
	if (const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld()))
	{
		DeactivationContext.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}

	for (TObjectPtr<UGameFeatureAction> RoomAction : CurrentRoom->Actions)
	{
		if (RoomAction)
		{
			RoomAction->OnGameFeatureDeactivating(DeactivationContext);
			RoomAction->OnGameFeatureUnregistering();
		}
	}
}

FSoftWorldReference UAstroRoomNavigationComponent::GetCurrentRoomWorldAsset() const
{
	FSoftWorldReference CurrentRoomWorldAsset;
	if (const ULevelStreamingDynamic* CurrentRoomLevelStreaming = FindCurrentRoomWorldStreaming())
	{
		CurrentRoomWorldAsset = AstroRoomNavigationUtils::GetWorldAssetByLevelStreaming(CurrentRoomLevelStreaming);
	}

	return CurrentRoomWorldAsset;
}

void UAstroRoomNavigationComponent::OnPreRestartCurrentLevel(bool& bShouldDeferRestart)
{
	// Deactivates all GFAs for the currently loaded room
	DeactivateRoom();

	// Sets the root level override to the current loaded room
	bShouldDeferRestart = true;
	SetShowLoadingScreen(true);

	const FSoftWorldReference LastLoadedRoom = GetCurrentRoomWorldAsset();
	if (!LastLoadedRoom.WorldAsset.GetLongPackageName().IsEmpty())
	{
		if (UAstroRoomNavigationSubsystem* RoomNavigationSubsystem = UAstroRoomNavigationSubsystem::Get(this))
		{
			RoomNavigationSubsystem->SetRootLevelOverride(LastLoadedRoom);
		}
	}
}