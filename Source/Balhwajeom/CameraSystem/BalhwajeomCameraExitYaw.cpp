#include "BalhwajeomCameraExitYaw.h"

#include "Math/UnrealMathUtility.h"


float BalhwajeomCameraExitYaw::ResolveExplorationYaw(
	const float CameraModeYaw,
	const float EntryAlignmentYawDelta)
{
	return FMath::UnwindDegrees(CameraModeYaw - EntryAlignmentYawDelta);
}


float BalhwajeomCameraExitYaw::StepSettleYaw(
	const float CurrentYaw,
	const float TargetYaw,
	const float DeltaSeconds,
	const float RemainingSeconds,
	const float EaseExponent)
{
	// The last step of the settle, and every degenerate schedule, land on the target
	// outright. Interpolating here would leave a remainder for the movement component to
	// finish after the fade, which is the visible swing this whole path exists to remove.
	if (RemainingSeconds <= 0.0f || RemainingSeconds <= DeltaSeconds)
	{
		return FMath::UnwindDegrees(TargetYaw);
	}

	// Unwinding first is what makes 350 -> 10 a 20 degree step rather than a 340 degree one.
	const float RemainingDegrees = FMath::UnwindDegrees(TargetYaw - CurrentYaw);
	const float Alpha = FMath::Clamp(DeltaSeconds / RemainingSeconds, 0.0f, 1.0f);
	const float Eased = FMath::Pow(Alpha, FMath::Max(EaseExponent, KINDA_SMALL_NUMBER));

	// Alpha is below 1 on every step that reaches here, and so is Eased, so the result
	// stays on the near side of the target.
	return FMath::UnwindDegrees(CurrentYaw + RemainingDegrees * Eased);
}
