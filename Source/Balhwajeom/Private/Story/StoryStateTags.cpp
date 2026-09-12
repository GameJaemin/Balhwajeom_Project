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
}
