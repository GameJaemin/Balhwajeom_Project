#include "Investigation/BalhwajeomInvestigationProgress.h"


bool BalhwajeomInvestigationProgress::IsReadyForStatement(
	const int32 AcquiredWordCount,
	const int32 RequiredWordCount,
	const int32 SolvedPhotoSentenceCount,
	const int32 TotalPhotoSentenceCount)
{
	// Nothing required means the tables are not loaded, not that the player is done.
	if (RequiredWordCount <= 0 || TotalPhotoSentenceCount <= 0)
	{
		return false;
	}

	// Greater-or-equal on the keywords, because DT_Words carries more acquirable rows than
	// the displayed total the HUD counts against (see DisplayedTotalKeywordCount).
	return AcquiredWordCount >= RequiredWordCount &&
		SolvedPhotoSentenceCount >= TotalPhotoSentenceCount;
}
