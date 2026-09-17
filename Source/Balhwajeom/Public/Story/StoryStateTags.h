#pragma once

#include "NativeGameplayTags.h"


namespace BalhwajeomGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Player_Mode);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Player_Mode_Exploration);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Player_Mode_PhotoCamera);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Player_Mode_Tablet);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Lock);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Lock_PhotoCamera);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Runtime_Lock_Tablet);

	/**
	 * Moments the tutorial overlays wait for. Native rather than ini-registered because
	 * only C++ ever adds them, so there is no second place for the spelling to drift.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_GameplayStarted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_InteractPromptShown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_InteractCompleted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_PhotoCaptureCompleted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_TabletOpened);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_TabletClosed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tutorial_Trigger_SisterFolderOpened);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Chapter_01_Phase_01_Completed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Chapter_01_Phase_02_Unlocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Chapter_01_Phase_02_Completed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Chapter_01_Phase_03_Unlocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Chapter_01_Phase_03_Completed);
}
