#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Story/StoryStateSubsystem.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


namespace StoryStateSentenceSolvedIntegrationTests
{
	struct FFixture
	{
		FFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
		}

		~FFixture()
		{
			if (!GameInstance)
			{
				return;
			}

			GameInstance->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateTracksSolvedPhotoSentenceTest,
	"Balhwajeom.StoryState.Evidence.SentenceSolvedTracksPhoto",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateTracksSolvedPhotoSentenceTest::RunTest(
	const FString& Parameters
)
{
	const StoryStateSentenceSolvedIntegrationTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState =
		Fixture.GameInstance->GetSubsystem<UStoryStateSubsystem>();
	UBalhwajeomInvestigationSubsystem* Investigation =
		Fixture.GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>();

	if (!TestNotNull(TEXT("StoryState subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Investigation subsystem should exist"), Investigation))
	{
		return false;
	}

	const FGameplayTag SentenceSolvedTag = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.SentenceSolved.PHOTO_01_006"),
		false
	);
	if (!TestTrue(
		TEXT("The configured sentence-solved tag should be registered"),
		SentenceSolvedTag.IsValid()))
	{
		return false;
	}

	StoryState->ClearStateTags();

	const FName SentenceID = TEXT("SENT_01_PHOTO_004");
	FSentenceDefinition Sentence;
	if (!TestTrue(
		TEXT("The photo sentence should exist"),
		Investigation->GetSentenceDefinition(SentenceID, Sentence)))
	{
		return false;
	}

	FSentenceSubmission Submission;
	for (const FSentenceWordSlot& Slot : Sentence.WordSlots)
	{
		Investigation->AcquireWord(
			Slot.CorrectWordID,
			EWordAcquisitionSource::Default,
			TEXT("StoryStateSentenceSolvedAutomation")
		);
		Submission.SubmittedWords.Add({Slot.SlotIndex, Slot.CorrectWordID});
	}

	FText ResultText;
	bool bSolved = false;
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		bSolved = Investigation->ValidateSentence(
			SentenceID,
			Submission,
			ResultText
		);
	}

	TestTrue(TEXT("A correct photo sentence should be solved"), bSolved);
	TestTrue(
		TEXT("Solving a photo sentence should add its photo-based story tag"),
		StoryState->HasStateTagExact(SentenceSolvedTag)
	);
	TestEqual(
		TEXT("The solved photo sentence should add one exact story tag"),
		StoryState->GetCurrentStateTags().Num(),
		1
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateIgnoresSolvedStatementSentenceTest,
	"Balhwajeom.StoryState.Evidence.SentenceSolvedIgnoresStatement",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateIgnoresSolvedStatementSentenceTest::RunTest(
	const FString& Parameters
)
{
	const StoryStateSentenceSolvedIntegrationTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState =
		Fixture.GameInstance->GetSubsystem<UStoryStateSubsystem>();
	UBalhwajeomInvestigationSubsystem* Investigation =
		Fixture.GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>();

	if (!TestNotNull(TEXT("StoryState subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Investigation subsystem should exist"), Investigation))
	{
		return false;
	}

	StoryState->ClearStateTags();
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		Investigation->OnSentenceSolved.Broadcast(TEXT("SENT_01_STATEMENT_01"));
	}

	TestTrue(
		TEXT("A solved statement should not add a photo sentence story tag"),
		StoryState->GetCurrentStateTags().IsEmpty()
	);

	return true;
}

#endif
