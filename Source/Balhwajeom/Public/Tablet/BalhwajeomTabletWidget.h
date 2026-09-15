#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Investigation/SentenceDefinitions.h"
#include "Styling/SlateBrush.h"
#include "BalhwajeomTabletWidget.generated.h"

class UBorder;
class UBalhwajeomInternetWidget;
class UBalhwajeomMessengerWidget;
class UButton;
class UImage;
class UOverlay;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class UFont;
class UWrapBox;
class UWidgetSwitcher;
class UWidgetAnimation;
class UBalhwajeomInvestigationSubsystem;
class USoundBase;

UENUM(BlueprintType)
enum class ETabletPage : uint8
{
	Home,
	PersonFolder,
	Messenger,
	Internet,
	Memo
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletPhotoSelected, FName, PhotoID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletFolderSelected, FName, CharacterID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTabletPersonFolderBackRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletPersonFolderTabRequested, int32, TabIndex);
/** OriginSlotIndex is the sentence blank the word was dragged out of (INDEX_NONE if it came from the acquired-keyword list instead). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTabletBlankDropped, int32, SlotIndex, FName, WordID, int32, OriginSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTabletPhotoSlotDropped, int32, SlotIndex, FName, PhotoID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletPhotoSlotClicked, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletWordChipClicked, FName, WordID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletBlankClicked, int32, SlotIndex);

/** Runtime-created photo entry shared by the folder grid and the statement tile. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoButton : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Thumbnail is optional: the statement tile passes nullptr for a text-only tile. */
	void Configure(FName InPhotoID, const FText& InLabel, UTexture2D* Thumbnail = nullptr);

	/** Overrides ThumbnailWidth/ThumbnailHeight's shared Class Default for just this instance
	 * (e.g. the folder's single, intentionally larger statement tile) -- every other tile of the
	 * same WBP_TabletFileTile class keeps using the Class Default. Call before Configure() so the
	 * override is already in effect the first time it sizes SB_Thumbnail. */
	void SetThumbnailSizeOverride(float InWidth, float InHeight);

	UPROPERTY()
	FOnTabletPhotoSelected OnPhotoSelected;

	/** Font for the filename label (TXT_Label) below the thumbnail. Null keeps the engine default typeface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|File Tile")
	TObjectPtr<UFont> LabelFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|File Tile", meta = (ClampMin = "8", ClampMax = "40"))
	int32 LabelFontSize = 16;

	/** Size of SB_Thumbnail/IMG_Thumbnail. The outer tile box (MakeFolderTileSlot) must be at
	 * least this tall plus room for the label below it, or the label gets clipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|File Tile", meta = (ClampMin = "1.0"))
	float ThumbnailWidth = 96.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|File Tile", meta = (ClampMin = "1.0"))
	float ThumbnailHeight = 64.0f;

private:
	virtual void NativeOnInitialized() override;
	void BuildFallbackVisuals();

	UFUNCTION()
	void HandleClicked();

	FName PhotoID = NAME_None;
	bool bHasThumbnailSizeOverride = false;
	float ThumbnailWidthOverride = 0.0f;
	float ThumbnailHeightOverride = 0.0f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_File;

	/** Holds SB_Thumbnail then TXT_Label (or SB_Label). Bound so Configure() can bottom-align it
	 * within BTN_File, so the label lines up across tiles of different outer heights (e.g. the
	 * folder's taller statement tile next to regular photo tiles). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VB_FileLayout;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SB_Thumbnail;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Thumbnail;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Label;
};

/** Runtime-created home-page entry for one DT_Characters folder. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletFolderButton : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(
		FName InCharacterID,
		const FText& InLabel,
		UTexture2D* IconTexture,
		const FSlateFontInfo& InLabelFont);

	UPROPERTY()
	FOnTabletFolderSelected OnFolderSelected;

private:
	virtual void NativeOnInitialized() override;
	void BuildFallbackVisuals();

	UFUNCTION()
	void HandleClicked();

	FName CharacterID = NAME_None;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Folder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_FolderIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_FolderLabel;
};

/**
 * Collapsible group header for the folder page's file list (like Windows Explorer's date groups):
 * a clickable title row ("{Title} ({Count})" with a ▼/▶ fold arrow) above a WrapBox of file tiles.
 * Built entirely at runtime; RefreshFolderContents() clears and rebuilds one of these per bucket
 * (단서와 정보/증거 사진/추억 사진) every time a folder is opened.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletFolderSection : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(const FText& InTitle, bool bStartExpanded = true);
	void AddTile(UWidget* Tile);
	void ClearTiles();
	bool IsEmpty() const { return TileCount == 0; }

	/** Font for the section title ("단서와 정보"/"증거 사진"/"추억 사진"). Null keeps the engine default typeface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Folder Section")
	TObjectPtr<UFont> TitleFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Folder Section", meta = (ClampMin = "8", ClampMax = "48"))
	int32 TitleFontSize = 22;

private:
	virtual void NativeOnInitialized() override;
	void BuildFallbackVisuals();

	UFUNCTION()
	void HandleHeaderClicked();

	void RefreshHeaderText();

	FText Title;
	int32 TileCount = 0;
	bool bExpanded = true;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HeaderButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ArrowText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> ContentWrapBox;
};

/** Designer-owned detail surface used by the separate statement and photo Widget Blueprints. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomTabletDetailWidget(const FObjectInitializer& ObjectInitializer);

	UTextBlock* GetTitleText() const { return TXT_PopupTitle; }
	UTextBlock* GetBodyText() const { return TXT_PopupBody; }
	UImage* GetPhotoImage() const { return IMG_PopupPhoto; }
	UImage* GetStatementIllustration() const { return IMG_StatementIllustration; }
	UButton* GetCloseButton() const { return BTN_PopupClose; }
	UButton* GetPlayVoiceButton() const { return BTN_PlayStoryVoice; }
	UWrapBox* GetPuzzleWords() const { return WB_PuzzleWords; }
	UWrapBox* GetSentenceBuilder() const { return WB_SentenceBuilder; }
	UTextBlock* GetPuzzleFeedback() const { return TXT_PuzzleFeedback; }
	UTextBlock* GetSelectedPhotoResult() const { return TXT_SelectedPhotoResult; }
	UTextBlock* GetPuzzlePhotoLabel() const { return TXT_PuzzlePhotoLabel; }
	UWrapBox* GetPuzzlePhotos() const { return WB_PuzzlePhotos; }
	UWrapBox* GetPhotoSlots() const { return WB_PhotoSlots; }
	UButton* GetStatementSubmitButton() const { return BTN_StatementSubmit; }
	UTextBlock* GetPuzzleKeywordCount() const { return TXT_PuzzleKeywordCount; }
	UWidget* GetPhotoPickerPanel() const { return PhotoPickerPanel; }
	UButton* GetPhotoPickerCloseButton() const { return BTN_PhotoPickerClose; }
	class UBalhwajeomTabletPersonFolderWidget* GetPhotoPickerFolder() const { return WBP_PhotoPickerFolder; }
	UFont* GetKeywordFont() const { return KeywordFont; }
	int32 GetKeywordFontSize() const { return KeywordFontSize; }
	UFont* GetStatementTextFont() const { return StatementTextFont; }
	int32 GetStatementTextFontSize() const { return StatementTextFontSize; }
	UFont* GetSelectedPhotoResultFont() const { return SelectedPhotoResultFont; }
	int32 GetSelectedPhotoResultFontSize() const { return SelectedPhotoResultFontSize; }
	UFont* GetAnalysisResultFont() const { return AnalysisResultFont; }
	int32 GetAnalysisResultFontSize() const { return AnalysisResultFontSize; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Statement Style")
	TObjectPtr<UFont> KeywordFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Statement Style", meta = (ClampMin = "8", ClampMax = "40"))
	int32 KeywordFontSize = 14;

	/** Font shared by TXT_PopupBody, sentence fragments, and keyword drop blanks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Statement Style")
	TObjectPtr<UFont> StatementTextFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Statement Style", meta = (ClampMin = "8", ClampMax = "40"))
	int32 StatementTextFontSize = 16;

	/** Font used by the selected evidence photo's ResultText below the photo area. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Statement Style")
	TObjectPtr<UFont> SelectedPhotoResultFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Statement Style", meta = (ClampMin = "8", ClampMax = "40"))
	int32 SelectedPhotoResultFontSize = 11;

	/** Photo detail widget only: font for TXT_PopupBody once a photo's analysis sentence has been
	 * solved (its ResultText), shown left-aligned instead of the puzzle's default centered style.
	 * Every other TXT_PopupBody case (plain natural-language photos, the statement body) keeps
	 * this widget's own Designer-authored justification/font untouched. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Photo Style")
	TObjectPtr<UFont> AnalysisResultFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Photo Style", meta = (ClampMin = "8", ClampMax = "40"))
	int32 AnalysisResultFontSize = 24;

private:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TXT_PopupTitle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TXT_PopupBody;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_PopupPhoto;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_StatementIllustration;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_PopupClose;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_PlayStoryVoice;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWrapBox> WB_PuzzleWords;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWrapBox> WB_SentenceBuilder;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TXT_PuzzleFeedback;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TXT_SelectedPhotoResult;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TXT_PuzzlePhotoLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWrapBox> WB_PuzzlePhotos;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWrapBox> WB_PhotoSlots;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_StatementSubmit;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TXT_PuzzleKeywordCount;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> PhotoPickerPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_PhotoPickerClose;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UBalhwajeomTabletPersonFolderWidget> WBP_PhotoPickerFolder;
};

/**
 * Designer-owned full folder screen shown after a home-page folder is selected.
 * WBP_TabletPersonFolder owns the window chrome and list placement; the parent tablet
 * supplies the selected character's title and the runtime-created section widgets.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPersonFolderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomTabletPersonFolderWidget(const FObjectInitializer& ObjectInitializer);
	void SetFolderHeader(const FText& FolderName, UTexture2D* FolderIcon);
	void SetSelectedTab(int32 TabIndex);
	void SetFeedbackMessage(const FText& Message);
	void ClearFeedbackMessage();
	void ClearFolderSections();
	void AddFolderSection(UWidget* Section);
	/** Re-checks whether SB_EvidencePhotos actually has anything to scroll and shows/hides
	 * IMG_FolderScroll accordingly. Call after repopulating the folder's contents, since
	 * OnUserScrolled only fires once the player actually drags the scrollbar. */
	void RefreshScrollIndicator();

	/** Only BTN_FolderSister's slot is used to show whichever family member's folder is actually
	 * open (BTN_FolderMother/BTN_FolderBrother and every *Selected image are unused) -- these are
	 * the three possible icons SetFolderHeader picks from by matching the folder name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Folder Icon")
	TObjectPtr<UTexture2D> SisterFolderIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Folder Icon")
	TObjectPtr<UTexture2D> MotherFolderIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Folder Icon")
	TObjectPtr<UTexture2D> BrotherFolderIcon;

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Folder")
	FOnTabletPersonFolderBackRequested OnBackRequested;

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Folder")
	FOnTabletPersonFolderTabRequested OnTabRequested;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION() void HandleSisterTabClicked();
	UFUNCTION() void HandleMotherTabClicked();
	UFUNCTION() void HandleBrotherTabClicked();
	UFUNCTION() void HandleFolderScrolled(float CurrentOffset);
	void RefreshTabVisuals();
	int32 SelectedTabIndex = 0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_FolderTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_FolderFeedback;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_FolderTitleIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_FolderClose;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_FolderSister;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_FolderMother;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BTN_FolderBrother;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderSisterIdle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderSisterSelected;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderMotherIdle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderMotherSelected;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderBrotherIdle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderBrotherSelected;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> IMG_FolderScroll;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SB_EvidencePhotos;
};

/** Drag payload: which acquired keyword is being dragged onto a sentence blank. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomWordDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Tablet")
	FName WordID = NAME_None;

	/** Set only when dragged out of a UBalhwajeomTabletSentenceBlank; INDEX_NONE when it came from the acquired-keyword list. */
	UPROPERTY(BlueprintReadWrite, Category = "Tablet")
	int32 OriginSlotIndex = INDEX_NONE;
};

