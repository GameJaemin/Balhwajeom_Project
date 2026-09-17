#pragma once

#include "CoreMinimal.h"


/**
 * Balances the SetIgnoreMoveInput() calls the capture card makes.
 *
 * APlayerController::SetIgnoreMoveInput() is a counter, not a flag: an unmatched acquire
 * leaves the player unable to walk for the rest of the session, and an unmatched release
 * hands movement back to something else that was holding it. The card is released from
 * more than one place (the moment it starts leaving, its teardown, and the malformed
 * layout timeout), so the two calls are tracked here instead of at each call site.
 */
class BALHWAJEOM_API FCapturePhotoMovementLock
{
public:
	/** True when the caller still has to call SetIgnoreMoveInput(true). */
	bool Acquire();

	/** True when the caller still has to call SetIgnoreMoveInput(false). */
	bool Release();

	bool IsLocked() const { return bLocked; }

private:
	bool bLocked = false;
};
