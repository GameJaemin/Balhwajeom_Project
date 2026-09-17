#pragma once
#include "CoreMinimal.h"
class AActor;
class APawn;

/** What the interact key should do given what is already on screen. */
enum class EBalhwajeomInteractAction : uint8
{
	None,
	CloseInspection,
	Interact
};

/** Host adapter; reusable plugins have no dependency on Balhwajeom. */
namespace BalhwajeomItemInspection
{
	BALHWAJEOM_API bool IsOpen(const AActor* PlayerActor);

	/** Open and past its entrance transition, so the player is actually looking at the model. */
	BALHWAJEOM_API bool IsInteractive(const AActor* PlayerActor);
	BALHWAJEOM_API bool IsOtherModalOpen(const AActor* PlayerActor);
	BALHWAJEOM_API bool CanInspect(AActor* Target, APawn* Pawn);
	BALHWAJEOM_API bool TryInspect(AActor* Target, APawn* Pawn);
	BALHWAJEOM_API bool RequestClose(const AActor* PlayerActor);

	/**
	 * The rotating inspector closes on F through its own widget focus, which a click can move to the
	 * game viewport. Routing the gameplay F to a close keeps the player from being stuck behind an
	 * inspector that no longer hears the keyboard.
	 *
	 * An inspection that is still opening is never closed here. The pawn can carry more than one
	 * interaction component - BP_OrbitViewCharacter_Legacy adds BPC_PlayerInteraction on top of the
	 * one its C++ parent creates - so a single F press can reach this twice in the same frame, and
	 * closing on the second pass would kill the inspector before its entrance transition ever drew.
	 * Kept pure so the policy is testable.
	 */
	BALHWAJEOM_API EBalhwajeomInteractAction ResolveInteractAction(
		bool bInspectionOpen,
		bool bInspectionInteractive,
		bool bOtherModalOpen);
}
