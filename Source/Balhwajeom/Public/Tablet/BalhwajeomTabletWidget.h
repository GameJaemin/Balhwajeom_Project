#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Investigation/SentenceDefinitions.h"
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
/** OriginSlotIndex is the sentence blank the word was dragged out of (INDEX_NONE if it came from the acquired-keyword list instead). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTabletBlankDropped, int32, SlotIndex, FName, WordID, int32, OriginSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTabletPhotoSlotDropped, int32, SlotIndex, FName, PhotoID);

/** Runtime-created photo entry shared by the folder grid and the statement tile. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoButton : public UButton
{
	GENERATED_BODY()

public:
	/** Thumbnail is optional: the statement tile passes nullptr for a text-only tile. */
	void Configure(FName InPhotoID, const FText& InLabel, UTexture2D* Thumbnail = nullptr);

	UPROPERTY()
	FOnTabletPhotoSelected OnPhotoSelected;

private:
	UFUNCTION()
	void HandleClicked();

	FName PhotoID = NAME_None;
};

/** Runtime-created home-page entry for one DT_Characters folder. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletFolderButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(FName InCharacterID, const FText& InLabel, UTexture2D* IconTexture);

	UPROPERTY()
	FOnTabletFolderSelected OnFolderSelected;

private:
	UFUNCTION()
	void HandleClicked();

	FName CharacterID = NAME_None;
};

/**
 * Collapsible group header for the folder page's file list (like Windows Explorer's date groups):
 * a clickable title row ("{Title} ({Count})" with a ▼/▶ fold arrow) above a WrapBox of file tiles.
 * Built entirely at runtime; RefreshFolderContents() clears and rebuilds one of these per bucket
 * (진술서/분석 문장/완성 문장) every time a folder is opened.
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

private:
	UFUNCTION()
	void HandleHeaderClicked();

	void RefreshHeaderText();

	FText Title;
	int32 TileCount = 0;
	bool bExpanded = true;

	UPROPERTY()
	TObjectPtr<UButton> HeaderButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> ArrowText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UWrapBox> ContentWrapBox;
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
	void Configure(FName InWordID, const FText& InLabel);
	FName GetWordID() const { return WordID; }

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	FName WordID = NAME_None;
	FText DisplayLabel;
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
	void Configure(int32 InSlotIndex);
	void SetFilled(FName InWordID, const FText& WordText);
	void SetEmpty();
	int32 GetSlotIndex() const { return SlotIndex; }

	UPROPERTY()
	FOnTabletBlankDropped OnBlankDropped;

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	int32 SlotIndex = 0;
	FName FilledWordID = NAME_None;

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
	void Configure(FName InPhotoID, const FText& InLabel);
	FName GetPhotoID() const { return PhotoID; }

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	FName PhotoID = NAME_None;
	FText DisplayLabel;
};

/** One droppable photo-evidence slot (FSentencePhotoSlot) inside the active sentence's photo evidence row. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(int32 InSlotIndex);
	void SetFilled(const FText& PhotoLabel);
	void SetEmpty();
	int32 GetSlotIndex() const { return SlotIndex; }

	UPROPERTY()
	FOnTabletPhotoSlotDropped OnPhotoSlotDropped;

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	int32 SlotIndex = 0;

	UPROPERTY()
	TObjectPtr<UTextBlock> DisplayText;
};

/** Navigation/state logic for the designer-owned WBP_Tablet visual tree. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletWidget : public UUserWidget
{
	GENERATED_BODY()

public:
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

	/** Icon shown on the folder page's 진술서 tile, so it reads as a document like the photo tiles next to it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Family")
	TObjectPtr<UTexture2D> StatementFileIcon;

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
	void BuildPhotoSlots(const FSentenceDefinition& Sentence);
	void HidePuzzleControls();
	void ValidateActivePuzzle(bool bExplicitStatementSubmit);
	/** Runs ValidateActivePuzzle once every word blank and photo slot has something in it, so
	 * failure feedback ("잘못된 증거인 것 같다") only appears after the puzzle is fully filled in,
	 * not after every single drop. */
	void EvaluatePuzzleIfComplete();
	void ShowPopup(const FText& Title, const FText& Body, UTexture2D* PhotoTexture = nullptr);
	void HidePopup();
	void UpdateUnreadBadge();

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

	/** Bound to a UBalhwajeomTabletPhotoSlot's OnPhotoSlotDropped; fills that photo evidence slot. */
	UFUNCTION()
	void HandlePhotoSlotDropped(int32 SlotIndex, FName PhotoID);

	/** Bound to the folder grid's statement tile (reuses UBalhwajeomTabletPhotoButton; the broadcast FName is a SentenceID here, not a PhotoID). */
	UFUNCTION()
	void HandleStatementTileSelected(FName SentenceID);

	UFUNCTION()
	void HandlePopupCloseClicked();

	UFUNCTION()
	void HandleStatementSubmitClicked();

	/** Bound to BTN_PlayStoryVoice; replays the currently open photo's StoryVoice on demand. */
	UFUNCTION()
	void HandlePlayStoryVoiceClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_TabletPage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomMessengerWidget> WBP_Messenger;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomInternetWidget> WBP_Internet;

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

	/** Shows the captured PNG for the photo currently open in the popup. Collapsed for non-photo popups (e.g. the statement). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_PopupPhoto;

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
	 * UBalhwajeomTabletFolderSection children (진술서/분석 문장/완성 문장), each holding its own tiles --
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

	/** Draggable photo-evidence candidates: captured photos whose own analysis sentence is solved.
	 * Only populated/shown while the active sentence has PhotoSlots. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PuzzlePhotos;

	/** Droppable photo-evidence slots (FSentencePhotoSlot), one per required/optional photo slot. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PhotoSlots;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_StatementSubmit;

	TArray<ETabletPage> PageHistory;
	TArray<FName> VisiblePhotoIDs;
	TArray<FName> VisibleStatementIDs;
	/** PhotoID currently shown in the popup (NAME_None for the statement popup or when closed); drives BTN_PlayStoryVoice. */
	FName ActivePhotoID = NAME_None;
	FName ActiveSentenceID = NAME_None;
	FSentenceSubmission ActiveSubmission;
	TArray<FName> AvailablePuzzleWordIDs;

	/** Blank widgets for the sentence currently open, keyed by SlotIndex. Rebuilt each PreparePuzzle. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBalhwajeomTabletSentenceBlank>> ActiveBlanksBySlot;

	/** Photo evidence slot widgets for the sentence currently open, keyed by SlotIndex. Rebuilt each PreparePuzzle. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBalhwajeomTabletPhotoSlot>> ActivePhotoSlotsBySlot;

	/** PhotoID -> decoded PNG, so reopening a folder/photo doesn't re-read the file from disk. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> CapturedPhotoTextureCache;
};
