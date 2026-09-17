#include "BalhwajeomPhotoCaptureAvailability.h"


bool BalhwajeomPhotoCaptureAvailability::IsCapturableNow(
	const bool bTargetInfoResolved,
	const bool bWithinSearchRadius,
	const bool bStateCanCapture,
	const bool bHasPhotoID,
	const bool bAlreadyCaptured)
{
	// A state that opts into capture without naming a photo cannot be recorded, which is
	// how several pre-interaction states are authored (a PhotoID is present but
	// bCanCapture is off, and the reverse would be just as unrecordable).
	return bTargetInfoResolved && bWithinSearchRadius && bStateCanCapture && bHasPhotoID &&
		!bAlreadyCaptured;
}
