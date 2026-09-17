#pragma once

#include "CoreMinimal.h"


/**
 * Hover feedback for the title screen's start control.
 *
 * The button used to carry a different texture on its Hovered brush, which Slate swaps
 * the instant the cursor crosses the edge. Grazing that edge reads as flickering, so the
 * art is now identical in every state and the feedback is an eased opacity change
 * instead. These are free functions so the curve can be tested without a live widget.
 */
namespace BalhwajeomMainMenuHoverFade
{
	/**
	 * Cubic ease-out over 0..1: the fade moves fastest on its first frame and
	 * decelerates into the end, so the opacity responds immediately to the cursor and
	 * then settles instead of coasting at a constant speed.
	 */
	BALHWAJEOM_API float EaseOut(float Alpha);

	/**
	 * Moves the 0..1 hover alpha toward bHovered over Duration seconds and clamps it.
	 * A non-positive Duration snaps, which is how a designer turns the fade off.
	 */
	BALHWAJEOM_API float AdvanceHoverAlpha(
		float CurrentAlpha,
		bool bHovered,
		float DeltaSeconds,
		float Duration);

	/** Eases the alpha, then reads off the opacity between the idle and hovered values. */
	BALHWAJEOM_API float ResolveOpacity(
		float HoverAlpha,
		float IdleOpacity,
		float HoverOpacity);
}
