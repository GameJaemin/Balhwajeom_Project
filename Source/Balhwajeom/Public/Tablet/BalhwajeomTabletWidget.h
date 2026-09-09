#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "BalhwajeomTabletWidget.generated.h"

class UBorder;
class UBalhwajeomMessengerWidget;
class UButton;
class UOverlay;
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

/** Runtime-created photo entry shared by the folder and statement candidate lists. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTabletPhotoButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(FName InPhotoID, const FText& InLabel);

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
	void ShowPopup(const FText& Title, const FText& Body);
	void HidePopup();
	void UpdateUnreadBadge();

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

	UFUNCTION()
	void HandleStatementClicked();

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PopupTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_PopupBody;

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_FolderBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_InternetBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_MemoBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PhysicalHome;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_EvidencePhotos;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_AcquiredWords;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_EvidenceStatement;

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
};
