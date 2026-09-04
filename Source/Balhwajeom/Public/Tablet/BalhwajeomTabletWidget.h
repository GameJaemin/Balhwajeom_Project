#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomTabletWidget.generated.h"

class UBorder;
class UButton;
class UOverlay;
class UTextBlock;
class UWidgetSwitcher;
class UWidgetAnimation;

UENUM(BlueprintType)
enum class ETabletPage : uint8
{
	Home,
	PersonFolder,
	Messenger,
	Internet,
	Memo
};

UENUM(BlueprintType)
enum class EFamilyMember : uint8
{
	Sister,
	Brother,
	Mother
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

	UFUNCTION(BlueprintPure, Category = "Tablet|Navigation")
	ETabletPage GetCurrentPage() const { return CurrentPage; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Family")
	EFamilyMember GetActiveFamilyMember() const { return ActiveFamilyMember; }

#if WITH_EDITOR
	/** Commandlet-created widgets have no local player, so UMG skips NativeOnInitialized. */
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tablet|Messenger", meta = (ClampMin = "0"))
	int32 UnreadMessageCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Navigation")
	ETabletPage CurrentPage = ETabletPage::Home;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Family")
	EFamilyMember ActiveFamilyMember = EFamilyMember::Sister;

private:
	void SetTabletPage(ETabletPage NewPage, bool bAddToHistory = true);
	void NavigateBack();
	void ShowFolder(EFamilyMember FamilyMember);
	void ShowPopup(const FText& Title, const FText& Body);
	void HidePopup();
	void UpdateUnreadBadge();

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> TabletUpAnim;

	bool bWaitingForCloseAnimation = false;

	UFUNCTION()
	void HandleSisterClicked();

	UFUNCTION()
	void HandleBrotherClicked();

	UFUNCTION()
	void HandleMotherClicked();

	UFUNCTION()
	void HandleMessengerClicked();

	UFUNCTION()
	void HandleInternetClicked();

	UFUNCTION()
	void HandleMemoClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandlePhysicalHomeClicked();

	UFUNCTION()
	void HandlePhoto01Clicked();

	UFUNCTION()
	void HandlePhoto02Clicked();

	UFUNCTION()
	void HandleStatementClicked();

	UFUNCTION()
	void HandlePopupCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_TabletPage;

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Sister;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Brother;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Mother;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Messenger;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Internet;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Memo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_FolderBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_MessengerBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_InternetBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_MemoBack;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PhysicalHome;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_EvidencePhoto01;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_EvidencePhoto02;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_EvidenceStatement;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_PopupClose;

	TArray<ETabletPage> PageHistory;
};
