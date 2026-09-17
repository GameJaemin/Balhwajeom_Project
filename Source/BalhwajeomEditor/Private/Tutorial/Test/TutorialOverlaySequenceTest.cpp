#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Tutorial/BalhwajeomTutorialOverlayQueue.h"
#include "Tutorial/TutorialOverlayDefinitions.h"

namespace
{
	const TCHAR* OverlayTablePath =
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_TutorialOverlay.DT_TutorialOverlay");

	/** The rows in the order the presenter walks them, straight from the shipped table. */
	bool LoadOrderedRows(
		FAutomationTestBase& Test,
		TArray<BalhwajeomTutorialOverlayQueue::FCandidate>& OutRows)
	{
		const UDataTable* Table = LoadObject<UDataTable>(nullptr, OverlayTablePath);
		if (!Test.TestNotNull(TEXT("DT_TutorialOverlay should load"), Table))
		{
			return false;
		}

		for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
		{
			OutRows.Add(
				{ Row.Key, reinterpret_cast<const FTutorialOverlayDefinition*>(Row.Value) });
		}
		OutRows.Sort([](const BalhwajeomTutorialOverlayQueue::FCandidate& Left,
			const BalhwajeomTutorialOverlayQueue::FCandidate& Right)
		{
			return Left.RowName.LexicalLess(Right.RowName);
		});
		return true;
	}

