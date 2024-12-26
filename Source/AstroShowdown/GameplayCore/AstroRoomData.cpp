/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroRoomData.h"
#include "GameFeatureAction.h"

UAstroRoomData::UAstroRoomData()
{
}

void UAstroRoomData::PostLoad()
{
	Super::PostLoad();

	if (const bool bIsZeroId = !RoomId.IsValid())
	{
		RoomId = FGuid::NewGuid();
	}
}

#if WITH_EDITOR
EDataValidationResult UAstroRoomData::IsDataValid(FDataValidationContext& Context) const
{
	return EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
void UAstroRoomData::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	for (UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			Action->AddAdditionalAssetBundleData(AssetBundleData);
		}
	}
}
#endif // WITH_EDITORONLY_DATA
