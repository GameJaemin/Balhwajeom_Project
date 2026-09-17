#pragma once

#include "CoreMinimal.h"


/**
 * The click path's keyword flight: a clicked candidate keyword travels to the sentence
 * blank instead of appearing in it. Dragging already carries the chip there by hand, so
 * only the click animates.
 *
 * ResolveFirstOpenSlot is what keeps the completion check honest. A chip in flight has
 * only reserved its blank, not filled it, so the submission record -- and with it
 * EvaluatePuzzleIfComplete -- is written on arrival rather than on the click. Two quick
 * clicks therefore take two different blanks, and the sentence is judged after the last
 * keyword has landed rather than the moment it was picked.
 *
 * Free functions so the curve and the slot choice can be tested without a widget.
 */
namespace BalhwajeomTabletKeywordFlight
{
	/**
	 * Eased 0..1 progress. An exponent above 1 decelerates into the blank, which is what
	 * reads as the chip shooting out and settling; 1.0 is constant speed. A spent or
	 * degenerate duration reports 1 so the caller lands rather than dividing by it.
	 */
	BALHWAJEOM_API float ResolveEaseOutAlpha(float Elapsed, float Duration, float Exponent);

	/** Viewport-space centre of the flying chip at that progress. */
	BALHWAJEOM_API FVector2D ResolveFlightPosition(
		const FVector2D& StartPosition,
		const FVector2D& EndPosition,
		float Alpha);

	/** StartScale as the chip leaves, 1.0 as it lands. StartScale 1.0 disables the pop. */
	BALHWAJEOM_API float ResolveFlightScale(float StartScale, float Alpha);

	/**
	 * Lowest blank that is neither already submitted nor already claimed by a chip still
	 * in flight, or INDEX_NONE when the sentence has no room left. Slot indices may
	 * arrive in any order.
	 */
	BALHWAJEOM_API int32 ResolveFirstOpenSlot(
		const TArray<int32>& SlotIndices,
		const TArray<int32>& FilledSlotIndices,
		const TArray<int32>& PendingSlotIndices);
}
