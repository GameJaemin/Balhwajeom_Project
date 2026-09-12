#pragma once

#include "NativeGameplayTags.h"

/** Runtime-registered tags used only by the optional Editor automation module. */
namespace JMGameplayEventTestTags
{
    JMGAMEPLAYEVENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Door);
    JMGAMEPLAYEVENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(DoorOpened);
    JMGAMEPLAYEVENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(DoorClosed);
    JMGAMEPLAYEVENT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nested);
}
