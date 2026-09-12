#pragma once

#include "NativeGameplayTags.h"

/** Shared modal UI lifecycle tags. Feature plugins publish these; HUD systems subscribe. */
namespace JMGameplayUIEventTags
{
	JMGAMEPLAYEVENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ModalOpened);
	JMGAMEPLAYEVENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ModalClosed);
}
