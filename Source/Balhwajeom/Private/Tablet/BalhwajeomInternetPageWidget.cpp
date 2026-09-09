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
	if (!IsValidInternetPage(InPageID))
	{
		return;
	}

	PageID = InPageID;
	if (PageID == EBalhwajeomInternetPage::Weather)
	{
		if (WBP_Keyword_Cloud)
		{
			WBP_Keyword_Cloud->SetupKeyword(
				TEXT("WORD_01_014"), TEXT("WEB_WEATHER"), FText::FromString(TEXT("구름")));
		}
		if (WBP_Keyword_Clear)
		{
			WBP_Keyword_Clear->SetupKeyword(
				TEXT("WORD_01_015"), TEXT("WEB_WEATHER"), FText::FromString(TEXT("맑음")));
		}
	}
	else if (PageID == EBalhwajeomInternetPage::News1 && WBP_Keyword_Ignition)
	{
		WBP_Keyword_Ignition->SetupKeyword(
			TEXT("WORD_01_019"), TEXT("WEB_NEWS_01"), FText::FromString(TEXT("발화")));
	}
	else if (PageID == EBalhwajeomInternetPage::News2 && WBP_Keyword_BurnedOut)
	{
		WBP_Keyword_BurnedOut->SetupKeyword(
			TEXT("WORD_01_020"), TEXT("WEB_NEWS_02"), FText::FromString(TEXT("전소")));
	}
	else if (PageID == EBalhwajeomInternetPage::Ad)
	{
		if (WBP_Keyword_Light)
		{
			WBP_Keyword_Light->SetupKeyword(
				TEXT("WORD_01_016"), TEXT("WEB_FIRE_PSA"), FText::FromString(TEXT("빛")));
		}
		if (WBP_Keyword_Fire)
		{
			WBP_Keyword_Fire->SetupKeyword(
				TEXT("WORD_01_017"), TEXT("WEB_FIRE_PSA"), FText::FromString(TEXT("화재")));
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
