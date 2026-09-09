#include "Tablet/BalhwajeomInternetTypes.h"
#include "Tablet/BalhwajeomInternetPageWidget.h"
#include "Tablet/BalhwajeomInternetTabWidget.h"
#include "Tablet/BalhwajeomInternetWidget.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomInternetSessionTest,
	"Balhwajeom.Tablet.Internet.Session",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomInternetSessionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FBalhwajeomInternetSessionState State;
	State.Reset();

	TestEqual(TEXT("Main is the only default tab"), State.GetOpenPages().Num(), 1);
	TestEqual(
		TEXT("Main is the default active page"),
		State.GetActivePage(),
		EBalhwajeomInternetPage::Main);
	TestTrue(TEXT("Main starts open"), State.IsPageOpen(EBalhwajeomInternetPage::Main));
	TestFalse(TEXT("Main cannot close"), State.ClosePage(EBalhwajeomInternetPage::Main));

	TestTrue(TEXT("News1 opens once"), State.OpenPage(EBalhwajeomInternetPage::News1));
	TestFalse(TEXT("News1 does not duplicate"), State.OpenPage(EBalhwajeomInternetPage::News1));
	TestEqual(TEXT("Duplicate open still focuses News1"), State.GetActivePage(), EBalhwajeomInternetPage::News1);
	TestEqual(TEXT("There are two unique tabs"), State.GetOpenPages().Num(), 2);

	TestTrue(TEXT("Weather opens"), State.OpenPage(EBalhwajeomInternetPage::Weather));
	TestTrue(TEXT("Active Weather closes"), State.ClosePage(EBalhwajeomInternetPage::Weather));
	TestEqual(TEXT("Closing active tab selects its left neighbor"), State.GetActivePage(), EBalhwajeomInternetPage::News1);
	TestTrue(TEXT("News1 closes"), State.ClosePage(EBalhwajeomInternetPage::News1));
	TestEqual(TEXT("Main remains after closing other tabs"), State.GetActivePage(), EBalhwajeomInternetPage::Main);

	State.SetScrollOffset(EBalhwajeomInternetPage::News2, 315.5f);
	TestEqual(TEXT("Scroll offset is stored per page"), State.GetScrollOffset(EBalhwajeomInternetPage::News2), 315.5f);
	State.SetScrollOffset(EBalhwajeomInternetPage::News2, -20.0f);
	TestEqual(TEXT("Scroll offset cannot be negative"), State.GetScrollOffset(EBalhwajeomInternetPage::News2), 0.0f);

	State.SetMaximized(true);
	TestTrue(TEXT("Maximize state is stored"), State.IsMaximized());
	State.SetMaximized(false);
	TestFalse(TEXT("Restore state is stored"), State.IsMaximized());

	State.SetNormalWindowPosition(FVector2D(-50.0f, 900.0f));
	TestEqual(
		TEXT("Normal window remains fully inside the tablet"),
		State.GetNormalWindowPosition(),
		FVector2D(0.0f, 230.0f));

	TestEqual(
		TEXT("News page title is stable"),
		InternetPageTitle(EBalhwajeomInternetPage::News1).ToString(),
		FString(TEXT("뉴스 1")));
	TestFalse(TEXT("Main tab has no close action"), IsInternetPageCloseable(EBalhwajeomInternetPage::Main));
	TestTrue(TEXT("News tab has a close action"), IsInternetPageCloseable(EBalhwajeomInternetPage::News1));

	UBalhwajeomInternetPageWidget* PageWidget = NewObject<UBalhwajeomInternetPageWidget>();
	PageWidget->SetupPage(EBalhwajeomInternetPage::News2);
	TestEqual(TEXT("Page widget stores its page ID"), PageWidget->GetPageID(), EBalhwajeomInternetPage::News2);
	PageWidget->SetSavedScrollOffset(480.0f);
	TestEqual(TEXT("Page widget stores scroll without a designer tree"), PageWidget->GetSavedScrollOffset(), 480.0f);

	UBalhwajeomInternetTabWidget* TabWidget = NewObject<UBalhwajeomInternetTabWidget>();
	TabWidget->SetupTab(EBalhwajeomInternetPage::Main, true);
	TestEqual(TEXT("Tab widget stores its page ID"), TabWidget->GetPageID(), EBalhwajeomInternetPage::Main);
	TestFalse(TEXT("Main tab widget stays non-closeable"), TabWidget->IsCloseable());
	TabWidget->SetupTab(EBalhwajeomInternetPage::Ad, false);
	TestTrue(TEXT("Ad tab widget is closeable"), TabWidget->IsCloseable());

	UBalhwajeomInternetWidget* InternetWidget = NewObject<UBalhwajeomInternetWidget>();
	InternetWidget->InitializeInternet();
	TestEqual(TEXT("Internet controller initializes with Main"), InternetWidget->GetOpenTabCount(), 1);
	InternetWidget->OpenPage(EBalhwajeomInternetPage::News1);
	InternetWidget->OpenPage(EBalhwajeomInternetPage::News1);
	TestEqual(TEXT("Internet controller does not duplicate News1"), InternetWidget->GetOpenTabCount(), 2);
	TestEqual(TEXT("Internet controller focuses News1"), InternetWidget->GetActivePage(), EBalhwajeomInternetPage::News1);
	InternetWidget->SetNormalWindowPosition(FVector2D(90.0f, 70.0f));
	InternetWidget->ToggleMaximize();
	TestTrue(TEXT("Internet controller maximizes"), InternetWidget->IsMaximized());
	InternetWidget->PrepareForDesktopOpen();
	TestFalse(TEXT("Desktop reopen restores normal mode"), InternetWidget->IsMaximized());
	TestEqual(TEXT("Desktop reopen keeps normal position"), InternetWidget->GetNormalWindowPosition(), FVector2D(90.0f, 70.0f));
	TestEqual(TEXT("Desktop reopen keeps open tabs"), InternetWidget->GetOpenTabCount(), 2);
	TestTrue(TEXT("News1 closes through the controller"), InternetWidget->ClosePage(EBalhwajeomInternetPage::News1));
	TestEqual(TEXT("Controller returns to Main"), InternetWidget->GetActivePage(), EBalhwajeomInternetPage::Main);

	return true;
}

#endif
