#include "Interaction/GateDoorLockedFeedback.h"

#include "Math/UnrealMathUtility.h"


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


float GateDoorLockedFeedback::ResolveFadeOpacity(
	float ElapsedSeconds,
	float FadeInDuration,
	float HoldDuration,
	float FadeOutDuration)
{
	const float FadeIn = FMath::Max(0.0f, FadeInDuration);
	const float Hold = FMath::Max(0.0f, HoldDuration);
	const float FadeOut = FMath::Max(0.0f, FadeOutDuration);

	if (ElapsedSeconds <= 0.0f)
	{
		// A message with no fade-in is already fully readable on its first frame.
		return FadeIn > 0.0f ? 0.0f : 1.0f;
	}

	if (ElapsedSeconds < FadeIn)
	{
		return FMath::Clamp(ElapsedSeconds / FadeIn, 0.0f, 1.0f);
	}

	if (ElapsedSeconds <= FadeIn + Hold)
	{
		return 1.0f;
	}

	const float FadeOutElapsed = ElapsedSeconds - FadeIn - Hold;
	if (FadeOut <= 0.0f || FadeOutElapsed >= FadeOut)
	{
		return 0.0f;
	}

	return FMath::Clamp(1.0f - (FadeOutElapsed / FadeOut), 0.0f, 1.0f);
}
