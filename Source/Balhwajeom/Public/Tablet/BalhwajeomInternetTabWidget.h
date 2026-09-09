#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tablet/BalhwajeomInternetTypes.h"
#include "BalhwajeomInternetTabWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FInternetTabSelectedSignature,
	EBalhwajeomInternetPage,
	Page);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FInternetTabCloseRequestedSignature,
	EBalhwajeomInternetPage,
	Page);

/** Runtime tab row created by UBalhwajeomInternetWidget. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomInternetTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void SetupTab(EBalhwajeomInternetPage InPageID, bool bInSelected);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	EBalhwajeomInternetPage GetPageID() const { return PageID; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	bool IsCloseable() const { return bCloseable; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Internet")
	FInternetTabSelectedSignature OnTabSelected;

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Internet")
	FInternetTabCloseRequestedSignature OnTabCloseRequested;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

private:
	void RefreshVisualState();

	UFUNCTION()
	void HandleTabClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(Transient)
	EBalhwajeomInternetPage PageID = EBalhwajeomInternetPage::Main;

	UPROPERTY(Transient)
	bool bSelected = false;

	UPROPERTY(Transient)
	bool bCloseable = false;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Tab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_CloseTab;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_TabTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_Selected;
};
