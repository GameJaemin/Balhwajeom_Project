#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PlayerInteractionModeTestTypes.generated.h"


UCLASS()
class UPlayerInteractionModeTestObserver final : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleInspectionDismissRequested();

	int32 DismissRequestCount = 0;
};
