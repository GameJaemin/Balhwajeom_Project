#pragma once

#include "CoreMinimal.h"


/**
 * When the investigation has produced everything the statement needs.
 *
 * "Every photo" means every photo whose analysis sentence is solved rather than merely
 * captured: an unsolved photo is not usable as statement evidence, and a photo cannot be
 * solved without having been captured first, so the one count covers both.
 *
 * A free function so the rule can be tested without a subsystem, and so the moment the
 * player is told to go read the statement is not buried in a widget.
 */
namespace BalhwajeomInvestigationProgress
{
	/**
	 * False whenever either total is zero. Data tables that have not loaded yet report
	 * nothing acquired out of nothing required, and announcing completion there would fire
	 * the prompt on the level's first frame.
	 */
	BALHWAJEOM_API bool IsReadyForStatement(
		int32 AcquiredWordCount,
		int32 RequiredWordCount,
		int32 SolvedPhotoSentenceCount,
		int32 TotalPhotoSentenceCount);
}
