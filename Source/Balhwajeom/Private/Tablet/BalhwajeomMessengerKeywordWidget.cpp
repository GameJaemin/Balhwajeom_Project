#include "Tablet/BalhwajeomMessengerKeywordWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"

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
		BTN_Keyword->SetIsEnabled(!WordID.IsEmpty());
	}
}

void UBalhwajeomMessengerKeywordWidget::HandleKeywordClicked()
{
	if (WordID.IsEmpty())
	{
		return;
	}

	UKismetSystemLibrary::PrintString(this, TEXT("단서"), true, true);
	OnKeywordClicked.Broadcast(WordID);
}
