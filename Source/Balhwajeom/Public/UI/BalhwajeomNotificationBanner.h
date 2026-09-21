#pragma once

#include "CoreMinimal.h"


/**
 * The top-of-screen notification banner's own lifetime.
 *
 * Nothing dismisses this banner -- it takes no input at all -- so the schedule is the only
 * thing that ends it. Free functions so that schedule can be tested without a viewport.
 */
namespace BalhwajeomNotificationBanner
{
	/** Fades up, holds, fades away. Past the end of the schedule it stays at 0. */
	BALHWAJEOM_API float ResolveOpacity(
		float Elapsed,
		float FadeInSeconds,
		float HoldSeconds,
		float FadeOutSeconds);

	/** True once the banner has nothing left to show and can leave the viewport. */
	BALHWAJEOM_API bool IsFinished(
		float Elapsed,
		float FadeInSeconds,
		float HoldSeconds,
		float FadeOutSeconds);
}
