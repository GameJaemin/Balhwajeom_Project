#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "Investigation/EvidenceDefinitions.h"
#include "Investigation/PhotoDefinitions.h"
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
		const TSubclassOf<UUserWidget> DiaryClass = DiaryBlueprint->GeneratedClass.Get();
		TestTrue(TEXT("WBP_Diary can be hosted by the native modal"),
			Modal->Present(DiaryClass, FText::GetEmpty(), {}));
		const UOverlay* ModalRoot = Cast<UOverlay>(
			Modal->GetWidgetFromName(TEXT("InteractionModalRoot")));
		const UUserWidget* DiaryWidget = ModalRoot && ModalRoot->GetChildrenCount() == 1
			? Cast<UUserWidget>(ModalRoot->GetChildAt(0))
			: nullptr;
		TestNotNull(TEXT("modal contains only the authored diary widget"), DiaryWidget);
		TestNull(TEXT("modal no longer creates a hard-coded close button"),
			Modal->GetWidgetFromName(TEXT("BTN_Close")));
		TestNull(TEXT("modal no longer creates a hard-coded keyword list"),
			Modal->GetWidgetFromName(TEXT("KeywordList")));
		Modal->RemoveFromRoot();
	}

	const UDataTable* EvidenceStates = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates.DT_EvidenceStates"));
	const UDataTable* Photos = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_Photos.DT_Photos"));
	if (!TestNotNull(TEXT("DT_EvidenceStates exists"), EvidenceStates) ||
		!TestNotNull(TEXT("DT_Photos exists"), Photos))
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
	const FPhotoDefinition* DiaryPhoto = Photos->FindRow<FPhotoDefinition>(
		TEXT("PHOTO_01_004"), TEXT("DiaryInteractionAssetsTest"));
	if (!TestNotNull(TEXT("closed diary state exists"), ClosedState) ||
		!TestNotNull(TEXT("open diary state exists"), OpenState) ||
		!TestNotNull(TEXT("memory diary state exists"), MemoryState) ||
		!TestNotNull(TEXT("diary photo exists"), DiaryPhoto))
	{
		return false;
	}

	TestEqual(TEXT("closed diary changes state"), ClosedState->InteractionBehavior,
		EEvidenceInteractionBehavior::ChangeState);
	TestEqual(TEXT("closed diary opens the modal"), ClosedState->InteractionPresentation,
		EEvidenceInteractionPresentation::ModalWidget);
	TestEqual(TEXT("closed diary transitions to open"), ClosedState->NextStateID,
		FName(TEXT("STATE_01_004_OPEN")));
	TestTrue(TEXT("closed diary interaction grants no keywords"),
		ClosedState->GrantedWordIDs.IsEmpty());
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
	TestTrue(TEXT("memory diary interaction grants no keywords"),
		MemoryState->GrantedWordIDs.IsEmpty());
	TestEqual(TEXT("memory diary resolves the shared story photo"),
		MemoryState->PhotoID, FName(TEXT("PHOTO_01_004")));

	TestEqual(TEXT("capturing the diary grants exactly three keywords"),
		DiaryPhoto->GrantedWordIDs.Num(), 3);
	TestTrue(TEXT("diary photo grants WORD_01_004"),
		DiaryPhoto->GrantedWordIDs.Contains(FName(TEXT("WORD_01_004"))));
	TestTrue(TEXT("diary photo grants WORD_01_005"),
		DiaryPhoto->GrantedWordIDs.Contains(FName(TEXT("WORD_01_005"))));
	TestTrue(TEXT("diary photo grants WORD_01_028"),
		DiaryPhoto->GrantedWordIDs.Contains(FName(TEXT("WORD_01_028"))));
	return !HasAnyErrors();
}

#endif
