#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMessengerKeywordWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMessengerKeywordClickedSignature,
	const FString&,
	WordID);

/** Displays and activates one valid WordID-backed clue keyword. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomMessengerKeywordWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetupKeyword(const FText& InDisplayText, const FString& InWordID);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	const FString& GetWordID() const { return WordID; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerKeywordClickedSignature OnKeywordClicked;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleKeywordClicked();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FText DisplayText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FString WordID;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Keyword;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Keyword;
};
