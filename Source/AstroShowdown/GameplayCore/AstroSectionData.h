#pragma once

#include "Engine/DataAsset.h"
#include "AstroSectionData.generated.h"

class UAstroRoomData;
class UMaterialInterface;

UCLASS(BlueprintType, Const)
class UAstroSectionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

#pragma region UPrimaryDataAsset
public:
	UAstroSectionData();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
#pragma endregion


public:
	/** Player-facing section name. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FText SectionDisplayName;

	/**
	* NOTE: For now, we're assuming that all levels are interconnected with north/south doors, so we're going
	* to treat this array as the level order for the section. If this ever changes, we'll need to implement
	* a map editor (maybe based on UBehaviorTreeGraph), and refactor this to consider the new system.
	*/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<TObjectPtr<UAstroRoomData>> Rooms;

	/** When the player enters this section, we'll use this to override the AstroDome's material */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> DomeMaterialOverride = nullptr;

};