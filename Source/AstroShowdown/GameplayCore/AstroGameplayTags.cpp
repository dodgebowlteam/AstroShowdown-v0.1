/*
* Copyright (c) 2024 DodgeBowl Team
* Licensed under the Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
*
* Full license terms: https://creativecommons.org/licenses/by-nc/4.0/
* This file is part of Astro Showdown, and is intended for educational and non-commercial use only.
*/

#include "AstroGameplayTags.h"

#include "Engine/EngineTypes.h"
#include "GameplayTagsManager.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstroGameplayTags, Log, All);
DEFINE_LOG_CATEGORY(LogAstroGameplayTags);

namespace AstroGameplayTags
{
	/* Ability tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayAbility_BulletTime, "GameplayAbility.BulletTime", "Tag for the Bullet Time GA.");

	/* Gameplay tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Damage_Invulnerable, "Gameplay.Damage.Invulnerable", "When applied, denies all damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Stamina_Infinite, "Gameplay.Stamina.Infinite", "When applied, prevents stamina consumption.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Team_Ally, "Gameplay.Team.Ally", "Used by player-allied units.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Team_Enemy, "Gameplay.Team.Enemy", "Used by anti-player units.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Team_Neutral, "Gameplay.Team.Neutral", "Used by neutral units.");
	
	/* Gameplay mission tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Mission_Start, "Gameplay.Mission.Start", "Signals mission start events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Mission_Success, "Gameplay.Mission.Success", "Signals mission success events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Mission_Fail, "Gameplay.Mission.Fail", "Signals mission fail events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Mission_Stop, "Gameplay.Mission.Stop", "Signals mission stop event.");

	/* Gameplay mission tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Game_RequestInitialize, "Gameplay.Message.Game.RequestInitialize", "Signals a game initialization request.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Interstitial_Start, "Gameplay.Message.Interstitial.Start", "Signals interstitial start event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Interstitial_End, "Gameplay.Message.Interstitial.End", "Signals interstitial end event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Room_Enter, "Gameplay.Message.Room.Enter", "Signals when the player enters a given room.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Section_Enter, "Gameplay.Message.Section.Enter", "Signals when the player enters a given section.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_NPC_Revive, "Gameplay.Message.NPC.Revive", "Signals that an NPC has just revived.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_PracticeMode_Start, "Gameplay.Message.PracticeMode.Start", "Signals when the player enters the practice mode.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Indicator_Register, "Gameplay.Message.Indicator.Register", "Signals an indicator register request.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_Message_Indicator_Unregister, "Gameplay.Message.Indicator.Unregister", "Signals indicator unregister request.");

	/* Input tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag, "InputTag", "Generic input tag.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move_Released, "InputTag.Move.Released", "Input tag for when the move input is released.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Focus, "InputTag.Focus", "Focus state input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_CancelFocus, "InputTag.CancelFocus", "Cancel focus state input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AstroDash, "InputTag.AstroDash", "Astro Dash ability input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AstroThrow, "InputTag.AstroThrow", "Astro Throw ability input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AstroThrow_Release, "InputTag.AstroThrow.Release", "Astro Throw ability input (input release).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AstroThrow_Cancel, "InputTag.AstroThrow.Cancel", "Astro Throw ability input (input cancel).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact", "Interact input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_ExitPracticeMode, "InputTag.ExitPracticeMode", "Practice mode exit input.");

	/* State tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AstroState, "AstroState", "Generic gameplay tag for Astro States.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AstroState_Win, "AstroState.Win", "Gameplay tag for the player's win state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AstroState_Lose, "AstroState.Lose", "Gameplay tag for the player's lose state.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AstroState_Focus, "AstroState.Focus", "Gameplay tag for when the player is focused.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AstroState_Fatigued, "AstroState.Fatigued", "Gameplay tag for when the player is fatigued.");

	/* Gameplay event tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_AstroDashPayload, "GameplayEvent.AstroDash", "Astro Dash gameplay event tag. We're using this to send data to the Astro Dash GA.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_AstroThrowPayload, "GameplayEvent.AstroThrow", "Astro Throw gameplay event tag. We're using this to send data to the Astro Throw GA.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_GenericStateDataEvent, "GameplayEvent.GenericStateDataEvent", "Generic gameplay event to receive data inside the player's state machine.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Generic_AnimNotify, "GameplayEvent.Generic.AnimNotify", "Generic gameplay event used by animation notifies to communicate with GAs.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_GenericMelee_AnimNotify, "GameplayEvent.GenericMelee.AnimNotify", "Generic gameplay event used by animation notifies to communicate with melee GAs.");

	/* Surface types */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SurfaceType_Grass, "SurfaceType.Grass", "Tag that represents the grass surface.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SurfaceType_Metal, "SurfaceType.Metal", "Tag that represents the metal surface.");

	/* UI tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game, "UI.Layer.Game", "Represents the game layer of a base layout.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu, "UI.Layer.GameMenu", "Represents the game menu layer of a base layout.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu, "UI.Layer.Menu", "Represents the menu layer of a base layout.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal, "UI.Layer.Modal", "Represents the modal layer of a base layout.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Action_Pause, "UI.Action.Pause", "Pause menu action input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Action_Restart, "UI.Action.Restart", "Restart action input.");

	/* SetByCaller tags */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_StaminaCost, "SetByCaller.StaminaCost", "Tag used to pass data to the stamina cost GE, through a SetByCaller attribute.");

	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (!Tag.IsValid() && bMatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				if (TestTag.ToString().Contains(TagString))
				{
					UE_LOG(LogAstroGameplayTags, Display, TEXT("Could not find exact match for tag [%s] but found partial match on tag [%s]."), *TagString, *TestTag.ToString());
					Tag = TestTag;
					break;
				}
			}
		}

		return Tag;
	}
}