	/**
	 * Native tags are declared, not exported, so this module cannot link against the
	 * symbols. Resolving by name instead also proves they reached the tag registry.
	 */
	FGameplayTag Tag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}

	/**
	 * Plays one moment: adds the tag, takes whatever became due, and marks those rows seen
	 * as if the player had read and dismissed each screen in turn.
	 */
	TArray<FName> PlayMoment(
		const TArray<BalhwajeomTutorialOverlayQueue::FCandidate>& Rows,
		FGameplayTagContainer& StateTags,
		const FGameplayTag& MomentTag)
	{
		if (MomentTag.IsValid())
		{
			StateTags.AddTag(MomentTag);
		}

		TArray<FName> Shown;
		for (int32 Guard = 0; Guard < 16; ++Guard)
		{
			const TArray<BalhwajeomTutorialOverlayQueue::FCandidate> Due =
				BalhwajeomTutorialOverlayQueue::SelectDueRows(Rows, StateTags, Shown);
			if (Due.IsEmpty())
			{
				break;
			}

			for (const BalhwajeomTutorialOverlayQueue::FCandidate& Row : Due)
			{
				Shown.Add(Row.RowName);
				StateTags.AddTag(Row.Definition->CompletionTag);
			}
		}
		return Shown;
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlaySequenceTest,
	"Balhwajeom.Tutorial.Overlay.Sequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlaySequenceTest::RunTest(const FString& Parameters)
{
	TArray<BalhwajeomTutorialOverlayQueue::FCandidate> Rows;
	if (!LoadOrderedRows(*this, Rows))
	{
		return false;
	}

	FGameplayTagContainer StateTags;

	// The intro opens the tablet on the statement before the tutorial has taught anything.
	// A one-shot "tablet was opened" tag would be spent here and OVL_01_007 would fire the
	// moment OVL_01_006 closed, nowhere near the tablet.
	TArray<FName> Shown = PlayMoment(
		Rows, StateTags, Tag(TEXT("Tutorial.Trigger.TabletOpened")));
	TestTrue(TEXT("Nothing is due before gameplay starts"), Shown.IsEmpty());
	StateTags.RemoveTag(Tag(TEXT("Tutorial.Trigger.TabletOpened")));

	// Gameplay starts while the intro's statement tablet is still open, so only the
	// statement screen is due. The movement screen has to wait for the world to be visible.
	StateTags.AddTag(Tag(TEXT("Tutorial.Trigger.TabletOpened")));
	Shown = PlayMoment(
		Rows, StateTags, Tag(TEXT("Tutorial.Trigger.GameplayStarted")));
	TestEqual(TEXT("Starting gameplay shows only the statement screen"), Shown.Num(), 1);
	if (!Shown.IsEmpty())
	{
		TestEqual(TEXT("The statement screen is first"), Shown[0], FName(TEXT("OVL_01_001")));
	}

	// Closing that tablet is what releases the movement screen.
	StateTags.RemoveTag(Tag(TEXT("Tutorial.Trigger.TabletOpened")));
	Shown = PlayMoment(Rows, StateTags, Tag(TEXT("Tutorial.Trigger.TabletClosed")));
	TestEqual(TEXT("Closing the statement tablet shows the movement screen"), Shown.Num(), 1);
	if (!Shown.IsEmpty())
	{
		TestEqual(TEXT("The movement screen is second"), Shown[0], FName(TEXT("OVL_01_002")));
	}

	Shown = PlayMoment(
		Rows, StateTags, Tag(TEXT("Tutorial.Trigger.InteractPromptShown")));
	TestEqual(TEXT("Looking at an interactable shows only the interaction screen"),
		Shown.Num(), 1);
	if (!Shown.IsEmpty())
	{
		TestEqual(TEXT("The interaction screen is third"), Shown[0], FName(TEXT("OVL_01_003")));
	}

	Shown = PlayMoment(
		Rows, StateTags, Tag(TEXT("Tutorial.Trigger.InteractCompleted")));
	TestEqual(TEXT("Finishing an interaction shows only the camera screen"), Shown.Num(), 1);
	if (!Shown.IsEmpty())
	{
		TestEqual(TEXT("The camera screen is fourth"), Shown[0], FName(TEXT("OVL_01_004")));
	}

	// The worst ordering case: all three frames photographed from inside camera mode, so
	// the photographed tags land while the viewfinder still owns the screen.
	StateTags.AddTag(Tag(TEXT("Runtime.Player.Mode.PhotoCamera")));
	StateTags.AddTag(Tag(TEXT("Evidence.Photographed.OBJ_01_001")));
	StateTags.AddTag(Tag(TEXT("Evidence.Photographed.OBJ_01_002")));
	Shown = PlayMoment(Rows, StateTags, Tag(TEXT("Evidence.Photographed.OBJ_01_003")));
	TestTrue(TEXT("Photographing inside camera mode shows nothing yet"), Shown.IsEmpty());

	// Lowering the camera puts the player back in third person and ends the photo trip.
	StateTags.RemoveTag(Tag(TEXT("Runtime.Player.Mode.PhotoCamera")));
	StateTags.AddTag(Tag(TEXT("Tutorial.Trigger.PhotoCaptureCompleted")));
	Shown = PlayMoment(Rows, StateTags, Tag(TEXT("Runtime.Player.Mode.Exploration")));
	TestEqual(TEXT("Leaving camera mode shows the memory and tablet screens in order"),
		Shown.Num(), 2);
	if (Shown.Num() == 2)
	{
		TestEqual(TEXT("The memory screen comes before the tablet screen"),
			Shown[0], FName(TEXT("OVL_01_005")));
		TestEqual(TEXT("The tablet screen is sixth"), Shown[1], FName(TEXT("OVL_01_006")));
	}

	StateTags.RemoveTag(Tag(TEXT("Tutorial.Trigger.TabletClosed")));
	Shown = PlayMoment(
		Rows, StateTags, Tag(TEXT("Tutorial.Trigger.TabletOpened")));
	TestEqual(TEXT("Opening the tablet now shows the sister folder screen"), Shown.Num(), 1);
	if (!Shown.IsEmpty())
	{
		TestEqual(TEXT("The sister folder screen is seventh"),
			Shown[0], FName(TEXT("OVL_01_007")));
	}

	Shown = PlayMoment(
		Rows, StateTags, Tag(TEXT("Tutorial.Trigger.SisterFolderOpened")));
	TestEqual(TEXT("Opening the sister folder shows the last screen"), Shown.Num(), 1);
	if (!Shown.IsEmpty())
	{
		TestEqual(TEXT("The photo puzzle screen is last"), Shown[0], FName(TEXT("OVL_01_008")));
	}

	// Every screen has now been seen, so replaying the whole run must show nothing again.
	const TArray<FGameplayTag> EveryTrigger = {
		Tag(TEXT("Tutorial.Trigger.GameplayStarted")),
		Tag(TEXT("Tutorial.Trigger.InteractPromptShown")),
		Tag(TEXT("Tutorial.Trigger.InteractCompleted")),
		Tag(TEXT("Tutorial.Trigger.PhotoCaptureCompleted")),
		Tag(TEXT("Tutorial.Trigger.TabletOpened")),
		Tag(TEXT("Tutorial.Trigger.TabletClosed")),
		Tag(TEXT("Tutorial.Trigger.SisterFolderOpened"))
	};
	for (const FGameplayTag& Replayed : EveryTrigger)
	{
		TestTrue(TEXT("A seen overlay never reopens"),
			PlayMoment(Rows, StateTags, Replayed).IsEmpty());
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayRequiredTagsTest,
	"Balhwajeom.Tutorial.Overlay.RequiredTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlayRequiredTagsTest::RunTest(const FString& Parameters)
{
	TArray<BalhwajeomTutorialOverlayQueue::FCandidate> Rows;
	if (!LoadOrderedRows(*this, Rows))
	{
		return false;
	}

	FGameplayTag PreviousCompletionTag;
	for (const BalhwajeomTutorialOverlayQueue::FCandidate& Row : Rows)
	{
		const FString RowLabel = Row.RowName.ToString();
		if (!TestNotNull(FString::Printf(TEXT("Row %s should have data"), *RowLabel),
			Row.Definition))
		{
			continue;
		}

		// An empty container would make the row unreachable, which is easy to miss because
		// nothing errors: the screen simply never appears.
		TestFalse(FString::Printf(TEXT("Row %s should have a trigger condition"), *RowLabel),
			Row.Definition->RequiredTags.IsEmpty());

		for (const FGameplayTag& RequiredTag : Row.Definition->RequiredTags)
		{
			TestTrue(
				FString::Printf(TEXT("Row %s requires the registered tag %s"),
					*RowLabel, *RequiredTag.ToString()),
				RequiredTag.IsValid() &&
					FGameplayTag::RequestGameplayTag(RequiredTag.GetTagName(), false).IsValid());
		}

		// Each row waits on the previous one, which is what keeps the screens in order
		// when two moments happen close together.
		if (PreviousCompletionTag.IsValid())
		{
			TestTrue(
				FString::Printf(TEXT("Row %s waits for the previous overlay"), *RowLabel),
				Row.Definition->RequiredTags.HasTagExact(PreviousCompletionTag));
		}
		PreviousCompletionTag = Row.Definition->CompletionTag;
	}
	return true;
}

#endif
