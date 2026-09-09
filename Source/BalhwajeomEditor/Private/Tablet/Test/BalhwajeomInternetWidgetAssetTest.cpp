#include "Misc/AutomationTest.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Investigation/WordDefinitions.h"
#include "Tablet/BalhwajeomInternetKeywordWidget.h"
#include "Tablet/BalhwajeomInternetPageWidget.h"
#include "Tablet/BalhwajeomInternetTabWidget.h"
#include "Tablet/BalhwajeomInternetWidget.h"
#include "Tablet/BalhwajeomMessengerDataAssets.h"
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

	struct FExpectedPageKeyword
	{
		const TCHAR* PagePath;
		const TCHAR* WidgetName;
		EBalhwajeomInternetPage PageID;
		const TCHAR* WordID;
	};
	const FExpectedPageKeyword ExpectedPageKeywords[] = {
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Weather.WBP_InternetPage_Weather"), TEXT("WBP_Keyword_Cloud"), EBalhwajeomInternetPage::Weather, TEXT("WORD_01_014")},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Weather.WBP_InternetPage_Weather"), TEXT("WBP_Keyword_Clear"), EBalhwajeomInternetPage::Weather, TEXT("WORD_01_015")},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News1.WBP_InternetPage_News1"), TEXT("WBP_Keyword_Ignition"), EBalhwajeomInternetPage::News1, TEXT("WORD_01_019")},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News2.WBP_InternetPage_News2"), TEXT("WBP_Keyword_BurnedOut"), EBalhwajeomInternetPage::News2, TEXT("WORD_01_020")},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Ad.WBP_InternetPage_Ad"), TEXT("WBP_Keyword_Light"), EBalhwajeomInternetPage::Ad, TEXT("WORD_01_016")},
		{TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Ad.WBP_InternetPage_Ad"), TEXT("WBP_Keyword_Fire"), EBalhwajeomInternetPage::Ad, TEXT("WORD_01_017")}
	};
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Editor world exists for Internet page setup"), EditorWorld);
	for (const FExpectedPageKeyword& Expected : ExpectedPageKeywords)
	{
		UWidgetBlueprint* Page = LoadObject<UWidgetBlueprint>(nullptr, Expected.PagePath);
		TestNotNull(FString::Printf(TEXT("Keyword page exists: %s"), Expected.PagePath), Page);
		if (Page && Page->WidgetTree)
		{
			UBalhwajeomInternetKeywordWidget* Keyword =
				Cast<UBalhwajeomInternetKeywordWidget>(Page->WidgetTree->FindWidget(Expected.WidgetName));
			TestNotNull(
				FString::Printf(TEXT("Keyword widget exists: %s"), Expected.WidgetName),
				Keyword);
			if (EditorWorld && Page->GeneratedClass)
			{
				const TSubclassOf<UUserWidget> PageClass(Page->GeneratedClass.Get());
				UBalhwajeomInternetPageWidget* PageInstance =
					CreateWidget<UBalhwajeomInternetPageWidget>(EditorWorld, PageClass);
				TestNotNull(TEXT("Internet page instance can be created"), PageInstance);
				if (PageInstance)
				{
					PageInstance->SetupPage(Expected.PageID);
					UBalhwajeomInternetKeywordWidget* KeywordInstance =
						Cast<UBalhwajeomInternetKeywordWidget>(
							PageInstance->GetWidgetFromName(Expected.WidgetName));
					TestNotNull(TEXT("Internet keyword instance can be created"), KeywordInstance);
					if (KeywordInstance)
					{
						TestEqual(
							FString::Printf(TEXT("Internet keyword binding: %s"), Expected.WidgetName),
							KeywordInstance->GetWordID(),
							FName(Expected.WordID));
					}
				}
			}
		}
	}

	UDataTable* Words = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_Words.DT_Words"));
	TestNotNull(TEXT("DT_Words exists"), Words);
	if (Words)
	{
		struct FExpectedWord
		{
			const TCHAR* WordID;
			const TCHAR* DisplayWord;
		};
		const FExpectedWord ExpectedWords[] = {
			{TEXT("WORD_01_013"), TEXT("스노우글로브")},
			{TEXT("WORD_01_014"), TEXT("구름")},
			{TEXT("WORD_01_015"), TEXT("맑음")},
			{TEXT("WORD_01_016"), TEXT("빛")},
			{TEXT("WORD_01_017"), TEXT("화재")},
			{TEXT("WORD_01_019"), TEXT("발화")},
			{TEXT("WORD_01_020"), TEXT("전소")}
		};
		for (const FExpectedWord& Expected : ExpectedWords)
		{
			const FWordDefinition* Word = Words->FindRow<FWordDefinition>(
				Expected.WordID, TEXT("Tablet keyword asset test"), false);
			TestNotNull(FString::Printf(TEXT("Word row exists: %s"), Expected.WordID), Word);
			if (Word)
			{
				TestEqual(
					FString::Printf(TEXT("Word display text: %s"), Expected.WordID),
					Word->DisplayWord.ToString(),
					FString(Expected.DisplayWord));
			}
		}
	}

	UBalhwajeomMessengerCatalogDataAsset* MessengerCatalog =
		LoadObject<UBalhwajeomMessengerCatalogDataAsset>(
			nullptr,
			TEXT("/Game/Balhwajeom/Data/Messenger/DA_MessengerCatalog.DA_MessengerCatalog"));
	TestNotNull(TEXT("Messenger catalog exists"), MessengerCatalog);
	if (MessengerCatalog)
	{
		int32 KeywordMessageCount = 0;
		for (const UBalhwajeomMessengerRoomDataAsset* Room : MessengerCatalog->Rooms)
		{
			if (!Room)
			{
				continue;
			}
			for (const FST_MessengerMessage& Message : Room->Messages)
			{
				if (Message.WordID.IsEmpty() && Message.KeywordText.IsEmpty())
				{
					continue;
				}
				++KeywordMessageCount;
				TestEqual(TEXT("Messenger keyword WordID"), Message.WordID, FString(TEXT("WORD_01_013")));
				TestEqual(TEXT("Messenger keyword text"), Message.KeywordText.ToString(), FString(TEXT("스노우 글로브")));
				TestEqual(
					TEXT("Messenger keyword message content remains unchanged"),
					Message.Message.ToString(),
					FString(TEXT("내 생일에 스노우 글로브 사준다고 했잖아")));
			}
		}
		TestEqual(TEXT("Only the snow-globe messenger keyword remains interactive"), KeywordMessageCount, 1);
	}
	return true;
}

#endif
