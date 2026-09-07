#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Story/StoryStateSubsystem.h"


namespace InspectionDataSelectionTests
{
	FGameplayTag RequestTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName));
	}


	FGameplayTagQuery MakeRequiredTagQuery(const FGameplayTag& RequiredTag)
	{
		FGameplayTagQueryExpression Expression;
		Expression.AllTagsMatch().AddTag(RequiredTag);

		FGameplayTagQuery Query;
		Query.Build(Expression);
		return Query;
	}


	FGameplayTagQuery MakeSearchWithoutPhotographedQuery(
		const FGameplayTag& SearchTag,
		const FGameplayTag& PhotographedTag
	)
	{
		FGameplayTagQueryExpression SearchExpression;
		SearchExpression.AllTagsMatch().AddTag(SearchTag);

		FGameplayTagQueryExpression NotPhotographedExpression;
		NotPhotographedExpression.NoTagsMatch().AddTag(PhotographedTag);

		FGameplayTagQueryExpression RootExpression;
		RootExpression
			.AllExprMatch()
			.AddExpr(SearchExpression)
			.AddExpr(NotPhotographedExpression);

		FGameplayTagQuery Query;
		Query.Build(RootExpression);
		return Query;
	}


	FInspectionData MakeInspectionData(const TCHAR* Prefix)
	{
		FInspectionData Data;
		Data.FarLabel = FText::FromString(FString::Printf(TEXT("%s Far"), Prefix));
		Data.MidLabel = FText::FromString(FString::Printf(TEXT("%s Mid"), Prefix));
		Data.NearLabel = FText::FromString(FString::Printf(TEXT("%s Near"), Prefix));
		Data.InspectionText = FText::FromString(
			FString::Printf(TEXT("%s Inspection"), Prefix)
		);
		return Data;
	}


	void SetDefaultInspectionData(UInspectionComponent& Inspection)
	{
		Inspection.FarLabel = FText::FromString(TEXT("Default Far"));
		Inspection.MidLabel = FText::FromString(TEXT("Default Mid"));
		Inspection.NearLabel = FText::FromString(TEXT("Default Near"));
		Inspection.InspectionText = FText::FromString(TEXT("Default Inspection"));
	}


	void TestDataEquals(
		FAutomationTestBase& Test,
		const TCHAR* Context,
		const FInspectionData& Actual,
		const TCHAR* ExpectedPrefix
	)
	{
		Test.TestEqual(
			FString::Printf(TEXT("%s FarLabel"), Context),
			Actual.FarLabel.ToString(),
			FString::Printf(TEXT("%s Far"), ExpectedPrefix)
		);
		Test.TestEqual(
			FString::Printf(TEXT("%s MidLabel"), Context),
			Actual.MidLabel.ToString(),
			FString::Printf(TEXT("%s Mid"), ExpectedPrefix)
		);
		Test.TestEqual(
			FString::Printf(TEXT("%s NearLabel"), Context),
			Actual.NearLabel.ToString(),
			FString::Printf(TEXT("%s Near"), ExpectedPrefix)
		);
		Test.TestEqual(
			FString::Printf(TEXT("%s InspectionText"), Context),
			Actual.InspectionText.ToString(),
			FString::Printf(TEXT("%s Inspection"), ExpectedPrefix)
		);
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataDefaultTest,
	"Balhwajeom.Interaction.InspectionData.Default",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataDefaultTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(FGameplayTagContainer());

	InspectionDataSelectionTests::TestDataEquals(
		*this,
		TEXT("No conditions should preserve all default fields"),
		Resolved,
		TEXT("Default")
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataMatchingConditionTest,
	"Balhwajeom.Interaction.InspectionData.MatchingCondition",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataMatchingConditionTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));

	FConditionalInspectionData Entry;
	Entry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(SearchTag);
	Entry.Data = InspectionDataSelectionTests::MakeInspectionData(TEXT("Search"));
	Inspection->ConditionalData.Add(Entry);

	FGameplayTagContainer StateTags;
	StateTags.AddTag(SearchTag);

	const FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("A matching condition should select its InspectionText"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Search Inspection"))
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataNonMatchingConditionTest,
	"Balhwajeom.Interaction.InspectionData.NonMatchingCondition",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataNonMatchingConditionTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));
	const FGameplayTag FoundTag = InspectionDataSelectionTests::RequestTag(
		TEXT("Test.Clue.Bloodstain.Found")
	);

	FConditionalInspectionData Entry;
	Entry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(FoundTag);
	Entry.Data = InspectionDataSelectionTests::MakeInspectionData(TEXT("Found"));
	Inspection->ConditionalData.Add(Entry);

	FGameplayTagContainer StateTags;
	StateTags.AddTag(SearchTag);

	const FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("A non-matching condition should fall back to default"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Default Inspection"))
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataFirstMatchTest,
	"Balhwajeom.Interaction.InspectionData.FirstMatchWins",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataFirstMatchTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));
	const FGameplayTag FoundTag = InspectionDataSelectionTests::RequestTag(
		TEXT("Test.Clue.Bloodstain.Found")
	);

	FConditionalInspectionData FirstEntry;
	FirstEntry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(SearchTag);
	FirstEntry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("First"));
	Inspection->ConditionalData.Add(FirstEntry);

	FConditionalInspectionData SecondEntry;
	SecondEntry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(FoundTag);
	SecondEntry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("Second"));
	Inspection->ConditionalData.Add(SecondEntry);

	FGameplayTagContainer StateTags;
	StateTags.AddTag(SearchTag);
	StateTags.AddTag(FoundTag);

	const FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("The first matching array entry should win"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("First Inspection"))
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataEmptyQueryTest,
	"Balhwajeom.Interaction.InspectionData.EmptyQuery",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataEmptyQueryTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));

	FConditionalInspectionData EmptyEntry;
	EmptyEntry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("Empty"));
	Inspection->ConditionalData.Add(EmptyEntry);

	FConditionalInspectionData MatchingEntry;
	MatchingEntry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(SearchTag);
	MatchingEntry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("Matching"));
	Inspection->ConditionalData.Add(MatchingEntry);

	FGameplayTagContainer StateTags;
	StateTags.AddTag(SearchTag);

	FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("An empty query should be skipped in favor of a later match"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Matching Inspection"))
	);

	Inspection->ConditionalData.RemoveAt(1);
	Resolved = Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("Only empty queries should fall back to default"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Default Inspection"))
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataNotQueryTest,
	"Balhwajeom.Interaction.InspectionData.NotQuery",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataNotQueryTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));
	const FGameplayTag PhotographedTag =
		InspectionDataSelectionTests::RequestTag(
			TEXT("Test.Clue.Bloodstain.Photographed")
		);

	FConditionalInspectionData Entry;
	Entry.Condition =
		InspectionDataSelectionTests::MakeSearchWithoutPhotographedQuery(
			SearchTag,
			PhotographedTag
		);
	Entry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("Unphotographed"));
	Inspection->ConditionalData.Add(Entry);

	FGameplayTagContainer StateTags;
	StateTags.AddTag(SearchTag);

	FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("Search without Photographed should match the NOT query"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Unphotographed Inspection"))
	);

	StateTags.AddTag(PhotographedTag);
	Resolved = Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("Adding Photographed should make the NOT query fall back"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Default Inspection"))
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataStateChangeTest,
	"Balhwajeom.Interaction.InspectionData.StateChange",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataStateChangeTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));
	const FGameplayTag FoundTag = InspectionDataSelectionTests::RequestTag(
		TEXT("Test.Clue.Bloodstain.Found")
	);

	FConditionalInspectionData SearchEntry;
	SearchEntry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(SearchTag);
	SearchEntry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("Search"));
	Inspection->ConditionalData.Add(SearchEntry);

	FConditionalInspectionData FoundEntry;
	FoundEntry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(FoundTag);
	FoundEntry.Data =
		InspectionDataSelectionTests::MakeInspectionData(TEXT("Found"));
	Inspection->ConditionalData.Add(FoundEntry);

	FGameplayTagContainer StateTags;
	StateTags.AddTag(SearchTag);

	FInspectionData Resolved =
		Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("Search state should select Search data"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Search Inspection"))
	);

	StateTags.Reset();
	StateTags.AddTag(FoundTag);
	Resolved = Inspection->ResolveInspectionDataForState(StateTags);

	TestEqual(
		TEXT("Changing the supplied state should select Found data"),
		Resolved.InspectionText.ToString(),
		FString(TEXT("Found Inspection"))
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionDataNoWorldTest,
	"Balhwajeom.Interaction.InspectionData.NoWorldFallback",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FInspectionDataNoWorldTest::RunTest(const FString& Parameters)
{
	UInspectionComponent* Inspection = NewObject<UInspectionComponent>();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FInspectionData Resolved = Inspection->GetCurrentInspectionData();

	InspectionDataSelectionTests::TestDataEquals(
		*this,
		TEXT("A component without a World should return default data"),
		Resolved,
		TEXT("Default")
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionConditionalInspectionTest,
	"Balhwajeom.Interaction.InspectionData.PlayerInteractionUsesCurrentState",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionConditionalInspectionTest::RunTest(
	const FString& Parameters
)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();

	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>(Owner);
	Owner->AddInstanceComponent(Inspection);
	Inspection->RegisterComponent();
	InspectionDataSelectionTests::SetDefaultInspectionData(*Inspection);

	const FGameplayTag SearchTag =
		InspectionDataSelectionTests::RequestTag(TEXT("Test.Story.Search"));

	FConditionalInspectionData Entry;
	Entry.Condition =
		InspectionDataSelectionTests::MakeRequiredTagQuery(SearchTag);
	Entry.Data = InspectionDataSelectionTests::MakeInspectionData(TEXT("Search"));
	Inspection->ConditionalData.Add(Entry);

	UPlayerInteractionComponent* PlayerInteraction =
		NewObject<UPlayerInteractionComponent>(Owner);
	Owner->AddInstanceComponent(PlayerInteraction);
	PlayerInteraction->RegisterComponent();
	PlayerInteraction->UpdateDistanceStateForInspectable(
		Inspection,
		FVector::ZeroVector,
		FVector(100.0f, 0.0f, 0.0f)
	);
	PlayerInteraction->SetFocusedInspection(Inspection);

	UStoryStateSubsystem* StoryState =
		GameInstance->GetSubsystem<UStoryStateSubsystem>();
	TestNotNull(TEXT("GameInstance should provide StoryStateSubsystem"), StoryState);
	if (StoryState)
	{
		StoryState->AddStateTag(SearchTag);
	}

	FText ResultText;
	const bool bInspected = PlayerInteraction->TryInspect(ResultText);

	TestTrue(TEXT("A focused Close target should remain inspectable"), bInspected);
	TestEqual(
		TEXT("TryInspect should forward the data selected from current Story State"),
		ResultText.ToString(),
		FString(TEXT("Search Inspection"))
	);

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	return true;
}

#endif
