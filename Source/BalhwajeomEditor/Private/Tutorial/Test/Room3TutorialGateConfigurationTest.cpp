#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "WidgetBlueprint.h"
#include "Interaction/BalhwajeomGateDoorActor.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Tutorial/BalhwajeomTutorialDirector.h"
#include "Tutorial/BalhwajeomTutorialFlow.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoom3TutorialGateConfigurationTest,
	"Balhwajeom.Tutorial.Room3.GateConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoom3TutorialGateConfigurationTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* FeedbackWidget = LoadObject<UWidgetBlueprint>(
		nullptr, TEXT("/Game/Balhwajeom/UI/HUD/WBP_Check.WBP_Check"));
	if (!TestNotNull(TEXT("WBP_Check should load"), FeedbackWidget) ||
		!TestNotNull(TEXT("WBP_Check should have a widget tree"),
			FeedbackWidget ? FeedbackWidget->WidgetTree.Get() : nullptr))
	{
		return false;
	}

	UWidget* MessageTarget = FeedbackWidget->WidgetTree->FindWidget(TEXT("MessageText"));
	if (!MessageTarget)
	{
		MessageTarget = FeedbackWidget->WidgetTree->FindWidget(TEXT("Text"));
	}
	if (!MessageTarget)
	{
		TArray<UWidget*> AllWidgets;
		FeedbackWidget->WidgetTree->GetAllWidgets(AllWidgets);
		TArray<UTextBlock*> TextBlocks;
		for (UWidget* Widget : AllWidgets)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				TextBlocks.Add(TextBlock);
			}
		}
		if (TextBlocks.Num() == 1)
		{
			MessageTarget = TextBlocks[0];
		}
	}
	if (!TestNotNull(TEXT("WBP_Check should expose a Text or MessageText target"),
		MessageTarget))
	{
		return false;
	}
	TestTrue(TEXT("WBP_Check message target should support SetText(FText)"),
		Cast<UTextBlock>(MessageTarget) != nullptr ||
		MessageTarget->FindFunction(TEXT("SetText")) != nullptr);

	UWorld* Room3 = LoadObject<UWorld>(nullptr, TEXT("/Game/Levels/room3.room3"));
	if (!TestNotNull(TEXT("room3 level should load"), Room3) ||
		!TestNotNull(TEXT("room3 should have a persistent level"), Room3->PersistentLevel.Get()))
	{
		return false;
	}

	ABalhwajeomGateDoorActor* ExitDoor = nullptr;
	ABalhwajeomTutorialDirector* TutorialDirector = nullptr;
	for (AActor* Actor : Room3->PersistentLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetActorLabel() == TEXT("GateDoor_Exit"))
		{
			ExitDoor = Cast<ABalhwajeomGateDoorActor>(Actor);
		}
		if (!TutorialDirector)
		{
			TutorialDirector = Cast<ABalhwajeomTutorialDirector>(Actor);
		}
	}

	if (!TestNotNull(TEXT("room3 should contain GateDoor_Exit"), ExitDoor) ||
		!TestNotNull(TEXT("room3 should contain a tutorial director"), TutorialDirector))
	{
		return false;
	}

	UBalhwajeomTutorialFlow* Flow = TutorialDirector->GetFlow();
	if (!TestNotNull(TEXT("room3 tutorial director should have a flow"), Flow))
	{
		return false;
	}

	TestEqual(
		TEXT("room3 should use the shipped Room2 tutorial flow"),
		Flow->GetPathName(),
		FString(TEXT("/Game/Balhwajeom/Data/Tutorial/DA_TutorialFlow_Room2.DA_TutorialFlow_Room2")));

	const FGameplayTag FamilyPhotoSolvedTag = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.SentenceSolved.PHOTO_01_003"), false);
	if (!TestTrue(TEXT("The family-photo sentence-solved tag should exist"),
		FamilyPhotoSolvedTag.IsValid()))
	{
		return false;
	}

	const UDoorInteractionComponent* DoorInteraction = ExitDoor->GetDoorInteraction();
	if (!TestNotNull(TEXT("GateDoor_Exit should have a door interaction component"),
		DoorInteraction))
	{
		return false;
	}

	TestEqual(TEXT("GateDoor_Exit should require exactly one progress tag"),
		DoorInteraction->UnlockRequiresTags.Num(), 1);
	TestTrue(TEXT("GateDoor_Exit should require the completed family photo"),
		DoorInteraction->UnlockRequiresTags.HasTagExact(FamilyPhotoSolvedTag));

	const TArray<FGateDoorLockedFeedbackStage>& FeedbackStages =
		ExitDoor->GetLockedFeedbackStages();
	if (!TestEqual(TEXT("GateDoor_Exit should have three ordered feedback stages"),
		FeedbackStages.Num(), 3))
	{
		return false;
	}

	const TArray<TArray<FName>> ExpectedStageTags = {
		{
			TEXT("Evidence.State.STATE_01_001_CLEAR"),
			TEXT("Evidence.State.STATE_01_002_CLEAR"),
			TEXT("Evidence.State.STATE_01_003_CLEAR")
		},
		{
			TEXT("Evidence.Photographed.OBJ_01_001"),
			TEXT("Evidence.Photographed.OBJ_01_002"),
			TEXT("Evidence.Photographed.OBJ_01_003")
		},
		{ TEXT("Evidence.SentenceSolved.PHOTO_01_003") }
	};
	const TArray<FString> ExpectedFeedbackMessages = {
		TEXT("[F]를 눌러 아직 조사하지 않은 액자를 살펴보자."),
		TEXT("우클릭으로 카메라를 켜고, 아직 찍지 않은 액자를 촬영해 보자."),
		TEXT("[TAB]으로 태블릿을 열고, 여동생 폴더의 가족 사진 추리를 완성해 보자.")
	};

	for (int32 StageIndex = 0; StageIndex < FeedbackStages.Num(); ++StageIndex)
	{
		const FGateDoorLockedFeedbackStage& Stage = FeedbackStages[StageIndex];
		TestEqual(
			FString::Printf(TEXT("Feedback stage %d should use the approved message"), StageIndex),
			Stage.IncompleteMessage.ToString(),
			ExpectedFeedbackMessages[StageIndex]);
		TestEqual(
			FString::Printf(TEXT("Feedback stage %d should require the expected tag count"), StageIndex),
			Stage.CompleteWhenAllTags.Num(),
			ExpectedStageTags[StageIndex].Num());

		for (const FName ExpectedTagName : ExpectedStageTags[StageIndex])
		{
			const FGameplayTag ExpectedTag = FGameplayTag::RequestGameplayTag(
				ExpectedTagName, false);
			TestTrue(
				FString::Printf(TEXT("Feedback stage %d should contain %s"),
					StageIndex, *ExpectedTagName.ToString()),
				ExpectedTag.IsValid() && Stage.CompleteWhenAllTags.HasTagExact(ExpectedTag));
		}
	}

	const FGameplayTag TabletLockTag = FGameplayTag::RequestGameplayTag(
		TEXT("Runtime.Lock.Tablet"), false);
	const FGameplayTag FamilyPhotoStageTag = FGameplayTag::RequestGameplayTag(
		TEXT("Tutorial.Stage.CompleteFamilyPhoto"), false);
	if (!TestTrue(TEXT("The tablet lock tag should exist"), TabletLockTag.IsValid()) ||
		!TestTrue(TEXT("The family-photo tutorial stage tag should exist"),
			FamilyPhotoStageTag.IsValid()))
	{
		return false;
	}

	const FBalhwajeomTutorialStep* AnalysisStep = Flow->Steps.FindByPredicate(
		[](const FBalhwajeomTutorialStep& Step)
		{
			return Step.StepID == TEXT("CompleteFamilyPhoto");
		});
	if (!TestNotNull(TEXT("The flow should contain CompleteFamilyPhoto"), AnalysisStep))
	{
		return false;
	}

	TestEqual(TEXT("CompleteFamilyPhoto should publish its tutorial stage"),
		AnalysisStep->StageTag, FamilyPhotoStageTag);
	TestTrue(TEXT("CompleteFamilyPhoto should unlock the tablet on entry"),
		AnalysisStep->RemoveOnEnter.HasTagExact(TabletLockTag));
	TestEqual(TEXT("CompleteFamilyPhoto should require one completion tag"),
		AnalysisStep->CompleteWhenAllTags.Num(), 1);
	TestTrue(TEXT("CompleteFamilyPhoto should wait for the solved family photo"),
		AnalysisStep->CompleteWhenAllTags.HasTagExact(FamilyPhotoSolvedTag));

	return true;
}

#endif
