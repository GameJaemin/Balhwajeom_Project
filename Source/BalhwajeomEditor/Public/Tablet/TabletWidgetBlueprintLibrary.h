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

	/** Creates the editable full folder screen and embeds it in WBP_Tablet's folder page. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InstallTabletPersonFolderWidget();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectTabletWidgetBlueprint();

	/** Same dump as InspectTabletWidgetBlueprint, but for any Widget Blueprint by asset path (e.g. for reviewing a designer-authored WBP before wiring it up). */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectWidgetBlueprintByPath(const FString& AssetPath);

	/** Sets a named UButton's content to an Image using the given texture (e.g. an icon for a physical button). */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool SetButtonIconTexture(const FString& AssetPath, const FString& ButtonName, const FString& TexturePath);

	/** Wraps a Widget Blueprint's existing root in a new ScaleBox(ScaleToFit)+SizeBox(DesignWidth x DesignHeight),
	 * so its existing absolute-coordinate content (designed for that fixed size) scales proportionally to
	 * whatever area it's actually given at runtime, instead of staying pinned to old pixel coordinates.
	 * No-op (returns true) if the root is already a ScaleBox. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool WrapRootInScaleBox(const FString& AssetPath, float DesignWidth, float DesignHeight);

	/** Sets a named widget's UCanvasPanelSlot position/size (its parent must be a CanvasPanel). */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool SetCanvasSlotGeometry(
		const FString& AssetPath, const FString& WidgetName, float X, float Y, float Width, float Height);

	/** Sets a named USizeBox's WidthOverride/HeightOverride. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool SetSizeBoxOverride(
		const FString& AssetPath, const FString& WidgetName, float Width, float Height);

	/** Adds a new transparent text button (matching the popup's existing BTN_PopupClose/BTN_StatementSubmit
	 * styling) as a canvas-positioned child of a named CanvasPanel. Structural add only, never a rename --
	 * safe against the WidgetVariableNameToGuidMap ensure. No-op (returns true) if ButtonName already exists. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool AddTextButtonToCanvas(
		const FString& AssetPath, const FString& ParentCanvasName, const FString& ButtonName,
		const FString& Label, float X, float Y, float Width, float Height, int32 FontSize = 22);

	/** Sets a UTexture2D-typed EditDefaultsOnly property on a Widget Blueprint's class default object
	 * (e.g. DefaultFolderIcon, StatementFileIcon), the same way the Designer's Class Defaults panel would. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool SetClassDefaultTexture(
		const FString& AssetPath, const FString& PropertyName, const FString& TexturePath);

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
