#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "BalhwajeomInternetKeywordWidget.generated.h"

class UBalhwajeomInvestigationSubsystem;
class UButton;
class UTextBlock;

/** Clickable emphasized word embedded in an offline Internet page. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomInternetKeywordWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Internet")
	void SetupKeyword(FName InWordID, FName InSourceID, const FText& InDisplayText);

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	FName GetWordID() const { return WordID; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Internet")
	bool IsKeywordAcquired() const { return bAcquired; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;
	void RefreshVisualState();

	UFUNCTION()
	void HandleKeywordClicked();

	UFUNCTION()
	void HandleWordAcquired(const FAcquiredWordRecord& WordRecord);

	UPROPERTY(Transient)
	FName WordID = NAME_None;

	UPROPERTY(Transient)
	FName SourceID = NAME_None;

	UPROPERTY(Transient)
	FText DisplayText;

	UPROPERTY(Transient)
	bool bAcquired = false;

	UPROPERTY(EditDefaultsOnly, Category = "Tablet|Internet")
	FLinearColor AvailableColor = FLinearColor(0.10f, 0.42f, 0.92f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Tablet|Internet")
	FLinearColor AcquiredColor = FLinearColor(0.38f, 0.43f, 0.48f, 1.0f);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Keyword;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Keyword;
};
