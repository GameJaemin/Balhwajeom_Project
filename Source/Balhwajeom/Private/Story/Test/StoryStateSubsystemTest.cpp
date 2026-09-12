#include "StoryStateSubsystemTestTypes.h"

#include "Engine/GameInstance.h"
#include "Story/StoryStateSubsystem.h"


void UStoryStateSubsystemTestObserver::HandleStateTagAdded(
	FGameplayTag StateTag
)
{
	++AddedCount;
	LastAddedTag = StateTag;
}


void UStoryStateSubsystemTestObserver::HandleStateTagRemoved(
	FGameplayTag StateTag
)
{
	++RemovedCount;
	LastRemovedTag = StateTag;
}


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


namespace StoryStateSubsystemTests
{
	struct FFixture
	{
		FFixture()
			: GameInstance(NewObject<UGameInstance>())
			, Subsystem(NewObject<UStoryStateSubsystem>(GameInstance))
			, SearchTag(FGameplayTag::RequestGameplayTag(
				FName(TEXT("Test.Story.Search"))))
			, FoundTag(FGameplayTag::RequestGameplayTag(
				FName(TEXT("Test.Clue.Bloodstain.Found"))))
			, PhotographedTag(FGameplayTag::RequestGameplayTag(
				FName(TEXT("Test.Clue.Bloodstain.Photographed"))))
			, BloodstainParentTag(FGameplayTag::RequestGameplayTag(
				FName(TEXT("Test.Clue.Bloodstain"))))
		{
		}

