#pragma once

#include "CoreMinimal.h"


/**
 * Zoom readout for the viewfinder.
 *
 * Zoom is not stored anywhere: it only exists as the photo camera's field of view,
 * between MinCameraFieldOfView (fully zoomed in) and MaxCameraFieldOfView (fully zoomed
 * out). These turn that into the 0..1 the UI wants, and then into a slot position. Free
 * functions so both steps can be tested without a camera or a widget.
 */
namespace BalhwajeomPhotoCameraZoom
{
	/**
	 * 0 at the widest field of view, 1 at the narrowest. The bounds are accepted in
	 * either order, matching how ZoomCamera() normalises them before clamping, and a
	 * degenerate range reports 0 rather than dividing by it.
	 */
	BALHWAJEOM_API float ResolveZoomAlpha(
		float FieldOfView,
		float MinFieldOfView,
		float MaxFieldOfView);

	/** Reads the zoom bar's vertical slot position off the two authored endpoints. */
	BALHWAJEOM_API float ResolveZoomBarPosition(
		float ZoomAlpha,
		float MinZoomPosition,
		float MaxZoomPosition);
}
