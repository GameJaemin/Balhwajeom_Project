#include "Tablet/BalhwajeomTabletKeywordFlight.h"

#include "Math/UnrealMathUtility.h"


float BalhwajeomTabletKeywordFlight::ResolveEaseOutAlpha(
	const float Elapsed,
	const float Duration,
	const float Exponent)
{
	// A zero-length flight is still a valid configuration -- it just means "land at once".
	if (Duration <= 0.0f || Elapsed >= Duration)
	{
		return 1.0f;
	}
	if (Elapsed <= 0.0f)
	{
		return 0.0f;
	}

	const float Time = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	return 1.0f - FMath::Pow(1.0f - Time, FMath::Max(Exponent, KINDA_SMALL_NUMBER));
}


FVector2D BalhwajeomTabletKeywordFlight::ResolveFlightPosition(
	const FVector2D& StartPosition,
	const FVector2D& EndPosition,
	const float Alpha)
{
	return FMath::Lerp(StartPosition, EndPosition, FMath::Clamp(Alpha, 0.0f, 1.0f));
}


float BalhwajeomTabletKeywordFlight::ResolveFlightScale(const float StartScale, const float Alpha)
{
	return FMath::Lerp(StartScale, 1.0f, FMath::Clamp(Alpha, 0.0f, 1.0f));
}


int32 BalhwajeomTabletKeywordFlight::ResolveFirstOpenSlot(
	const TArray<int32>& SlotIndices,
	const TArray<int32>& FilledSlotIndices,
	const TArray<int32>& PendingSlotIndices)
{
	int32 OpenSlot = INDEX_NONE;
	for (const int32 SlotIndex : SlotIndices)
	{
		if (FilledSlotIndices.Contains(SlotIndex) || PendingSlotIndices.Contains(SlotIndex))
		{
			continue;
		}

		// Scanned rather than taking the first hit, so the caller does not have to sort.
		if (OpenSlot == INDEX_NONE || SlotIndex < OpenSlot)
		{
			OpenSlot = SlotIndex;
		}
	}

	return OpenSlot;
}
