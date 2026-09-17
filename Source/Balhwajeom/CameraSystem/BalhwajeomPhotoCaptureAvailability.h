#pragma once

#include "CoreMinimal.h"


/**
 * Whether a camera target can be photographed from where the player is standing.
 *
 * This mirrors the gate UBalhwajeomPhotoCameraComponent::TryCaptureActiveFocusTarget()
 * applies, minus the screen-occupancy check: whether anything is still worth keeping the
 * camera up for is a different question from whether it fills enough of this exact
 * frame. Kept as a free function so the rule can be tested without a live world.
 */
namespace BalhwajeomPhotoCaptureAvailability
{
	/**
	 * @param bTargetInfoResolved  RequestCameraTargetInfo() succeeded. This is already
	 *                             false for evidence whose progression gate is closed,
	 *                             so phase-locked objects never count as available.
	 * @param bWithinSearchRadius  The target is inside the search radius around the
	 *                             player. Only the far edge is applied: an object the
	 *                             player is standing right next to still counts, even
	 *                             though the camera would need a step back to focus it.
	 * @param bStateCanCapture     The live state's bCanCapture.
	 * @param bHasPhotoID          The live state names a photo to record.
	 * @param bAlreadyCaptured     That photo is already in the gallery.
	 */
	BALHWAJEOM_API bool IsCapturableNow(
		bool bTargetInfoResolved,
		bool bWithinSearchRadius,
		bool bStateCanCapture,
		bool bHasPhotoID,
		bool bAlreadyCaptured);
}
