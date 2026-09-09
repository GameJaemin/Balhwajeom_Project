#include "Tablet/BalhwajeomInternetPageWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Tablet/BalhwajeomInternetKeywordWidget.h"

void UBalhwajeomInternetPageWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (SB_PageContent)
	{
		SB_PageContent->OnUserScrolled.AddUniqueDynamic(this, &ThisClass::HandleUserScrolled);
		SB_PageContent->SetScrollOffset(SavedScrollOffset);
	}
	if (BTN_OpenWeather)
	{
		BTN_OpenWeather->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleWeatherClicked);
	}
	if (BTN_OpenNews1)
	{
		BTN_OpenNews1->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleNews1Clicked);
	}
	if (BTN_OpenNews2)
	{
		BTN_OpenNews2->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleNews2Clicked);
	}
	if (BTN_OpenAd)
	{
		BTN_OpenAd->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleAdClicked);
	}
}

void UBalhwajeomInternetPageWidget::SetupPage(const EBalhwajeomInternetPage InPageID)
{
	if (IsValidInternetPage(InPageID))
	{
		PageID = InPageID;
		if (PageID == EBalhwajeomInternetPage::News1 && WBP_Keyword_RelatedAgency)
		{
			WBP_Keyword_RelatedAgency->SetupKeyword(
				TEXT("WORD_RELATED_AGENCY"),
				TEXT("WEB_NEWS_01"),
				FText::FromString(TEXT("관계기관")));
		}
	}
}

void UBalhwajeomInternetPageWidget::SetSavedScrollOffset(const float InOffset)
{
	SavedScrollOffset = FMath::Max(0.0f, InOffset);
	if (SB_PageContent)
	{
		SB_PageContent->SetScrollOffset(SavedScrollOffset);
	}
}

void UBalhwajeomInternetPageWidget::RequestPage(const EBalhwajeomInternetPage Page)
{
	if (IsValidInternetPage(Page))
	{
		OnPageLinkRequested.Broadcast(Page);
	}
}

void UBalhwajeomInternetPageWidget::HandleWeatherClicked()
{
	RequestPage(EBalhwajeomInternetPage::Weather);
}

void UBalhwajeomInternetPageWidget::HandleNews1Clicked()
{
	RequestPage(EBalhwajeomInternetPage::News1);
}

void UBalhwajeomInternetPageWidget::HandleNews2Clicked()
{
	RequestPage(EBalhwajeomInternetPage::News2);
}

void UBalhwajeomInternetPageWidget::HandleAdClicked()
{
	RequestPage(EBalhwajeomInternetPage::Ad);
}

void UBalhwajeomInternetPageWidget::HandleUserScrolled(const float CurrentOffset)
{
	SavedScrollOffset = FMath::Max(0.0f, CurrentOffset);
}
