#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tablet/BalhwajeomInternetTypes.h"
#include "BalhwajeomInternetPageWidget.generated.h"

class UButton;
class UBalhwajeomInternetKeywordWidget;
class UScrollBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FInternetPageLinkRequestedSignature,
	EBalhwajeomInternetPage,
	Page);

/** Common parent for every authored offline browser page. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomInternetPageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void SetupPage(EBalhwajeomInternetPage InPageID);

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	EBalhwajeomInternetPage GetPageID() const { return PageID; }

	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void SetSavedScrollOffset(float InOffset);

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	float GetSavedScrollOffset() const { return SavedScrollOffset; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Internet")
	FInternetPageLinkRequestedSignature OnPageLinkRequested;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

private:
	void RequestPage(EBalhwajeomInternetPage Page);

	UFUNCTION()
	void HandleWeatherClicked();

	UFUNCTION()
	void HandleNews1Clicked();

	UFUNCTION()
	void HandleNews2Clicked();

	UFUNCTION()
	void HandleAdClicked();

	UFUNCTION()
	void HandleUserScrolled(float CurrentOffset);

	UPROPERTY(Transient)
	EBalhwajeomInternetPage PageID = EBalhwajeomInternetPage::Main;

	UPROPERTY(Transient)
	float SavedScrollOffset = 0.0f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SB_PageContent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_OpenWeather;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_OpenNews1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_OpenNews2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_OpenAd;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBalhwajeomInternetKeywordWidget> WBP_Keyword_RelatedAgency;
};
