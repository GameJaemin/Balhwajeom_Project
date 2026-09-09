#include "Misc/AutomationTest.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/DataTable.h"
#include "Investigation/WordDefinitions.h"
#include "Tablet/BalhwajeomInternetKeywordWidget.h"
#include "Tablet/BalhwajeomInternetPageWidget.h"
#include "Tablet/BalhwajeomInternetTabWidget.h"
#include "Tablet/BalhwajeomInternetWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomInternetWidgetAssetTest,
	"Balhwajeom.Tablet.Internet.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomInternetWidgetAssetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	struct FExpectedAsset
	{
		const TCHAR* Path;
		UClass* ParentClass;
	};

	const FExpectedAsset ExpectedAssets[] = {
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_Internet.WBP_Internet"), UBalhwajeomInternetWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetTab.WBP_InternetTab"), UBalhwajeomInternetTabWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetKeyword.WBP_InternetKeyword"), UBalhwajeomInternetKeywordWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Main.WBP_InternetPage_Main"), UBalhwajeomInternetPageWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Weather.WBP_InternetPage_Weather"), UBalhwajeomInternetPageWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News1.WBP_InternetPage_News1"), UBalhwajeomInternetPageWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News2.WBP_InternetPage_News2"), UBalhwajeomInternetPageWidget::StaticClass()},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Ad.WBP_InternetPage_Ad"), UBalhwajeomInternetPageWidget::StaticClass()}
	};

	for (const FExpectedAsset& Expected : ExpectedAssets)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Expected.Path);
		TestNotNull(FString::Printf(TEXT("Asset exists: %s"), Expected.Path), Blueprint);
		if (Blueprint)
		{
			TestTrue(
				FString::Printf(TEXT("Parent class matches: %s"), Expected.Path),
				Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(Expected.ParentClass));
		}
	}

	UWidgetBlueprint* MainPage = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Main.WBP_InternetPage_Main"));
	if (MainPage && MainPage->WidgetTree)
	{
		for (const TCHAR* LinkName : {
			TEXT("BTN_OpenWeather"), TEXT("BTN_OpenNews1"), TEXT("BTN_OpenNews2"), TEXT("BTN_OpenAd")})
		{
			TestNotNull(
				FString::Printf(TEXT("Main link exists: %s"), LinkName),
				Cast<UButton>(MainPage->WidgetTree->FindWidget(LinkName)));
		}
	}

	UWidgetBlueprint* Internet = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_Internet.WBP_Internet"));
	if (Internet && Internet->WidgetTree)
	{
		TestNotNull(TEXT("Browser window exists"), Cast<USizeBox>(Internet->WidgetTree->FindWidget(TEXT("SizeBox_BrowserWindow"))));
		TestNotNull(TEXT("Draggable title bar exists"), Cast<UBorder>(Internet->WidgetTree->FindWidget(TEXT("BRD_TitleBar"))));
		TestNotNull(TEXT("Tab row exists"), Cast<UHorizontalBox>(Internet->WidgetTree->FindWidget(TEXT("HB_TabBar"))));
		TestNotNull(TEXT("Page switcher exists"), Cast<UWidgetSwitcher>(Internet->WidgetTree->FindWidget(TEXT("WS_PageContent"))));
		TestNotNull(TEXT("Maximize button exists"), Cast<UButton>(Internet->WidgetTree->FindWidget(TEXT("BTN_Maximize"))));
		TestNotNull(TEXT("Close button exists"), Cast<UButton>(Internet->WidgetTree->FindWidget(TEXT("BTN_Close"))));
	}

	UWidgetBlueprint* News1 = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News1.WBP_InternetPage_News1"));
	if (News1 && News1->WidgetTree)
	{
		TestNotNull(
			TEXT("News1 contains the related-agency keyword widget"),
			Cast<UBalhwajeomInternetKeywordWidget>(
				News1->WidgetTree->FindWidget(TEXT("WBP_Keyword_RelatedAgency"))));
	}

	UDataTable* Words = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_Words.DT_Words"));
	TestNotNull(TEXT("DT_Words exists"), Words);
	if (Words)
	{
		const FWordDefinition* Word = Words->FindRow<FWordDefinition>(
			TEXT("WORD_RELATED_AGENCY"), TEXT("Internet asset test"), false);
		TestNotNull(TEXT("Related-agency keyword row exists"), Word);
		if (Word)
		{
			TestEqual(TEXT("Keyword display text"), Word->DisplayWord.ToString(), FString(TEXT("관계기관")));
		}
	}
	return true;
}

#endif
