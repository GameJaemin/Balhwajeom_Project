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
	static bool UpdateMessengerTimeline();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool TestMessengerTimeline();

	/** Creates the planner-owned catalog/room Data Assets if missing; never overwrites existing data. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool CreateMessengerDataAssets();

	/** Creates the offline browser and its page Widget Blueprints without overwriting existing Designer edits. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool CreateInternetWidgetBlueprints();

	/** Rebuilds the Internet children and replaces only WBP_Tablet's Internet page slot. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InstallInternetBrowser();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectTabletWidgetBlueprint();

	/** Same dump as InspectTabletWidgetBlueprint, but for any Widget Blueprint by asset path (e.g. for reviewing a designer-authored WBP before wiring it up). */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectWidgetBlueprintByPath(const FString& AssetPath);

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool CreateTabletWidgetBlueprint();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RedesignTabletWidgetBlueprint();

	/** Instantiates the compiled WBP and exercises its navigation/state contract. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RunTabletWidgetSmokeTest();

	/** Adds an editable StoryText TextBlock to the photo story Widget Blueprint Designer. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Photo Story")
	static bool RedesignPhotoWorldStoryWidget();

	/** Creates DT_KeywordChoices and migrates choices embedded in legacy keyword documents. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Investigation")
	static bool UpgradeInvestigationDataTables();

};