/**
 * Draggable keyword tag. Used both as the puzzle's candidate list (drag onto a sentence
 * blank to submit it) and as the folder's plain acquired-word display (drag has nowhere to
 * land there, so it's inert). A UUserWidget rather than a UButton because native drag
 * detection (NativeOnMouseButtonDown/NativeOnDragDetected) is only overridable on UUserWidget.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletWordChip : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(
		FName InWordID,
		const FText& InLabel,
		bool bInStatementStyle = false,
		UFont* InStatementFont = nullptr,
		int32 InStatementFontSize = 14);
	FName GetWordID() const { return WordID; }

	/** Broadcast on a plain left-click (no drag detected), so a candidate keyword can be placed
	 * into the puzzle's first empty blank with a single click instead of a drag. */
	UPROPERTY()
	FOnTabletWordChipClicked OnWordChipClicked;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	FName WordID = NAME_None;
	FText DisplayLabel;
	bool bStatementStyle = false;

	UPROPERTY()
	TObjectPtr<UBorder> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> LabelText;

	/** Photo-style (non-statement) chip's background brush before hover, so mouse-leave can restore it after ApplyKeywordHoverBrush overwrites Background's brush on hover. */
	FSlateBrush NormalBrush;
};

/**
 * One droppable blank ("[]") inside the interactive sentence-builder row. Once filled, it is
 * also itself a drag source: picking a filled blank back up carries its word onward (tagged with
 * this blank's own slot index as the drag's origin) so a placed keyword can be freely moved to a
 * different blank, swapping with whatever is already there, instead of staying locked in place.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletSentenceBlank : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(
		int32 InSlotIndex,
		bool bInStatementStyle = false,
		UFont* InStatementFont = nullptr,
		int32 InStatementFontSize = 16);
	void SetFilled(FName InWordID, const FText& WordText);
	void SetEmpty();
	void SetErrorStyle(bool bInError);
	int32 GetSlotIndex() const { return SlotIndex; }
	bool IsFilled() const { return !FilledWordID.IsNone(); }

	UPROPERTY()
	FOnTabletBlankDropped OnBlankDropped;

	/** Broadcast on a plain left-click (no drag detected) while filled, so the placed keyword can
	 * be removed with a single click instead of a drag. Never fires while empty. */
	UPROPERTY()
	FOnTabletBlankClicked OnBlankClicked;

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	int32 SlotIndex = 0;
	FName FilledWordID = NAME_None;
	bool bStatementStyle = false;
	bool bErrorStyle = false;

	UPROPERTY()
	TObjectPtr<UBorder> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> DisplayText;
};

