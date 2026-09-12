#pragma once
#include "CoreMinimal.h"
class AActor;
class APawn;

/** Host adapter; reusable plugins have no dependency on Balhwajeom. */
namespace BalhwajeomItemInspection
{
	BALHWAJEOM_API bool IsOpen(const AActor* PlayerActor);
	BALHWAJEOM_API bool IsOtherModalOpen(const AActor* PlayerActor);
	BALHWAJEOM_API bool CanInspect(AActor* Target, APawn* Pawn);
	BALHWAJEOM_API bool TryInspect(AActor* Target, APawn* Pawn);
}
