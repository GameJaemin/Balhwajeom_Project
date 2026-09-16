#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Interaction/GateDoorLockedFeedback.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGateDoorLockedFeedbackStageOrderTest,
	"Balhwajeom.Interaction.GateDoor.LockedFeedback.StageOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGateDoorLockedFeedbackStageOrderTest::RunTest(const FString& Parameters)
{
	const FGameplayTag InspectA = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.State.STATE_01_001_CLEAR"), false);
	const FGameplayTag InspectB = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.State.STATE_01_002_CLEAR"), false);
	const FGameplayTag InspectC = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.State.STATE_01_003_CLEAR"), false);
	const FGameplayTag PhotoA = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.Photographed.OBJ_01_001"), false);
	const FGameplayTag PhotoB = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.Photographed.OBJ_01_002"), false);
	const FGameplayTag PhotoC = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.Photographed.OBJ_01_003"), false);
	const FGameplayTag Solved = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.SentenceSolved.PHOTO_01_003"), false);

	if (!TestTrue(TEXT("All room3 feedback tags should be registered"),
		InspectA.IsValid() && InspectB.IsValid() && InspectC.IsValid() &&
		PhotoA.IsValid() && PhotoB.IsValid() && PhotoC.IsValid() && Solved.IsValid()))
	{
		return false;
	}

	TArray<FGateDoorLockedFeedbackStage> Stages;
	FGateDoorLockedFeedbackStage& InspectStage = Stages.AddDefaulted_GetRef();
	InspectStage.CompleteWhenAllTags.AddTag(InspectA);
	InspectStage.CompleteWhenAllTags.AddTag(InspectB);
	InspectStage.CompleteWhenAllTags.AddTag(InspectC);
	InspectStage.IncompleteMessage = FText::FromString(TEXT("Inspect"));

	FGateDoorLockedFeedbackStage& CaptureStage = Stages.AddDefaulted_GetRef();
	CaptureStage.CompleteWhenAllTags.AddTag(PhotoA);
	CaptureStage.CompleteWhenAllTags.AddTag(PhotoB);
	CaptureStage.CompleteWhenAllTags.AddTag(PhotoC);
	CaptureStage.IncompleteMessage = FText::FromString(TEXT("Capture"));

	FGateDoorLockedFeedbackStage& SolveStage = Stages.AddDefaulted_GetRef();
	SolveStage.CompleteWhenAllTags.AddTag(Solved);
	SolveStage.IncompleteMessage = FText::FromString(TEXT("Solve"));

	FGameplayTagContainer ProgressTags;
	TestEqual(TEXT("No progress should show the first-stage message"),
		GateDoorLockedFeedback::ResolveFirstIncompleteMessage(Stages, ProgressTags).ToString(),
		FString(TEXT("Inspect")));

	ProgressTags.AddTag(InspectA);
	ProgressTags.AddTag(PhotoA);
	TestEqual(TEXT("Later progress must not skip an incomplete earlier stage"),
		GateDoorLockedFeedback::ResolveFirstIncompleteMessage(Stages, ProgressTags).ToString(),
		FString(TEXT("Inspect")));

	ProgressTags.AddTag(InspectB);
	ProgressTags.AddTag(InspectC);
	TestEqual(TEXT("Completed inspection should advance to capture guidance"),
		GateDoorLockedFeedback::ResolveFirstIncompleteMessage(Stages, ProgressTags).ToString(),
		FString(TEXT("Capture")));

	ProgressTags.AddTag(PhotoB);
	ProgressTags.AddTag(PhotoC);
	ProgressTags.AddTag(Solved);
	TestTrue(TEXT("All completed stages should return no guidance"),
		GateDoorLockedFeedback::ResolveFirstIncompleteMessage(Stages, ProgressTags)
			.IsEmptyOrWhitespace());

	return true;
}

#endif