/** Drag payload: which captured (and analysis-solved) photo is being dragged onto a photo evidence slot. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomPhotoDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Tablet")
	FName PhotoID = NAME_None;
};

/**
 * Draggable photo-evidence tag, shown for a captured photo once its own analysis sentence
 * (PhotoSentenceID) is solved. Drag onto a UBalhwajeomTabletPhotoSlot to submit it as evidence.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoChip : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(FName InPhotoID, const FText& InLabel, UTexture2D* Thumbnail = nullptr);
	FName GetPhotoID() const { return PhotoID; }

	UPROPERTY()
	FOnTabletPhotoSelected OnPhotoSelected;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	FName PhotoID = NAME_None;
	FText DisplayLabel;

	UPROPERTY()
	TObjectPtr<UTexture2D> Thumbnail;
};

/** One droppable photo-evidence slot (FSentencePhotoSlot) inside the active sentence's photo evidence row. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(int32 InSlotIndex, bool bInStatementStyle = false);
	void SetFilled(const FText& PhotoLabel, UTexture2D* Thumbnail = nullptr);
	void SetEmpty();
	int32 GetSlotIndex() const { return SlotIndex; }

	UPROPERTY()
	FOnTabletPhotoSlotDropped OnPhotoSlotDropped;

	/** Broadcast on a left-click while this slot is empty, so the owning tablet can open a
	 * candidate-picker popup instead of requiring a drag-and-drop. */
	UPROPERTY()
	FOnTabletPhotoSlotClicked OnPhotoSlotClicked;

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	int32 SlotIndex = 0;
	bool bFilled = false;
	bool bStatementStyle = false;

	UPROPERTY()
	TObjectPtr<UBorder> Background;

	UPROPERTY()
	TObjectPtr<UTextBlock> DisplayText;

	UPROPERTY()
	TObjectPtr<USizeBox> ThumbnailBox;

	UPROPERTY()
	TObjectPtr<UImage> ThumbnailImage;
};

