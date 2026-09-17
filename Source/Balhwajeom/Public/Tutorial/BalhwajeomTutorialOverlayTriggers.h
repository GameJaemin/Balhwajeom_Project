#pragma once

#include "CoreMinimal.h"

struct FGameplayTag;


/**
 * One place for the systems that know when a tutorial moment happens to say so.
 *
 * Every emitter would otherwise repeat the same World -> GameInstance -> StoryState walk,
 * and a missing null check there would silently cost a tutorial screen instead of crashing.
 */
namespace BalhwajeomTutorialOverlayTriggers
{
	/** Adds Tag to the story state. Safe to call repeatedly; returns true only on the change. */
	BALHWAJEOM_API bool Set(const UObject* WorldContextObject, const FGameplayTag& Tag);

	/** Removes Tag again, for the moments that are a state rather than a milestone. */
	BALHWAJEOM_API bool Clear(const UObject* WorldContextObject, const FGameplayTag& Tag);

	BALHWAJEOM_API bool Has(const UObject* WorldContextObject, const FGameplayTag& Tag);
}
