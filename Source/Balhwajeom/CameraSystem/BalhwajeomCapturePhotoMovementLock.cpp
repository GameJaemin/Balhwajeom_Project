#include "BalhwajeomCapturePhotoMovementLock.h"


bool FCapturePhotoMovementLock::Acquire()
{
	if (bLocked)
	{
		return false;
	}

	bLocked = true;
	return true;
}


bool FCapturePhotoMovementLock::Release()
{
	if (!bLocked)
	{
		return false;
	}

	bLocked = false;
	return true;
}
