/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroSectionData.h"
#include "AstroRoomData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "AstroSystem"

UAstroSectionData::UAstroSectionData()
{
}

#if WITH_EDITOR
EDataValidationResult UAstroSectionData::IsDataValid(FDataValidationContext& Context) const
{
	TSet<TObjectPtr<UAstroRoomData>> AddedRooms;
	for (TObjectPtr<UAstroRoomData> Room : Rooms)
	{
		if (!Room)
		{
			Context.AddError(LOCTEXT("SectionValidationNull", "Null Room"));
			return EDataValidationResult::Invalid;
		}

		if (AddedRooms.Contains(Room))
		{
			Context.AddError(FText::Format(LOCTEXT("SectionValidationDupe", "Duplicate Room: {0}"), FText::FromString(Room->GetFName().ToString())));
			return EDataValidationResult::Invalid;
		}

		AddedRooms.Add(Room);
	}

	return EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE