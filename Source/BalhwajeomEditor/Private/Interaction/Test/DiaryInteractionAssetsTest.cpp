#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Investigation/EvidenceDefinitions.h"
#include "UI/BalhwajeomInteractionModalWidget.h"
#include "WidgetBlueprint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDiaryInteractionAssetsTest,
	"Balhwajeom.Interaction.DiaryAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDiaryInteractionAssetsTest::RunTest(const FString& Parameters)
{
	const UWidgetBlueprint* DiaryBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Diary/WBP_Diary.WBP_Diary"));
	if (!TestNotNull(TEXT("WBP_Diary exists"), DiaryBlueprint) ||
		!TestNotNull(TEXT("WBP_Diary has a widget tree"),
			DiaryBlueprint ? DiaryBlueprint->WidgetTree.Get() : nullptr))
	{
		return false;
	}
	TestNotNull(TEXT("WBP_Diary has a root visual"),
		DiaryBlueprint->WidgetTree->RootWidget.Get());

	UBalhwajeomInteractionModalWidget* Modal = GEditor
		? CreateWidget<UBalhwajeomInteractionModalWidget>(
			GEditor->GetEditorWorldContext().World())
		: nullptr;
	if (TestNotNull(TEXT("native interaction modal can be created"), Modal))
	{
		Modal->AddToRoot();
		TArray<FText> Keywords{
			FText::FromString(TEXT("엄마와")),
			FText::FromString(TEXT("다투었다."))};
		const TSubclassOf<UUserWidget> DiaryClass = DiaryBlueprint->GeneratedClass.Get();
		TestTrue(TEXT("WBP_Diary can be hosted by the native modal"),
			Modal->Present(DiaryClass, FText::GetEmpty(), Keywords));
		const UOverlay* ModalRoot = Cast<UOverlay>(
			Modal->GetWidgetFromName(TEXT("InteractionModalRoot")));
		const UUserWidget* DiaryWidget = ModalRoot && ModalRoot->GetChildrenCount() == 1
			? Cast<UUserWidget>(ModalRoot->GetChildAt(0))
			: nullptr;
		if (TestNotNull(TEXT("modal contains only the authored diary widget"), DiaryWidget))
		{
			const UTextBlock* Keyword1 = Cast<UTextBlock>(
				DiaryWidget->GetWidgetFromName(TEXT("Text_Keyword1")));
			const UTextBlock* Keyword2 = Cast<UTextBlock>(
				DiaryWidget->GetWidgetFromName(TEXT("Text_Keyword2")));
			if (TestNotNull(TEXT("WBP_Diary has Text_Keyword1"), Keyword1))
			{
				TestTrue(TEXT("first keyword is written to Text_Keyword1"),
					Keyword1->GetText().EqualTo(Keywords[0]));
			}
			if (TestNotNull(TEXT("WBP_Diary has Text_Keyword2"), Keyword2))
			{
				TestTrue(TEXT("second keyword is written to Text_Keyword2"),
					Keyword2->GetText().EqualTo(Keywords[1]));
			}
		}
		TestNull(TEXT("modal no longer creates a hard-coded close button"),
			Modal->GetWidgetFromName(TEXT("BTN_Close")));
		TestNull(TEXT("modal no longer creates a hard-coded keyword list"),
			Modal->GetWidgetFromName(TEXT("KeywordList")));
		Modal->RemoveFromRoot();
	}

	const UDataTable* EvidenceStates = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates.DT_EvidenceStates"));
	if (!TestNotNull(TEXT("DT_EvidenceStates exists"), EvidenceStates))
	{
		return false;
	}

	const FEvidenceStateDefinition* ClosedState =
		EvidenceStates->FindRow<FEvidenceStateDefinition>(
			TEXT("STATE_01_004_CLOSED"), TEXT("DiaryInteractionAssetsTest"));
	const FEvidenceStateDefinition* OpenState =
		EvidenceStates->FindRow<FEvidenceStateDefinition>(
			TEXT("STATE_01_004_OPEN"), TEXT("DiaryInteractionAssetsTest"));
	const FEvidenceStateDefinition* MemoryState =
		EvidenceStates->FindRow<FEvidenceStateDefinition>(
			TEXT("STATE_01_004_MEMORY"), TEXT("DiaryInteractionAssetsTest"));
	if (!TestNotNull(TEXT("closed diary state exists"), ClosedState) ||
		!TestNotNull(TEXT("open diary state exists"), OpenState) ||
		!TestNotNull(TEXT("memory diary state exists"), MemoryState))
	{
		return false;
	}

	TestEqual(TEXT("closed diary changes state"), ClosedState->InteractionBehavior,
		EEvidenceInteractionBehavior::ChangeState);
	TestEqual(TEXT("closed diary opens the modal"), ClosedState->InteractionPresentation,
		EEvidenceInteractionPresentation::ModalWidget);
	TestEqual(TEXT("closed diary transitions to open"), ClosedState->NextStateID,
		FName(TEXT("STATE_01_004_OPEN")));
	TestEqual(TEXT("closed diary grants both keywords"),
		ClosedState->GrantedWordIDs.Num(), 2);
	TestTrue(TEXT("closed diary modal class resolves"),
		ClosedState->InteractionWidgetClass.LoadSynchronous() != nullptr);
	TestTrue(TEXT("closed diary defers its world story until the modal closes"),
		ClosedState->bPlayWorldStoryAfterPresentation);
	TestEqual(TEXT("closed diary can resolve the shared story photo"),
		ClosedState->PhotoID, FName(TEXT("PHOTO_01_004")));

	TestEqual(TEXT("open diary remains repeatable"), OpenState->InteractionBehavior,
		EEvidenceInteractionBehavior::Repeatable);
	TestEqual(TEXT("open diary reopens the modal"), OpenState->InteractionPresentation,
		EEvidenceInteractionPresentation::ModalWidget);
	TestTrue(TEXT("open diary does not grant duplicate keywords"),
		OpenState->GrantedWordIDs.IsEmpty());
	TestTrue(TEXT("open diary defers its world story until the modal closes"),
		OpenState->bPlayWorldStoryAfterPresentation);

	TestEqual(TEXT("memory diary also opens the modal"),
		MemoryState->InteractionPresentation,
		EEvidenceInteractionPresentation::ModalWidget);
	TestTrue(TEXT("memory diary uses the same modal class"),
		MemoryState->InteractionWidgetClass.LoadSynchronous() ==
		ClosedState->InteractionWidgetClass.LoadSynchronous());
	TestTrue(TEXT("memory diary plays its world story only after the modal closes"),
		MemoryState->bPlayWorldStoryAfterPresentation);
	TestEqual(TEXT("memory diary remains repeatable"),
		MemoryState->InteractionBehavior,
		EEvidenceInteractionBehavior::Repeatable);
	TestEqual(TEXT("memory diary resolves the shared story photo"),
		MemoryState->PhotoID, FName(TEXT("PHOTO_01_004")));
	return !HasAnyErrors();
}

#endif