		UGameInstance* GameInstance;
		UStoryStateSubsystem* Subsystem;
		FGameplayTag SearchTag;
		FGameplayTag FoundTag;
		FGameplayTag PhotographedTag;
		FGameplayTag BloodstainParentTag;
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateInitialStateTest,
	"Balhwajeom.StoryState.InitialState",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateInitialStateTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	TestTrue(
		TEXT("A new subsystem should have no active state tags"),
		Fixture.Subsystem->GetCurrentStateTags().IsEmpty()
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStatePlayerModeTagsRegisteredTest,
	"Balhwajeom.StoryState.PlayerMode.TagsRegistered",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStatePlayerModeTagsRegisteredTest::RunTest(
	const FString& Parameters
)
{
	const TArray<FName> RequiredTagNames = {
		TEXT("Runtime.Player.Mode"),
		TEXT("Runtime.Player.Mode.Exploration"),
		TEXT("Runtime.Player.Mode.PhotoCamera"),
		TEXT("Runtime.Player.Mode.Tablet")
	};

	for (const FName TagName : RequiredTagNames)
	{
		TestTrue(
			FString::Printf(TEXT("%s should be registered"), *TagName.ToString()),
			FGameplayTag::RequestGameplayTag(TagName, false).IsValid()
		);
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateAddAndDuplicateTest,
	"Balhwajeom.StoryState.AddAndDuplicate",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateAddAndDuplicateTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	TestTrue(
		TEXT("The first Add should change the current state"),
		Fixture.Subsystem->AddStateTag(Fixture.FoundTag)
	);

	TestTrue(
		TEXT("An added tag should be found exactly"),
		Fixture.Subsystem->HasStateTagExact(Fixture.FoundTag)
	);

	TestEqual(
		TEXT("The current state should contain one exact tag"),
		Fixture.Subsystem->GetCurrentStateTags().Num(),
		1
	);

	TestFalse(
		TEXT("Adding the same exact tag again should not change state"),
		Fixture.Subsystem->AddStateTag(Fixture.FoundTag)
	);

	TestEqual(
		TEXT("Duplicate Add should not create another stored tag"),
		Fixture.Subsystem->GetCurrentStateTags().Num(),
		1
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateRemoveAndMissingTest,
	"Balhwajeom.StoryState.RemoveAndMissing",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateRemoveAndMissingTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	TestTrue(
		TEXT("Removing an active exact tag should change state"),
		Fixture.Subsystem->RemoveStateTag(Fixture.FoundTag)
	);

	TestFalse(
		TEXT("A removed tag should no longer be active"),
		Fixture.Subsystem->HasStateTagExact(Fixture.FoundTag)
	);

	TestFalse(
		TEXT("Removing an absent tag should be safe and report no change"),
		Fixture.Subsystem->RemoveStateTag(Fixture.FoundTag)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateHierarchyTest,
	"Balhwajeom.StoryState.Hierarchy",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateHierarchyTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	TestTrue(
		TEXT("Hierarchical lookup should match the parent of an active tag"),
		Fixture.Subsystem->HasStateTag(Fixture.BloodstainParentTag)
	);

	TestFalse(
		TEXT("Exact lookup should not match an unstored parent tag"),
		Fixture.Subsystem->HasStateTagExact(
			Fixture.BloodstainParentTag
		)
	);

	TestTrue(
		TEXT("Exact lookup should match the exact active tag"),
		Fixture.Subsystem->HasStateTagExact(Fixture.FoundTag)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateExclusiveGroupReplacementTest,
	"Balhwajeom.StoryState.ExclusiveGroup.Replacement",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateExclusiveGroupReplacementTest::RunTest(
	const FString& Parameters
)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	TestTrue(
		TEXT("Selecting another tag in the same group should change state"),
		Fixture.Subsystem->SetExclusiveStateTag(
			Fixture.BloodstainParentTag,
			Fixture.PhotographedTag
		)
	);

	TestFalse(
		TEXT("The previously active group tag should be removed"),
		Fixture.Subsystem->HasStateTagExact(Fixture.FoundTag)
	);

	TestTrue(
		TEXT("The selected group tag should be active"),
		Fixture.Subsystem->HasStateTagExact(Fixture.PhotographedTag)
	);

	TestEqual(
		TEXT("An exclusive group should retain exactly one active tag"),
		Fixture.Subsystem->GetCurrentStateTags().Num(),
		1
	);

	TestFalse(
		TEXT("Selecting the already exclusive tag should report no change"),
		Fixture.Subsystem->SetExclusiveStateTag(
			Fixture.BloodstainParentTag,
			Fixture.PhotographedTag
		)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateExclusiveGroupValidationTest,
	"Balhwajeom.StoryState.ExclusiveGroup.Validation",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateExclusiveGroupValidationTest::RunTest(
	const FString& Parameters
)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	TestFalse(
		TEXT("A tag outside the requested group should be rejected"),
		Fixture.Subsystem->SetExclusiveStateTag(
			Fixture.BloodstainParentTag,
			Fixture.SearchTag
		)
	);

	TestTrue(
		TEXT("Rejecting an outside tag should preserve the previous state"),
		Fixture.Subsystem->HasStateTagExact(Fixture.FoundTag)
	);

	TestFalse(
		TEXT("Rejecting an outside tag should not add it"),
		Fixture.Subsystem->HasStateTagExact(Fixture.SearchTag)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateAnyAndAllTest,
	"Balhwajeom.StoryState.AnyAndAll",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateAnyAndAllTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.SearchTag);
	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	FGameplayTagContainer AnyMatchingTags;
	AnyMatchingTags.AddTag(Fixture.PhotographedTag);
	AnyMatchingTags.AddTag(Fixture.BloodstainParentTag);

	TestTrue(
		TEXT("Any should succeed when one requested parent matches"),
		Fixture.Subsystem->HasAnyStateTags(AnyMatchingTags)
	);

	FGameplayTagContainer NoMatchingTags;
	NoMatchingTags.AddTag(Fixture.PhotographedTag);

	TestFalse(
		TEXT("Any should fail when no requested tag matches"),
		Fixture.Subsystem->HasAnyStateTags(NoMatchingTags)
	);

	FGameplayTagContainer AllMatchingTags;
	AllMatchingTags.AddTag(Fixture.SearchTag);
	AllMatchingTags.AddTag(Fixture.BloodstainParentTag);

	TestTrue(
		TEXT("All should succeed when every requested tag matches"),
		Fixture.Subsystem->HasAllStateTags(AllMatchingTags)
	);

	FGameplayTagContainer IncompleteTags = AllMatchingTags;
	IncompleteTags.AddTag(Fixture.PhotographedTag);

	TestFalse(
		TEXT("All should fail when one requested tag is inactive"),
		Fixture.Subsystem->HasAllStateTags(IncompleteTags)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateQueryTest,
	"Balhwajeom.StoryState.Query",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateQueryTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.SearchTag);
	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	FGameplayTagQueryExpression PositiveExpression;
	PositiveExpression
		.AllTagsMatch()
		.AddTag(Fixture.SearchTag)
		.AddTag(Fixture.FoundTag);

	FGameplayTagQuery PositiveQuery;
	PositiveQuery.Build(PositiveExpression);

	TestTrue(
		TEXT("A query requiring both active tags should match"),
		Fixture.Subsystem->MatchesStateQuery(PositiveQuery)
	);

	FGameplayTagQueryExpression RequiredSearchExpression;
	RequiredSearchExpression
		.AllTagsMatch()
		.AddTag(Fixture.SearchTag);

	FGameplayTagQueryExpression NotPhotographedExpression;
	NotPhotographedExpression
		.NoTagsMatch()
		.AddTag(Fixture.PhotographedTag);

	FGameplayTagQueryExpression RootExpression;
	RootExpression
		.AllExprMatch()
		.AddExpr(RequiredSearchExpression)
		.AddExpr(NotPhotographedExpression);

	FGameplayTagQuery NotPhotographedQuery;
	NotPhotographedQuery.Build(RootExpression);

	TestTrue(
		TEXT("Search plus NOT Photographed should match before photography"),
		Fixture.Subsystem->MatchesStateQuery(NotPhotographedQuery)
	);

	Fixture.Subsystem->AddStateTag(Fixture.PhotographedTag);

	TestFalse(
		TEXT("Search plus NOT Photographed should fail after photography"),
		Fixture.Subsystem->MatchesStateQuery(NotPhotographedQuery)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateClearTest,
	"Balhwajeom.StoryState.Clear",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateClearTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;

	Fixture.Subsystem->AddStateTag(Fixture.SearchTag);
	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);

	TestTrue(
		TEXT("Clearing active tags should report a state change"),
		Fixture.Subsystem->ClearStateTags()
	);

	TestTrue(
		TEXT("Clear should leave the current state empty"),
		Fixture.Subsystem->GetCurrentStateTags().IsEmpty()
	);

	TestFalse(
		TEXT("Clearing an empty state should report no change"),
		Fixture.Subsystem->ClearStateTags()
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateDelegatesTest,
	"Balhwajeom.StoryState.Delegates",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateDelegatesTest::RunTest(const FString& Parameters)
{
	const StoryStateSubsystemTests::FFixture Fixture;
	UStoryStateSubsystemTestObserver* Observer =
		NewObject<UStoryStateSubsystemTestObserver>(Fixture.GameInstance);

	Fixture.Subsystem->OnStateTagAdded.AddDynamic(
		Observer,
		&UStoryStateSubsystemTestObserver::HandleStateTagAdded
	);

	Fixture.Subsystem->OnStateTagRemoved.AddDynamic(
		Observer,
		&UStoryStateSubsystemTestObserver::HandleStateTagRemoved
	);

	TestFalse(
		TEXT("Adding an invalid tag should report no change"),
		Fixture.Subsystem->AddStateTag(FGameplayTag())
	);

	TestFalse(
		TEXT("Removing an invalid tag should report no change"),
		Fixture.Subsystem->RemoveStateTag(FGameplayTag())
	);

	Fixture.Subsystem->AddStateTag(Fixture.SearchTag);
	Fixture.Subsystem->AddStateTag(Fixture.SearchTag);

	TestEqual(
		TEXT("Duplicate Add should emit only one added event"),
		Observer->AddedCount,
		1
	);

	TestTrue(
		TEXT("The added event should identify the changed tag"),
		Observer->LastAddedTag == Fixture.SearchTag
	);

	Fixture.Subsystem->RemoveStateTag(Fixture.PhotographedTag);

	TestEqual(
		TEXT("Removing an absent tag should not emit a removed event"),
		Observer->RemovedCount,
		0
	);

	Fixture.Subsystem->RemoveStateTag(Fixture.SearchTag);

	TestEqual(
		TEXT("Removing an active tag should emit one removed event"),
		Observer->RemovedCount,
		1
	);

	TestTrue(
		TEXT("The removed event should identify the changed tag"),
		Observer->LastRemovedTag == Fixture.SearchTag
	);

	Fixture.Subsystem->AddStateTag(Fixture.FoundTag);
	Fixture.Subsystem->AddStateTag(Fixture.PhotographedTag);
	Fixture.Subsystem->ClearStateTags();

	TestEqual(
		TEXT("Clear should emit once for each of its two removed tags"),
		Observer->RemovedCount,
		3
	);

	Fixture.Subsystem->ClearStateTags();

	TestEqual(
		TEXT("Empty Clear should not emit another removed event"),
		Observer->RemovedCount,
		3
	);

	TestEqual(
		TEXT("Invalid operations should not have added events"),
		Observer->AddedCount,
		3
	);

	return true;
}

#endif
