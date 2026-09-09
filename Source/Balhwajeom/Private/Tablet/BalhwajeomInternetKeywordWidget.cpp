#include "Tablet/BalhwajeomInternetKeywordWidget.h"

#include "Balhwajeom.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/WordDefinitions.h"

void UBalhwajeomInternetKeywordWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Keyword)
	{
		BTN_Keyword->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleKeywordClicked);
	}
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		Investigation->OnWordAcquired.AddUniqueDynamic(this, &ThisClass::HandleWordAcquired);
	}
	RefreshVisualState();
}

void UBalhwajeomInternetKeywordWidget::NativeDestruct()
{
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		Investigation->OnWordAcquired.RemoveDynamic(this, &ThisClass::HandleWordAcquired);
	}
	Super::NativeDestruct();
}

void UBalhwajeomInternetKeywordWidget::SetupKeyword(
	const FName InWordID,
	const FName InSourceID,
	const FText& InDisplayText)
{
	WordID = InWordID;
	SourceID = InSourceID;
	DisplayText = InDisplayText;
	RefreshVisualState();
}

UBalhwajeomInvestigationSubsystem* UBalhwajeomInternetKeywordWidget::GetInvestigationSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
}

void UBalhwajeomInternetKeywordWidget::RefreshVisualState()
{
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		bAcquired = !WordID.IsNone() && Investigation->HasAcquiredWord(WordID);
	}
	if (TXT_Keyword)
	{
		TXT_Keyword->SetText(DisplayText);
		TXT_Keyword->SetColorAndOpacity(FSlateColor(bAcquired ? AcquiredColor : AvailableColor));
	}
	if (BTN_Keyword)
	{
		BTN_Keyword->SetIsEnabled(!WordID.IsNone() && !bAcquired);
	}
}

void UBalhwajeomInternetKeywordWidget::HandleKeywordClicked()
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || WordID.IsNone())
	{
		return;
	}

	FWordDefinition Definition;
	if (!Investigation->GetWordDefinition(WordID, Definition))
	{
		UE_LOG(LogBalhwajeom, Error, TEXT("Internet keyword '%s' is not defined in DT_Words."), *WordID.ToString());
		return;
	}

	if (Investigation->AcquireWord(WordID, EWordAcquisitionSource::Browser, SourceID))
	{
		UE_LOG(
			LogBalhwajeom,
			Display,
			TEXT("Internet keyword acquired: WordID=%s SourceID=%s"),
			*WordID.ToString(),
			*SourceID.ToString());
	}
	RefreshVisualState();
}

void UBalhwajeomInternetKeywordWidget::HandleWordAcquired(const FAcquiredWordRecord& WordRecord)
{
	if (WordRecord.WordID == WordID)
	{
		RefreshVisualState();
	}
}
