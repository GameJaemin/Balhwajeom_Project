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