/** Navigation/state logic for the designer-owned WBP_Tablet visual tree. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomTabletWidget(const FObjectInitializer& ObjectInitializer);

	FSimpleMulticastDelegate OnTabletCloseAnimationFinished;

	/** Plays the designer-authored TabletUpAnim using its authored duration. */
	bool PlayTabletOpenAnimation();

	/** Returns false when TabletUpAnim is absent, so the owner can close immediately. */
	bool PlayTabletCloseAnimation();

	/** Lets automated validation confirm the WBP animation is bound without playing UMG in a commandlet. */
	bool HasTabletTransitionAnimation() const { return TabletUpAnim != nullptr; }

	/** Reverses an in-progress close when IA_Tablet is pressed again. */
	void CancelTabletCloseAnimation();

	/** Physical Home behavior: clears page history and always opens Home. */
	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void ResetToDesktop();

	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetUnreadMessageCount(int32 NewCount);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetUnreadMessageCount() const { return UnreadMessageCount; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerWidget* GetMessengerWidget() const { return WBP_Messenger; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	UBalhwajeomInternetWidget* GetInternetWidget() const { return WBP_Internet; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Navigation")
	ETabletPage GetCurrentPage() const { return CurrentPage; }

	/** DT_Characters CharacterID of the folder currently open (NAME_None while on another page). */
	UFUNCTION(BlueprintPure, Category = "Tablet|Family")
	FName GetActiveCharacterID() const { return ActiveCharacterID; }

	/** Opens the default person's folder and its first statement. Returns false when none exists. */
	bool OpenInitialStatement();

#if WITH_EDITOR
	/** Commandlet-created widgets have no local player, so UMG skips NativeOnInitialized. */
	void InitializeForAutomatedTest();
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tablet|Messenger", meta = (ClampMin = "0"))
	int32 UnreadMessageCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Navigation")
	ETabletPage CurrentPage = ETabletPage::Home;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Family")
	FName ActiveCharacterID = NAME_None;

	/** Shared icon shown on every home-page folder button. Assign in the WBP_Tablet class defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Home")
	TObjectPtr<UTexture2D> DefaultFolderIcon;

	/** Font used by runtime-created home-page folder labels. Assign in the WBP_Tablet class defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Home")
	FSlateFontInfo FolderLabelFont;

	/** Icon shown on the folder page's 진술서 tile, so it reads as a document like the photo tiles next to it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Family")
	TObjectPtr<UTexture2D> StatementFileIcon;

	/** Designer templates used for runtime-populated folder contents. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Designer Templates")
	TSoftClassPtr<UBalhwajeomTabletFolderButton> FolderButtonWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Designer Templates")
	TSoftClassPtr<UBalhwajeomTabletPhotoButton> FileTileWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Designer Templates")
	TSoftClassPtr<UBalhwajeomTabletFolderSection> FolderSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Designer Templates")
	TSoftClassPtr<UBalhwajeomTabletDetailWidget> StatementDetailWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Designer Templates")
	TSoftClassPtr<UBalhwajeomTabletDetailWidget> PhotoDetailWidgetClass;

private:
	void SetTabletPage(ETabletPage NewPage, bool bAddToHistory = true);
	void NavigateBack();
	void RefreshHomeFolders();
	void ShowFolder(FName CharacterID);
	void RefreshFolderContents();
	UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;
	void OpenPhoto(FName PhotoID);
	void PreparePuzzle(FName SentenceID);
	void RefreshPuzzleControls();
	void BuildSentenceBuilder(const FSentenceDefinition& Sentence);
	void SetPhotoPuzzleErrorStyle(bool bError);
	void ApplyPopupBodyResultStyle(bool bIsSolvedAnalysisResult);
	void BuildPhotoSlots(const FSentenceDefinition& Sentence);
	void HidePuzzleControls();
	void ValidateActivePuzzle(bool bExplicitStatementSubmit);
	/** Runs ValidateActivePuzzle once every word blank and photo slot has something in it, so
	 * failure feedback ("잘못된 증거인 것 같다") only appears after the puzzle is fully filled in,
	 * not after every single drop. */
	void EvaluatePuzzleIfComplete();
	bool ShowPopup(
		const FText& Title,
		const FText& Body,
		UTexture2D* PhotoTexture = nullptr,
		bool bStatementDetail = false);
	bool ActivateDetailWidget(bool bStatementDetail);
	void BindActiveDetailWidgets();
	void ClearActiveDetailWidgets();
	void HidePopup();
	void UpdateUnreadBadge();

	/** Reveals WB_PuzzlePhotos, listing every eligible evidence photo as a draggable candidate. */
	void OpenPhotoPicker(int32 SlotIndex);
	void ClosePhotoPicker();

	/** Populates WB_PuzzleWords with the active folder's acquired keywords. Called whenever a photo or
	 * statement popup opens, so keywords stay visible whether or not there's an active puzzle to solve;
	 * PreparePuzzle/RefreshPuzzleControls overwrites this with the interactive candidate list when one applies. */
	void RefreshAcquiredWordsDisplay();

	/** Loads (and caches) the PNG a camera capture saved to disk for PhotoID, for folder thumbnails and the detail popup. */
	UTexture2D* GetOrLoadCapturedPhotoTexture(FName PhotoID);

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> TabletUpAnim;

	bool bWaitingForCloseAnimation = false;

	UFUNCTION()
	void HandleHomeFolderSelected(FName CharacterID);

	UFUNCTION()
	void HandleMessengerClicked();

	UFUNCTION()
	void HandleMessengerBackRequested();

	UFUNCTION()
	void HandleMessengerUnreadChanged(int32 TotalUnreadCount);

	UFUNCTION()
	void HandleInternetClicked();

	UFUNCTION()
	void HandleInternetCloseRequested();

	UFUNCTION()
	void HandleMemoClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandlePhysicalHomeClicked();

	UFUNCTION()
	void HandleFolderPhotoSelected(FName PhotoID);

	/** Bound to a UBalhwajeomTabletSentenceBlank's OnBlankDropped; fills that word slot, swapping with
	 * whatever the drag's OriginSlotIndex blank held if it came from another blank (correctness is
	 * judged once the whole puzzle is filled in). */
	UFUNCTION()
	void HandleSentenceBlankDropped(int32 SlotIndex, FName WordID, int32 OriginSlotIndex);

	/** Bound to a UBalhwajeomTabletWordChip's OnWordChipClicked; places the clicked keyword into the
	 * active puzzle's first empty blank (lowest SlotIndex), same as dragging it there. No-op if every
	 * blank is already filled or there is no active blank puzzle (e.g. the folder's plain word list). */
	UFUNCTION()
	void HandleWordChipClicked(FName WordID);

	/** Bound to a UBalhwajeomTabletSentenceBlank's OnBlankClicked; clears that blank's word back out,
	 * same as picking it up and dropping it nowhere. */
	UFUNCTION()
	void HandleSentenceBlankClicked(int32 SlotIndex);

	/** Bound to a UBalhwajeomTabletPhotoSlot's OnPhotoSlotDropped; fills that photo evidence slot. */
	UFUNCTION()
	void HandlePhotoSlotDropped(int32 SlotIndex, FName PhotoID);

	/** Bound to a UBalhwajeomTabletPhotoSlot's OnPhotoSlotClicked; reveals the candidate photo row. */
	UFUNCTION()
	void HandlePhotoSlotClicked(int32 SlotIndex);

	/** Bound to the folder grid's statement tile (reuses UBalhwajeomTabletPhotoButton; the broadcast FName is a SentenceID here, not a PhotoID). */
	UFUNCTION()
	void HandleStatementTileSelected(FName SentenceID);

	UFUNCTION()
	void HandlePopupCloseClicked();

	UFUNCTION()
	void HandleStatementSubmitClicked();

	UFUNCTION()
	void HandlePhotoPickerSelected(FName PhotoID);

	UFUNCTION()
	void HandleUnavailablePhotoPickerSelected(FName PhotoID);

	UFUNCTION()
	void HandlePhotoPickerCloseClicked();

	UFUNCTION()
	void HandlePersonFolderTabRequested(int32 TabIndex);

	UFUNCTION()
	void HandlePhotoPickerTabRequested(int32 TabIndex);

	FName ResolveFolderTabCharacterID(int32 TabIndex) const;
	void PopulatePhotoPicker();

	/** Bound to BTN_PlayStoryVoice; replays the currently open photo's StoryVoice on demand. */
	UFUNCTION()
	void HandlePlayStoryVoiceClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_TabletPage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomMessengerWidget> WBP_Messenger;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomInternetWidget> WBP_Internet;

	/** Full folder page embedded in Page_PersonFolder. Its Designer controls all folder-screen geometry. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomTabletPersonFolderWidget> WBP_PersonFolder;

	/** Outer wrapper moved out of WidgetSwitcher_TabletPage to sit above it as its own overlay
	 * layer, so the Home page underneath (its background art, messenger/internet icons, etc.)
	 * stays visible/rendering around the folder instead of being switched away entirely. Shown
	 * or hidden directly by SetTabletPage instead of a switcher index. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Page_PersonFolder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> PopupLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_FolderTitle;

	/** Small icon next to TXT_FolderTitle, styled like a Windows Explorer window's title-bar icon. Shares DefaultFolderIcon. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_FolderTitleIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PopupTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PopupBody;

	/** TXT_PopupBody's Designer-authored font, cached the moment each fresh detail widget is bound
	 * (see BindActiveDetailWidgets), so OpenPhoto can switch a solved analysis sentence's
	 * ResultText to a different font and still restore this default for every other case (plain
	 * natural-language photos, the statement body, etc). UTextLayoutWidget's Justification has no
	 * public getter to cache the same way, so that default is just the ETextJustify::Center
	 * literal at the OpenPhoto call site -- true for every current TXT_PopupBody instance today. */
	FSlateFontInfo DefaultPopupBodyFont;

	/** Shows the captured PNG for the photo currently open in the popup. Collapsed for non-photo popups (e.g. the statement). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_PopupPhoto;

	/** Shows the active Statement's fixed Illustration, regardless of progress. Collapsed for every other popup. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_StatementIllustration;

	/** On-demand narration replay button. Shown only while a photo with a set StoryVoice is open. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PlayStoryVoice;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_MessengerBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_UnreadMessageCount;

	/** Home page container populated at runtime with one folder button per DT_Characters row. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PersonFolders;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Messenger;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Internet;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Memo;

	/** Windows-Explorer-style "x" close button in the folder window's title bar (still just navigates back). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_FolderClose;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_InternetBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_MemoBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PhysicalHome;

	/** Folder file list container. RefreshFolderContents() clears it and adds up to three
	 * UBalhwajeomTabletFolderSection children (단서와 정보/증거 사진/추억 사진), each holding its own tiles --
	 * replaces the old single flat WB_EvidencePhotos WrapBox + separate SB_StatementTile slot. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SB_EvidencePhotos;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PopupClose;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PuzzleWords;

	/** Interactive sentence row built from SentenceTemplate: static text segments plus one
	 * UBalhwajeomTabletSentenceBlank per "[]". Shown instead of TXT_PopupBody while an unsolved
	 * puzzle is active. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_SentenceBuilder;

	/** Shows "잘못된 증거인 것 같다" briefly after an incorrect drop. Cleared on the next correct drop or popup open. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PuzzleFeedback;

	/** The solved ResultText belonging to the evidence photo currently placed in the statement. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_SelectedPhotoResult;

	/** "증거 사진" label above WB_PuzzlePhotos/WB_PhotoSlots. Only shown alongside them, when the
	 * active sentence actually requires photo evidence (Sentence.PhotoSlots non-empty) -- never for
	 * a plain photo popup. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PuzzlePhotoLabel;

	/** Draggable photo-evidence candidates: captured photos whose own analysis sentence is solved.
	 * Hidden until an empty WB_PhotoSlots slot is clicked (see OpenPhotoPicker) instead of always shown. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PuzzlePhotos;

	/** Droppable photo-evidence slots (FSentencePhotoSlot), one per required/optional photo slot. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PhotoSlots;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_StatementSubmit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PuzzleKeywordCount;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PhotoPickerPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PhotoPickerClose;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomTabletPersonFolderWidget> PhotoPickerFolder;

	TArray<ETabletPage> PageHistory;
	TArray<FName> VisiblePhotoIDs;
	TArray<FName> VisibleStatementIDs;
	/** PhotoID currently shown in the popup (NAME_None for the statement popup or when closed); drives BTN_PlayStoryVoice. */
	FName ActivePhotoID = NAME_None;
	FName ActiveSentenceID = NAME_None;
	FSentenceSubmission ActiveSubmission;
	TArray<FName> AvailablePuzzleWordIDs;
	int32 PendingPhotoSlotIndex = INDEX_NONE;
	FName PhotoPickerCharacterID = NAME_None;

	/** Blank widgets for the sentence currently open, keyed by SlotIndex. Rebuilt each PreparePuzzle. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBalhwajeomTabletSentenceBlank>> ActiveBlanksBySlot;

	/** Generated static fragments kept separately because explicit line breaks still use nested layout state. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ActiveSentenceSegments;

	/** Photo evidence slot widgets for the sentence currently open, keyed by SlotIndex. Rebuilt each PreparePuzzle. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBalhwajeomTabletPhotoSlot>> ActivePhotoSlotsBySlot;

	/** PhotoID -> decoded PNG, so reopening a folder/photo doesn't re-read the file from disk. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> CapturedPhotoTextureCache;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomTabletDetailWidget> ActiveDetailWidget;
};
