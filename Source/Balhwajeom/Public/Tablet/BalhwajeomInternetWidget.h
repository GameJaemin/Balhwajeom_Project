#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tablet/BalhwajeomInternetTypes.h"
#include "BalhwajeomInternetWidget.generated.h"

class UBalhwajeomInternetPageWidget;
class UBalhwajeomInternetTabWidget;
class UBorder;
class UButton;
class UHorizontalBox;
class USizeBox;
class UTextBlock;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInternetCloseRequestedSignature);

/** Owns the offline browser window, its tabs, and play-session-only state. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomInternetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomInternetWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void InitializeInternet();

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	bool OpenPage(EBalhwajeomInternetPage Page);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	bool ActivatePage(EBalhwajeomInternetPage Page);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	bool ClosePage(EBalhwajeomInternetPage Page);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void ToggleMaximize();

	/** Called only when the desktop icon opens Internet after its X button closed it. */
	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void PrepareForDesktopOpen();

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void CloseInternetWindow();

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void SetNormalWindowPosition(FVector2D Position);

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	int32 GetOpenTabCount() const { return SessionState.GetOpenPages().Num(); }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	bool IsPageOpen(EBalhwajeomInternetPage Page) const { return SessionState.IsPageOpen(Page); }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	EBalhwajeomInternetPage GetActivePage() const { return SessionState.GetActivePage(); }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	bool IsMaximized() const { return SessionState.IsMaximized(); }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	FVector2D GetNormalWindowPosition() const { return SessionState.GetNormalWindowPosition(); }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Internet")
	FInternetCloseRequestedSignature OnCloseRequested;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	TSubclassOf<UBalhwajeomInternetPageWidget> ResolvePageClass(EBalhwajeomInternetPage Page) const;
	TSubclassOf<UBalhwajeomInternetTabWidget> ResolveTabClass() const;
	UBalhwajeomInternetPageWidget* EnsurePageWidget(EBalhwajeomInternetPage Page);
	void SaveActiveScrollOffset();
	void ShowActivePage();
	void RebuildTabs();
	void ApplyWindowGeometry();
	void RefreshMaximizeLabel();

	UFUNCTION()
	void HandlePageLinkRequested(EBalhwajeomInternetPage Page);

	UFUNCTION()
	void HandleTabSelected(EBalhwajeomInternetPage Page);

	UFUNCTION()
	void HandleTabCloseRequested(EBalhwajeomInternetPage Page);

	UFUNCTION()
	void HandleMaximizeClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	FBalhwajeomInternetSessionState SessionState;

	UPROPERTY(Transient)
	TMap<EBalhwajeomInternetPage, TObjectPtr<UBalhwajeomInternetPageWidget>> PageWidgets;

	UPROPERTY(EditDefaultsOnly, Category = "Tablet|Internet")
	TMap<EBalhwajeomInternetPage, TSoftClassPtr<UBalhwajeomInternetPageWidget>> PageWidgetClasses;

	UPROPERTY(EditDefaultsOnly, Category = "Tablet|Internet")
	TSoftClassPtr<UBalhwajeomInternetTabWidget> TabWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SizeBox_BrowserWindow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_TitleBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_TabBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> WS_PageContent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Maximize;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Maximize;

	bool bSessionInitialized = false;
	bool bDraggingWindow = false;
	FVector2D DragStartMousePosition = FVector2D::ZeroVector;
	FVector2D DragStartWindowPosition = FVector2D::ZeroVector;
};
