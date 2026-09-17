#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FTutorialOverlayDefinition;


/**
 * Decides which overlay rows have become due. Kept free of widgets, worlds and
 * subsystems so the whole tutorial order can be tested by feeding it tags.
 */
namespace BalhwajeomTutorialOverlayQueue
{
	/** One candidate row, paired with the Row Name the presenter needs to show it. */
	struct FCandidate
	{
		FName RowName;
		const FTutorialOverlayDefinition* Definition = nullptr;
	};

	/**
	 * True when every RequiredTag is present and the overlay has not been closed before.
	 *
	 * A row with no RequiredTags is never due. Rows are authored with their conditions
	 * empty while the copy is being written, and treating "no conditions" as "always
	 * satisfied" would dump every unfinished screen on the player at once.
	 */
	BALHWAJEOM_API bool IsDue(
		const FTutorialOverlayDefinition& Definition,
		const FGameplayTagContainer& CurrentStateTags);

	/**
	 * Rows that are due, in Row Name order, skipping anything in AlreadyQueued.
	 *
	 * One tag can complete two rows at once, so the caller appends the result to its
	 * queue and shows them one at a time rather than picking a single winner.
	 */
	BALHWAJEOM_API TArray<FCandidate> SelectDueRows(
		const TArray<FCandidate>& AllRows,
		const FGameplayTagContainer& CurrentStateTags,
		const TArray<FName>& AlreadyQueued);
}
