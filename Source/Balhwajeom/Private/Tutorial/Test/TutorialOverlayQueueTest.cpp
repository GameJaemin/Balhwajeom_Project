#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Tutorial/BalhwajeomTutorialOverlayQueue.h"
#include "Tutorial/TutorialOverlayDefinitions.h"

namespace
{
	FGameplayTag TestTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}

	FTutorialOverlayDefinition MakeRow(
		const TArray<FGameplayTag>& RequiredTags,
		const FGameplayTag& CompletionTag)
	{
		FTutorialOverlayDefinition Definition;
		for (const FGameplayTag& RequiredTag : RequiredTags)
		{
			Definition.RequiredTags.AddTag(RequiredTag);
		}
		Definition.CompletionTag = CompletionTag;
		return Definition;
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayQueueTest,
	"Balhwajeom.Tutorial.Overlay.Queue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlayQueueTest::RunTest(const FString& Parameters)
{
	namespace Queue = BalhwajeomTutorialOverlayQueue;

	const FGameplayTag TriggerA = TestTag(TEXT("Test.Tutorial.Trigger.One"));
	const FGameplayTag TriggerB = TestTag(TEXT("Test.Tutorial.Trigger.Two"));
	const FGameplayTag SeenA = TestTag(TEXT("Test.Tutorial.Stage.One"));
	const FGameplayTag SeenB = TestTag(TEXT("Test.Tutorial.Stage.Two"));
	if (!TestTrue(TEXT("The automation tutorial tags should be registered"),
		TriggerA.IsValid() && TriggerB.IsValid() && SeenA.IsValid() && SeenB.IsValid()))
	{
		return false;
	}

	const FTutorialOverlayDefinition RowA = MakeRow({ TriggerA }, SeenA);
	const FTutorialOverlayDefinition RowB = MakeRow({ TriggerB }, SeenB);

	FGameplayTagContainer StateTags;
	TestFalse(TEXT("Nothing is due before its trigger arrives"), Queue::IsDue(RowA, StateTags));

	StateTags.AddTag(TriggerA);
	TestTrue(TEXT("A row is due once its trigger arrives"), Queue::IsDue(RowA, StateTags));

	StateTags.AddTag(SeenA);
	TestFalse(TEXT("A row that has been seen is never due again"), Queue::IsDue(RowA, StateTags));

	// Rows are authored with their conditions empty while the copy is being written.
	// Reading that as "always satisfied" would dump every unfinished screen on the player.
	const FTutorialOverlayDefinition Unconditioned = MakeRow({}, SeenB);
	TestFalse(TEXT("A row with no conditions is never due"),
		Queue::IsDue(Unconditioned, StateTags));

	// Without a completion tag the row could never be marked seen, so it would reopen on
	// every matching moment for the rest of the run.
	const FTutorialOverlayDefinition Uncompletable = MakeRow({ TriggerA }, FGameplayTag());
	TestFalse(TEXT("A row with no completion tag is never due"),
		Queue::IsDue(Uncompletable, StateTags));

	// Partially satisfied: one of two required tags is present.
	const FTutorialOverlayDefinition RowBoth = MakeRow({ TriggerA, TriggerB }, SeenB);
	TestFalse(TEXT("A row needs every one of its required tags"),
		Queue::IsDue(RowBoth, StateTags));
	StateTags.AddTag(TriggerB);
	TestTrue(TEXT("A row is due once its last required tag arrives"),
		Queue::IsDue(RowBoth, StateTags));

	// One tag can complete two rows at once, and both must survive to the queue.
	const TArray<Queue::FCandidate> AllRows = {
		{ TEXT("RowA"), &RowA },
		{ TEXT("RowB"), &RowB }
	};
	FGameplayTagContainer BothReady;
	BothReady.AddTag(TriggerA);
	BothReady.AddTag(TriggerB);

	TArray<Queue::FCandidate> Due = Queue::SelectDueRows(AllRows, BothReady, {});
	TestEqual(TEXT("Two rows can come due at the same moment"), Due.Num(), 2);
	if (Due.Num() == 2)
	{
		TestEqual(TEXT("Due rows keep the order they were given"),
			Due[0].RowName, FName(TEXT("RowA")));
	}

	Due = Queue::SelectDueRows(AllRows, BothReady, { TEXT("RowA") });
	TestEqual(TEXT("A row already queued is not queued twice"), Due.Num(), 1);
	if (!Due.IsEmpty())
	{
		TestEqual(TEXT("The row still waiting is the one returned"),
			Due[0].RowName, FName(TEXT("RowB")));
	}
	return true;
}

#endif
