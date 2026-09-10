#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DogAnimationBlueprintLibrary.generated.h"

/** Editor-only, deterministic generator/validator for ABP_Dog. */
UCLASS()
class UDogAnimationBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Animation")
	static bool CreateDogAnimationBlueprint();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Animation")
	static bool ValidateDogAnimationBlueprint();
};
