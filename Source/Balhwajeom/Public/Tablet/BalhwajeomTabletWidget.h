#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "BalhwajeomTabletWidget.generated.h"

class UBorder;
class UBalhwajeomMessengerWidget;
class UButton;
class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWrapBox;
class UWidgetSwitcher;
class UWidgetAnimation;
class UBalhwajeomInvestigationSubsystem;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletWordSelected, FName, WordID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabletFolderSelected, FName, CharacterID);

/** Runtime-created photo entry shared by the folder grid, the statement tile, and the puzzle candidate lists. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoButton : public UButton
{
	GENERATED_BODY()

public:
	/** Thumbnail is optional: puzzle-candidate entries and the statement tile pass nullptr for a text-only tile. */
	void Configure(FName InPhotoID, const FText& InLabel, UTexture2D* Thumbnail = nullptr);

	UPROPERTY()
	FOnTabletPhotoSelected OnPhotoSelected;

private:
	UFUNCTION()
	void HandleClicked();

	FName PhotoID = NAME_None;
};

/** Runtime-created keyword entry used by statement and photo-analysis puzzles. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletWordButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(FName InWordID, const FText& InLabel);

	UPROPERTY()
	FOnTabletWordSelected OnWordSelected;

private:
	UFUNCTION()
	void HandleClicked();

	FName WordID = NAME_None;
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
	void HidePuzzleControls();
	void SelectPuzzleWord(int32 Index);
	void SelectPuzzlePhoto(FName PhotoID);
	void ValidateActivePuzzle(bool bExplicitStatementSubmit);
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
	void HandleMemoClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandlePhysicalHomeClicked();

	UFUNCTION()
	void HandleFolderPhotoSelected(FName PhotoID);

	UFUNCTION()
	void HandlePuzzlePhotoSelected(FName PhotoID);

	UFUNCTION()
	void HandlePuzzleWordSelected(FName WordID);

	/** Bound to the folder grid's statement tile (reuses UBalhwajeomTabletPhotoButton; the broadcast FName is a SentenceID here, not a PhotoID). */
	UFUNCTION()
	void HandleStatementTileSelected(FName SentenceID);

	UFUNCTION()
	void HandlePopupCloseClicked();

	UFUNCTION()
	void HandleStatementSubmitClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_TabletPage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomMessengerWidget> WBP_Messenger;

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_EvidencePhotos;

	/** Single-slot container pinned at the bottom-center of the folder window, holding the statement tile. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SB_StatementTile;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PopupClose;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PuzzleWords;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_PuzzlePhotos;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_StatementSubmit;

	TArray<ETabletPage> PageHistory;
	TArray<FName> VisiblePhotoIDs;
	TArray<FName> VisibleStatementIDs;
	FName ActiveSentenceID = NAME_None;
	FSentenceSubmission ActiveSubmission;
	TArray<FName> AvailablePuzzleWordIDs;
	TArray<FName> AvailablePuzzlePhotoIDs;
	int32 NextWordSlotCursor = 0;
	int32 NextPhotoSlotCursor = 0;

	/** PhotoID -> decoded PNG, so reopening a folder/photo doesn't re-read the file from disk. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> CapturedPhotoTextureCache;
};
