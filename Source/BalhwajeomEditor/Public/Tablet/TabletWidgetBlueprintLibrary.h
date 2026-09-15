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
	/** Rebuilds the item inspector as a centered preview with an editable bottom controls hint. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Inspection")
	static bool CenterItemInspectionWidget();

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

	/** Creates the editable folder button, file tile, folder section, statement, and photo templates. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool CreateTabletDesignerWidgets();

	/** Rebuilds only the statement detail WBP from the 1274x907 statement reference art. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RedesignTabletStatementWidget();

	/** Rebuilds only the photo-analysis detail WBP from the 1274x907 reference art. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RedesignTabletPhotoWidget();

	/** Rebuilds WBP_Messenger as the static four-room UI using the final messenger artwork. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool RedesignMessengerWidget();

	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectTabletWidgetBlueprint();

	/** Same dump as InspectTabletWidgetBlueprint, but for any Widget Blueprint by asset path (e.g. for reviewing a designer-authored WBP before wiring it up). */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Tablet")
	static bool InspectWidgetBlueprintByPath(const FString& AssetPath);

	/** Gives WBP_ObjectLabel's text container a stable name and its non-photo X=22 default. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Inspection")
	static bool ConfigureObjectLabelLayout();

	/** Matches the focus guide's icon layout and copies WBP_ObjectLabel's Multi Shadow Text styling. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Inspection")
	static bool ConfigureEvidenceFocusGuideLayout();

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

	/** Creates the editable capture-result overlay used after a successful photo. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Camera")
	static bool CreateCapturePhotoWidgetBlueprint();

	/** Creates WBP_MainMenu, WBP_ScreenFade, and BP_IntroFlowController. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Intro")
	static bool CreateIntroFlowAssets();

	/** Configures the room4 intro actor for the project's full-screen MP4 assets. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Intro")
	static bool ConfigureRoom4IntroMedia();

	/** Creates DT_KeywordChoices and migrates choices embedded in legacy keyword documents. */
	UFUNCTION(BlueprintCallable, Category = "Balhwajeom|Editor|Investigation")
	static bool UpgradeInvestigationDataTables();

};
