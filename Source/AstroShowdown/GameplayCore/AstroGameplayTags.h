/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#pragma once

#include "NativeGameplayTags.h"

/*
 * Declares all of the custom native tags that the game will use
 */
namespace AstroGameplayTags
{
	ASTROSHOWDOWN_API	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	/* Ability tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayAbility_BulletTime);

	/* Gameplay tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Damage_Invulnerable);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Stamina_Infinite);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Team_Ally);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Team_Enemy);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Team_Neutral);

	/* Gameplay Mission Tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Mission_Start);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Mission_Success);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Mission_Fail);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Mission_Stop);

	/* Gameplay messages */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Game_RequestInitialize);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Interstitial_Start);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Interstitial_End);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Room_Enter);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Section_Enter);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_NPC_Revive);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_PracticeMode_Start);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Indicator_Register);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_Message_Indicator_Unregister);

	/* Input tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move_Released);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Focus);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_CancelFocus);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_AstroDash);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_AstroThrow);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_AstroThrow_Release);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_AstroThrow_Cancel);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_ExitPracticeMode);

	/* State tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AstroState);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AstroState_Win);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AstroState_Lose);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AstroState_Focus);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AstroState_Fatigued);

	/* Gameplay event tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_AstroDashPayload);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_AstroThrowPayload);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_GenericStateDataEvent);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Generic_AnimNotify);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_GenericMelee_AnimNotify);

	/* Surface types */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SurfaceType_Grass);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SurfaceType_Metal);
	
	/* UI tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Pause);
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Restart);

	/* SetByCaller tags */
	ASTROSHOWDOWN_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_StaminaCost);


};
