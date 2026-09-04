#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TabletWidgetBlueprintLibrary.generated.h"

/** Editor-only asset generator for the editable tablet prototype WBP. */
UCLASS()
class UTabletWidgetBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectTabletWidgetBlueprint();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool CreateTabletWidgetBlueprint();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RedesignTabletWidgetBlueprint();

	/** Instantiates the compiled WBP and exercises its navigation/state contract. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RunTabletWidgetSmokeTest();

};
