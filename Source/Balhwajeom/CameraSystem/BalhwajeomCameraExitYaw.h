#pragma once

#include "CoreMinimal.h"


/**
 * Yaw handoff between photo camera mode and exploration.
 *
 * Entry aims the eye camera at the world point under the outgoing screen centre, which
 * adds a parallax delta to the control yaw. Carrying the camera-mode yaw straight back
 * out would re-apply that delta on every RMB press, so the exit subtracts it: a trip
 * with no look input lands on the exact yaw it started from, and whatever the player
 * actually looked around by is kept. Keeping it is the point — the exploration walk
 * direction is read off the control yaw, so discarding it changes what W means at the
 * same instant the character is handed back to bOrientRotationToMovement.
 *
 * The settle step stands in for bOrientRotationToMovement for the length of the fade-in,
 * so the turn is finished by the time the screen clears instead of starting there.
 *
 * Free functions so both steps can be tested without a pawn, a controller or a world.
 */
namespace BalhwajeomCameraExitYaw
{
	/** Camera-mode yaw minus the alignment the entry applied, unwound to (-180, 180]. */
	BALHWAJEOM_API float ResolveExplorationYaw(
		float CameraModeYaw,
		float EntryAlignmentYawDelta);

	/**
	 * One frame of the settle. Lands exactly on TargetYaw once the remaining time is
	 * spent, so the handoff to the movement component never leaves a residue to finish.
	 *
	 * EaseExponent below 1 takes a larger bite early, which puts most of the turn under
	 * the darkest part of the fade; 1.0 is constant speed. The step always travels the
	 * short way around, and never overshoots.
	 */
	BALHWAJEOM_API float StepSettleYaw(
		float CurrentYaw,
		float TargetYaw,
		float DeltaSeconds,
		float RemainingSeconds,
		float EaseExponent);
}
