#include "UI/BalhwajeomNotificationBanner.h"

#include "Math/UnrealMathUtility.h"


float BalhwajeomNotificationBanner::ResolveOpacity(
	const float Elapsed,
	const float FadeInSeconds,
	const float HoldSeconds,
	const float FadeOutSeconds)
{
	if (Elapsed <= 0.0f)
	{
		return 0.0f;
	}

	const float FadeIn = FMath::Max(FadeInSeconds, 0.0f);
	const float Hold = FMath::Max(HoldSeconds, 0.0f);
	const float FadeOut = FMath::Max(FadeOutSeconds, 0.0f);

	// A zero-length fade is a valid setting -- it means "appear at once", not "divide by zero".
	if (Elapsed < FadeIn)
	{
		return FMath::Clamp(Elapsed / FadeIn, 0.0f, 1.0f);
	}

	const float HoldEnd = FadeIn + Hold;
	if (Elapsed < HoldEnd)
	{
		return 1.0f;
	}

	if (FadeOut <= 0.0f)
	{
		return 0.0f;
	}

	const float FadeOutElapsed = Elapsed - HoldEnd;
	return FMath::Clamp(1.0f - (FadeOutElapsed / FadeOut), 0.0f, 1.0f);
}


bool BalhwajeomNotificationBanner::IsFinished(
	const float Elapsed,
	const float FadeInSeconds,
	const float HoldSeconds,
	const float FadeOutSeconds)
{
	const float Total =
		FMath::Max(FadeInSeconds, 0.0f) +
		FMath::Max(HoldSeconds, 0.0f) +
		FMath::Max(FadeOutSeconds, 0.0f);
	return Elapsed >= Total;
}
