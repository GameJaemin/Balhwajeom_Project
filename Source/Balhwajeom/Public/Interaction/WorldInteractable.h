#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WorldInteractable.generated.h"

class APawn;

UINTERFACE(BlueprintType)
class BALHWAJEOM_API UWorldInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Interaction contract for actions that do not produce normal inspection text. */
class BALHWAJEOM_API IWorldInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(APawn* InteractingPawn) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool RequestInteraction(APawn* InteractingPawn);
};
