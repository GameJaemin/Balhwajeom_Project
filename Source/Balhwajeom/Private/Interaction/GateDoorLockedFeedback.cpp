#include "Interaction/GateDoorLockedFeedback.h"


FText GateDoorLockedFeedback::ResolveFirstIncompleteMessage(
	const TArray<FGateDoorLockedFeedbackStage>& Stages,
	const FGameplayTagContainer& CurrentStateTags)
{
	for (const FGateDoorLockedFeedbackStage& Stage : Stages)
	{
		// An empty requirement is already satisfied and should not block later guidance.
		if (!Stage.CompleteWhenAllTags.IsEmpty() &&
			!CurrentStateTags.HasAll(Stage.CompleteWhenAllTags))
		{
			return Stage.IncompleteMessage;
		}
	}

	return FText::GetEmpty();
}
