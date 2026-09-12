#include "Types/JMInteractionEventTags.h"

namespace JMInteractionEventTags
{
    UE_DEFINE_GAMEPLAY_TAG(Interaction, "Event.Interaction");
    UE_DEFINE_GAMEPLAY_TAG(Succeeded, "Event.Interaction.Succeeded");
    UE_DEFINE_GAMEPLAY_TAG(Failed, "Event.Interaction.Failed");
    UE_DEFINE_GAMEPLAY_TAG(FocusStarted, "Event.Interaction.FocusStarted");
    UE_DEFINE_GAMEPLAY_TAG(FocusEnded, "Event.Interaction.FocusEnded");
}
