#pragma once

#include "CoreMinimal.h"


/**
 * Where the emergency escape puts a player who is wedged in the level.
 *
 * The destination is derived from a placed door rather than a marker actor, so no level
 * needs editing to support the escape -- a map with a gate door already has somewhere
 * sensible to send the player.
 *
 * Free functions so the choice and the offset maths can be tested without a world.
 */
namespace BalhwajeomEmergencyEscape
{
	/**
	 * Index of the door closest to the player, or INDEX_NONE when there are none. Ties go
	 * to the lower index so the result does not depend on actor iteration order.
	 */
	BALHWAJEOM_API int32 ResolveNearestDoorIndex(
		const FVector& PlayerLocation,
		const TArray<FVector>& DoorLocations);

	/**
	 * Spot in front of the door, in the door's own space.
	 *
	 * Both the offset and the facing are local because a gate door actor sits on its hinge,
	 * not in the middle of its panel: which way "in front" points, and how far off-centre
	 * the hinge is, are properties of how the door was placed rather than anything that can
	 * be derived from it.
	 *
	 * LocalYaw is given rather than aimed at the door for the same reason -- aiming at the
	 * actor aims at the hinge, which leaves the player standing at an angle to the doorway
	 * by however wide the door is.
	 */
	BALHWAJEOM_API FTransform ResolveEscapeTransform(
		const FTransform& DoorTransform,
		const FVector& LocalOffset,
		float LocalYaw);
}
