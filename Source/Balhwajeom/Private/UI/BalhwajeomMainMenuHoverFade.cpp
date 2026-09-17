#include "UI/BalhwajeomMainMenuHoverFade.h"

#include "Math/UnrealMathUtility.h"


float BalhwajeomMainMenuHoverFade::EaseInOut(const float Alpha)
{
	const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
	return Clamped * Clamped * (3.0f - 2.0f * Clamped);
}


float BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(
	const float CurrentAlpha,
	const bool bHovered,
	const float DeltaSeconds,
	const float Duration)
{
	const float Target = bHovered ? 1.0f : 0.0f;
	if (Duration <= 0.0f)
	{
		return Target;
	}

	const float Step = FMath::Max(0.0f, DeltaSeconds) / Duration;
	const float Clamped = FMath::Clamp(CurrentAlpha, 0.0f, 1.0f);
	return bHovered
		? FMath::Min(Target, Clamped + Step)
		: FMath::Max(Target, Clamped - Step);
}


float BalhwajeomMainMenuHoverFade::ResolveOpacity(
	const float HoverAlpha,
	const float IdleOpacity,
	const float HoverOpacity)
{
	return FMath::Lerp(IdleOpacity, HoverOpacity, EaseInOut(HoverAlpha));
}
