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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateAdvancesChapter01PhasesTest,
	"Balhwajeom.StoryState.Progression.Chapter01Phases",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateAdvancesChapter01PhasesTest::RunTest(
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

	const FGameplayTag Phase01Completed = FGameplayTag::RequestGameplayTag(
		TEXT("Story.Chapter.01.Phase.01.Completed"), false);
	const FGameplayTag Phase02Completed = FGameplayTag::RequestGameplayTag(
		TEXT("Story.Chapter.01.Phase.02.Completed"), false);
	const FGameplayTag Phase03Completed = FGameplayTag::RequestGameplayTag(
		TEXT("Story.Chapter.01.Phase.03.Completed"), false);

	if (!TestTrue(TEXT("Chapter 01 phase 01 completion tag should be registered"), Phase01Completed.IsValid()) ||
		!TestTrue(TEXT("Chapter 01 phase 02 completion tag should be registered"), Phase02Completed.IsValid()) ||
		!TestTrue(TEXT("Chapter 01 phase 03 completion tag should be registered"), Phase03Completed.IsValid()))
	{
		return false;
	}

	StoryState->ClearStateTags();
	FEditorScriptExecutionGuard ScriptExecutionGuard;

	Investigation->OnSentenceSolved.Broadcast(TEXT("SENT_01_PHOTO_003"));
	TestTrue(
		TEXT("Solving the phase 01 hair iron sentence should complete phase 01"),
		StoryState->HasStateTagExact(Phase01Completed));
	TestFalse(
		TEXT("Phase 02 should remain incomplete before both of its sentences are solved"),
		StoryState->HasStateTagExact(Phase02Completed));

	Investigation->OnSentenceSolved.Broadcast(TEXT("SENT_01_PHOTO_002"));
	TestFalse(
		TEXT("Solving only the diary sentence should not complete phase 02"),
		StoryState->HasStateTagExact(Phase02Completed));

	Investigation->OnSentenceSolved.Broadcast(TEXT("SENT_01_PHOTO_009"));
	TestTrue(
		TEXT("Solving both phase 02 sentences should complete phase 02"),
		StoryState->HasStateTagExact(Phase02Completed));
	TestFalse(
		TEXT("Phase 03 should remain incomplete before its sentence is solved"),
		StoryState->HasStateTagExact(Phase03Completed));

	AddExpectedMessage(
		TEXT("게임 완료"),
		ELogVerbosity::Display,
		EAutomationExpectedMessageFlags::Contains,
		1);
	Investigation->OnSentenceSolved.Broadcast(TEXT("SENT_01_PHOTO_010"));
	TestTrue(
		TEXT("Solving the phase 03 note sentence should complete phase 03"),
		StoryState->HasStateTagExact(Phase03Completed));

	return true;
}

#endif
