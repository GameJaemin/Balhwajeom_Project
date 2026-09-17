#include "Tutorial/BalhwajeomTutorialOverlayQueue.h"

#include "Tutorial/TutorialOverlayDefinitions.h"

namespace BalhwajeomTutorialOverlayQueue
{
bool IsDue(
	const FTutorialOverlayDefinition& Definition,
	const FGameplayTagContainer& CurrentStateTags)
{
	if (Definition.RequiredTags.IsEmpty())
	{
		return false;
	}

	// An overlay with no completion tag could never be marked as seen, so it would
	// reopen on every matching moment for the rest of the run.
	if (!Definition.CompletionTag.IsValid() ||
		CurrentStateTags.HasTagExact(Definition.CompletionTag))
	{
		return false;
	}

	return CurrentStateTags.HasAllExact(Definition.RequiredTags);
}


TArray<FCandidate> SelectDueRows(
	const TArray<FCandidate>& AllRows,
	const FGameplayTagContainer& CurrentStateTags,
	const TArray<FName>& AlreadyQueued)
{
	TArray<FCandidate> DueRows;
	for (const FCandidate& Row : AllRows)
	{
		if (!Row.Definition || AlreadyQueued.Contains(Row.RowName))
		{
			continue;
		}

		if (IsDue(*Row.Definition, CurrentStateTags))
		{
			DueRows.Add(Row);
		}
	}
	return DueRows;
}
}
