#include "Story/StoryStateTags.h"


namespace BalhwajeomGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Player_Mode,
		"Runtime.Player.Mode",
		"Root for mutually exclusive local player runtime modes."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Player_Mode_Exploration,
		"Runtime.Player.Mode.Exploration",
		"Normal third-person exploration mode."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Player_Mode_PhotoCamera,
		"Runtime.Player.Mode.PhotoCamera",
		"First-person evidence photo camera mode."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Player_Mode_Tablet,
		"Runtime.Player.Mode.Tablet",
		"Tablet UI mode."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Lock,
		"Runtime.Lock",
		"Root for runtime ability locks. A lock tag being PRESENT blocks the ability, "
		"so levels that never add one keep every ability available with no configuration."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Lock_PhotoCamera,
		"Runtime.Lock.PhotoCamera",
		"Blocks entry into the first-person photo camera mode."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Runtime_Lock_Tablet,
		"Runtime.Lock.Tablet",
		"Blocks opening the tablet UI."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger,
		"Tutorial.Trigger",
		"Root for the moments a tutorial overlay waits for. A DT_TutorialOverlay row lists "
		"these in RequiredTags alongside the previous row's Tutorial.Overlay.Seen tag."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_GameplayStarted,
		"Tutorial.Trigger.GameplayStarted",
		"The intro has faded out and the player has control. One-shot."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_InteractPromptShown,
		"Tutorial.Trigger.InteractPromptShown",
		"The [F] prompt has become visible for the first time. One-shot."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_InteractCompleted,
		"Tutorial.Trigger.InteractCompleted",
		"An interaction modal has closed and the player is back in third person. One-shot."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_PhotoCaptureCompleted,
		"Tutorial.Trigger.PhotoCaptureCompleted",
		"The player has left camera mode having taken at least one photo. One-shot."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_TabletOpened,
		"Tutorial.Trigger.TabletOpened",
		"The tablet is open right now. Paired with the close, because the intro already "
		"opens the tablet once and a one-shot tag would be satisfied before the tutorial "
		"ever asks for it."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_TabletClosed,
		"Tutorial.Trigger.TabletClosed",
		"The tablet has been closed and the player is looking at the world again. The "
		"inverse of TabletOpened, so only one of the two is ever present."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Tutorial_Trigger_SisterFolderOpened,
		"Tutorial.Trigger.SisterFolderOpened",
		"The sister's tablet folder page is open right now. Paired, as above."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Story_Chapter_01_Phase_01_Completed,
		"Story.Chapter.01.Phase.01.Completed",
		"All required photo sentences in chapter 01 phase 01 are solved."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Story_Chapter_01_Phase_02_Unlocked,
		"Story.Chapter.01.Phase.02.Unlocked",
		"Chapter 01 phase 02 becomes interactable after the first obstacle is cleared."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Story_Chapter_01_Phase_02_Completed,
		"Story.Chapter.01.Phase.02.Completed",
		"All required photo sentences in chapter 01 phase 02 are solved."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Story_Chapter_01_Phase_03_Unlocked,
		"Story.Chapter.01.Phase.03.Unlocked",
		"Chapter 01 phase 03 becomes interactable after the second obstacle is cleared."
	);

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		Story_Chapter_01_Phase_03_Completed,
		"Story.Chapter.01.Phase.03.Completed",
		"All required photo sentences in chapter 01 phase 03 are solved."
	);
}
