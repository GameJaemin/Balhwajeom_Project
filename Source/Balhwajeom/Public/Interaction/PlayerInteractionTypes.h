#pragma once

#include "CoreMinimal.h"
#include "PlayerInteractionTypes.generated.h"


UENUM(BlueprintType)
enum class EPlayerInspectionDistanceState : uint8
{
	OutOfRange,
	Far,
	Middle,
	Close
};