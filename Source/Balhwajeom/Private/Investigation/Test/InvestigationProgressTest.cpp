#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Investigation/BalhwajeomInvestigationProgress.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationReadyForStatementTest,
	"Balhwajeom.Investigation.Progress.ReadyForStatement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationReadyForStatementTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomInvestigationProgress;

	constexpr int32 RequiredWords = 14;
	constexpr int32 TotalPhotos = 3;

	// Both halves have to land. Either one alone is still mid-investigation.
	TestTrue(TEXT("a complete set is ready"),
		IsReadyForStatement(RequiredWords, RequiredWords, TotalPhotos, TotalPhotos));
	TestFalse(TEXT("one keyword short is not ready"),
		IsReadyForStatement(RequiredWords - 1, RequiredWords, TotalPhotos, TotalPhotos));
	TestFalse(TEXT("one photo sentence short is not ready"),
		IsReadyForStatement(RequiredWords, RequiredWords, TotalPhotos - 1, TotalPhotos));
	TestFalse(TEXT("neither half done is not ready"),
		IsReadyForStatement(0, RequiredWords, 0, TotalPhotos));

	// DT_Words carries more acquirable rows than the displayed total, so overshooting the
	// keyword count is a normal state and must still count as done.
	TestTrue(TEXT("more keywords than required is still ready"),
		IsReadyForStatement(RequiredWords + 5, RequiredWords, TotalPhotos, TotalPhotos));

	// The guard that stops the line firing on the opening frame: before the data tables are
	// loaded both totals read zero, which is "nothing known yet", not "nothing left to do".
	TestFalse(TEXT("no keywords required means the data is not loaded"),
		IsReadyForStatement(0, 0, TotalPhotos, TotalPhotos));
	TestFalse(TEXT("no photo sentences means the data is not loaded"),
		IsReadyForStatement(RequiredWords, RequiredWords, 0, 0));
	TestFalse(TEXT("an entirely empty investigation is never ready"),
		IsReadyForStatement(0, 0, 0, 0));

	// Negative totals can only come from a bad read; they must not read as complete either.
	TestFalse(TEXT("a negative keyword requirement is not ready"),
		IsReadyForStatement(RequiredWords, -1, TotalPhotos, TotalPhotos));
	TestFalse(TEXT("a negative photo total is not ready"),
		IsReadyForStatement(RequiredWords, RequiredWords, TotalPhotos, -1));

	return !HasAnyErrors();
}

#endif
