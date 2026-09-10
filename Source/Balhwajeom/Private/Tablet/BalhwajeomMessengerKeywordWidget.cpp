#include "Tablet/BalhwajeomMessengerKeywordWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UBalhwajeomMessengerKeywordWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Keyword)
	{
		BTN_Keyword->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleKeywordClicked);
	}
}

void UBalhwajeomMessengerKeywordWidget::SetupKeyword(
	const FText& InDisplayText,
	const FString& InWordID)
{
	DisplayText = InDisplayText;
	WordID = InWordID;

	if (TXT_Keyword)
	{
		TXT_Keyword->SetText(DisplayText);
	}
	if (BTN_Keyword)
	{
		BTN_Keyword->SetIsEnabled(!WordID.IsEmpty() && !bAcquired);
	}
}

void UBalhwajeomMessengerKeywordWidget::SetAcquired(const bool bInAcquired)
{
	bAcquired = bInAcquired;
	if (BTN_Keyword)
	{
		BTN_Keyword->SetIsEnabled(!WordID.IsEmpty() && !bAcquired);
	}
	if (TXT_Keyword)
	{
		TXT_Keyword->SetOpacity(bAcquired ? 0.55f : 1.0f);
	}
}

void UBalhwajeomMessengerKeywordWidget::HandleKeywordClicked()
{
	if (WordID.IsEmpty())
	{
		return;
	}

	OnKeywordClicked.Broadcast(WordID);
}
