#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"
#include "Tutorial/BalhwajeomTutorialDirector.h"
#include "Tutorial/BalhwajeomTutorialFlow.h"
#include "UObject/ConstructorHelpers.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


/**
 * Drives the shipped Room2 tutorial flow against the shipped investigation DataTables.
 *
 * Everything here is real content: DA_TutorialFlow_Room2, DT_EvidenceDefinitions and
 * DT_EvidenceStates as configured for the game. A typo in a tag name, a missing
 * PostCaptureStateID, or an unregistered gameplay tag breaks this test rather than
 * turning into an unplayable level.
 */
namespace Room2TutorialFlowTest
{
	const TCHAR* FlowAssetPath =
		TEXT("/Game/Balhwajeom/Data/Tutorial/DA_TutorialFlow_Room2.DA_TutorialFlow_Room2");

	const FName ObjectIDs[3] = {
		TEXT("OBJ_01_001"), TEXT("OBJ_01_002"), TEXT("OBJ_01_003") };
	const FName PhotoIDs[3] = {
		TEXT("PHOTO_01_001"), TEXT("PHOTO_01_002"), TEXT("PHOTO_01_003") };
	const FName ClearStateIDs[3] = {
		TEXT("STATE_01_001_CLEAR"), TEXT("STATE_01_002_CLEAR"), TEXT("STATE_01_003_CLEAR") };
	const FName MemoryStateIDs[3] = {
		TEXT("STATE_01_001_MEMORY"), TEXT("STATE_01_002_MEMORY"), TEXT("STATE_01_003_MEMORY") };
	const FName FamilyPhotoSentenceID = TEXT("SENT_01_PHOTO_001");

	struct FFixture
	{
		FFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			if (World)
			{
				// Dynamic delegates only reach actors once the world's actors are initialized.
				const FURL URL;
				World->InitializeActorsForPlay(URL);
				World->BeginPlay();
			}
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

		UStoryStateSubsystem* StoryState() const
		{
			return GameInstance ? GameInstance->GetSubsystem<UStoryStateSubsystem>() : nullptr;
		}

		UBalhwajeomInvestigationSubsystem* Investigation() const
		{
			return GameInstance
				? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
				: nullptr;
		}

		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRoom2TutorialFlowEndToEndTest,
	"Balhwajeom.Tutorial.Room2.FlowEndToEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRoom2TutorialFlowEndToEndTest::RunTest(const FString& Parameters)
{
	using namespace Room2TutorialFlowTest;

	UBalhwajeomTutorialFlow* Flow = LoadObject<UBalhwajeomTutorialFlow>(nullptr, FlowAssetPath);
	if (!TestNotNull(TEXT("DA_TutorialFlow_Room2 should exist"), Flow))
	{
		return false;
	}

	FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.StoryState();
	UBalhwajeomInvestigationSubsystem* Investigation = Fixture.Investigation();
	if (!TestNotNull(TEXT("Story state subsystem"), StoryState) ||
		!TestNotNull(TEXT("Investigation subsystem"), Investigation) ||
		!TestNotNull(TEXT("World"), Fixture.World))
	{
		return false;
	}

	for (const FName& ClearStateID : ClearStateIDs)
	{
		FEvidenceStateDefinition ClearState;
		TestTrue(
			FString::Printf(TEXT("%s should exist"), *ClearStateID.ToString()),
			Investigation->GetEvidenceStateDefinition(ClearStateID, ClearState));
		TestTrue(
			FString::Printf(TEXT("%s should not show a redundant camera instruction"),
				*ClearStateID.ToString()),
			ClearState.InteractionText.IsEmpty());
	}

	// Deferred so the flow is in place before BeginPlay auto-starts it.
	ABalhwajeomTutorialDirector* Director =
		Fixture.World->SpawnActorDeferred<ABalhwajeomTutorialDirector>(
			ABalhwajeomTutorialDirector::StaticClass(), FTransform::Identity);
	if (Director)
	{
		Director->SetFlow(Flow);
		Director->FinishSpawning(FTransform::Identity);
	}

	ABalhwajeomCameraCharacter* Character =
		Fixture.World->SpawnActor<ABalhwajeomCameraCharacter>();
	if (!TestNotNull(TEXT("Director"), Director) || !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	if (!Character->HasActorBegunPlay())
	{
		Character->DispatchBeginPlay();
	}

	UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();

	// The exit door is gated on completing the family photo's analysis sentence.
	AActor* DoorActor = Fixture.World->SpawnActor<AActor>();
	UDoorInteractionComponent* Door = NewObject<UDoorInteractionComponent>(DoorActor);
	DoorActor->AddInstanceComponent(Door);
	Door->RegisterComponent();
	const FGameplayTag FamilyPhotoSolvedTag = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.SentenceSolved.PHOTO_01_003"), false);
	if (!TestTrue(
		TEXT("The family-photo sentence-solved tag must be registered"),
		FamilyPhotoSolvedTag.IsValid()))
	{
		return false;
	}
	Door->UnlockRequiresTags.AddTag(FamilyPhotoSolvedTag);

	// BeginPlay already auto-started it; this is a no-op that documents the entry point.
	Director->StartFlow();

	// --- Step 0: only F works ----------------------------------------------------------
	TestEqual(TEXT("The flow starts on DustTeach"),
		Director->GetCurrentStepID(), FName(TEXT("DustTeach")));
	// With no text on screen, the blinking [F] prompt is the entire instruction.
	TestTrue(TEXT("DustTeach blinks the [F] prompt"),
		ABalhwajeomTutorialDirector::GetTutorialHintTarget(Character) ==
			EBalhwajeomTutorialHintTarget::InteractPrompt);
	TestTrue(TEXT("The photo camera starts locked"),
		PhotoCamera && PhotoCamera->IsLockedByStoryState());
	TestFalse(TEXT("The exit door starts locked"), Door->IsUnlocked());

	// --- Dusting: F on each photo advances its evidence state --------------------------
	FGuid InstanceIDs[3];
	for (int32 Index = 0; Index < 3; ++Index)
	{
		InstanceIDs[Index] = FGuid::NewGuid();
		FName CurrentStateID = NAME_None;
		if (!TestTrue(
			FString::Printf(TEXT("%s should register"), *ObjectIDs[Index].ToString()),
			Investigation->RegisterEvidenceActor(
				InstanceIDs[Index], ObjectIDs[Index], CurrentStateID)))
		{
			return false;
		}

		FEvidenceInteractionViewData ViewData;
		if (!TestTrue(
			FString::Printf(TEXT("%s should offer its dust interaction"),
				*ObjectIDs[Index].ToString()),
			Investigation->BeginEvidenceInteraction(InstanceIDs[Index], ViewData)))
		{
			return false;
		}
		TestEqual(
			FString::Printf(TEXT("%s dust interaction should use the family modal"),
				*ObjectIDs[Index].ToString()),
			ViewData.Presentation,
			EEvidenceInteractionPresentation::ModalWidget);
		TestEqual(
			FString::Printf(TEXT("%s dust interaction should open the matching family widget"),
				*ObjectIDs[Index].ToString()),
			ViewData.InteractionWidgetClass.ToSoftObjectPath().ToString(),
			FString::Printf(
				TEXT("/Game/Balhwajeom/UI/Family/WBP_Family%d.WBP_Family%d_C"),
				Index + 1,
				Index + 1));
		TestTrue(
			FString::Printf(TEXT("%s dust interaction should complete"),
				*ObjectIDs[Index].ToString()),
			Investigation->CompleteEvidenceInteraction(InstanceIDs[Index], ViewData.StateID));

		if (Index == 0)
		{
			// One dust-off teaches F, and the camera prompt follows immediately rather
			// than waiting for the other two photos.
			TestEqual(TEXT("The first dust-off goes straight to PhotoPrompt"),
				Director->GetCurrentStepID(), FName(TEXT("PhotoPrompt")));
			TestFalse(TEXT("The first dust-off unlocks the photo camera"),
				PhotoCamera && PhotoCamera->IsLockedByStoryState());
			TestTrue(TEXT("The camera icon is highlighted right away"),
				ABalhwajeomTutorialDirector::GetTutorialHintTarget(Character) ==
					EBalhwajeomTutorialHintTarget::PhotoCameraIcon);
		}
		else
		{
			// Dusting the rest is ordinary play and must not move the flow along.
			TestEqual(TEXT("Dusting the other photos keeps the camera prompt up"),
				Director->GetCurrentStepID(), FName(TEXT("PhotoPrompt")));
		}
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FGameplayTag ClearTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Evidence.State.%s"), *ClearStateIDs[Index].ToString())),
			false);
		TestTrue(
			FString::Printf(TEXT("Dusting %s should record %s"),
				*ObjectIDs[Index].ToString(), *ClearStateIDs[Index].ToString()),
			ClearTag.IsValid() && StoryState->HasStateTagExact(ClearTag));
	}

	// --- PhotoPrompt: the camera is unlocked and its icon is the only bright thing ----
	TestTrue(TEXT("PhotoPrompt dims the screen"),
		ABalhwajeomTutorialDirector::GetTutorialDimOpacity(Character) > 0.0f);

	// --- Entering camera mode ends the prompt and clears the dim -----------------------
	StoryState->SetPlayerModeTag(BalhwajeomGameplayTags::Runtime_Player_Mode_PhotoCamera);
	TestEqual(TEXT("Raising the camera reaches Photograph"),
		Director->GetCurrentStepID(), FName(TEXT("Photograph")));
	TestEqual(TEXT("Camera mode leaves no dim behind"),
		ABalhwajeomTutorialDirector::GetTutorialDimOpacity(Character), 0.0f);
	StoryState->SetPlayerModeTag(BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration);

	// --- Photographing all three advances each photo into its memory state ------------
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FCapturedPhotoRecord Record;
		Record.PhotoID = PhotoIDs[Index];
		Record.ObjectID = ObjectIDs[Index];
		Record.EvidenceInstanceID = InstanceIDs[Index];
		Record.CapturedStateID = ClearStateIDs[Index];
		Record.ImageRelativePath = TEXT("Room2TutorialTest/Photo.png");
		TestTrue(
			FString::Printf(TEXT("%s should register"), *PhotoIDs[Index].ToString()),
			Investigation->RegisterCapturedPhoto(Record));

		// This swaps the evidence into its optional memory-conversation state.
		TestTrue(
			FString::Printf(TEXT("%s should advance to its memory state"),
				*ObjectIDs[Index].ToString()),
			Investigation->AdvanceEvidenceStateAfterCapture(InstanceIDs[Index]));
	}

	TestEqual(TEXT("Photographing all three reaches CompleteFamilyPhoto"),
		Director->GetCurrentStepID(), FName(TEXT("CompleteFamilyPhoto")));
	TestFalse(TEXT("The tablet unlocks for the family-photo puzzle"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));
	TestTrue(TEXT("CompleteFamilyPhoto highlights the tablet icon"),
		ABalhwajeomTutorialDirector::GetTutorialHintTarget(Character) ==
			EBalhwajeomTutorialHintTarget::TabletIcon);
	TestFalse(TEXT("The exit door stays locked until the family photo is completed"),
		Door->IsUnlocked());

	// --- The old family conversations remain optional and cannot finish the tutorial ---
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(
			FString::Printf(TEXT("%s story-heard tag should be recorded"),
				*ObjectIDs[Index].ToString()),
			StoryState->AddEvidenceStoryHeardTag(ObjectIDs[Index]));
	}
	TestEqual(TEXT("Conversations do not replace the family-photo puzzle"),
		Director->GetCurrentStepID(), FName(TEXT("CompleteFamilyPhoto")));
	TestFalse(TEXT("Hearing every conversation does not unlock the exit door"),
		Door->IsUnlocked());

	// --- Completing the existing family-photo sentence finishes the tutorial ----------
	FSentenceDefinition FamilyPhotoSentence;
	if (!TestTrue(
		TEXT("The family-photo sentence should exist"),
		Investigation->GetSentenceDefinition(FamilyPhotoSentenceID, FamilyPhotoSentence)))
	{
		return false;
	}

	FSentenceSubmission Submission;
	for (const FSentenceWordSlot& Slot : FamilyPhotoSentence.WordSlots)
	{
		TestTrue(
			FString::Printf(TEXT("Capturing the three frames should acquire %s"),
				*Slot.CorrectWordID.ToString()),
			Investigation->HasAcquiredWord(Slot.CorrectWordID));
		Submission.SubmittedWords.Add({Slot.SlotIndex, Slot.CorrectWordID});
	}

	FText ResultText;
	bool bSolved = false;
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		bSolved = Investigation->ValidateSentence(
			FamilyPhotoSentenceID, Submission, ResultText);
	}

	TestTrue(TEXT("The family-photo sentence accepts the three captured words"), bSolved);
	TestTrue(TEXT("Solving the family photo records its photo-based story tag"),
		StoryState->HasStateTagExact(FamilyPhotoSolvedTag));
	TestEqual(TEXT("Completing the family photo reaches Done"),
		Director->GetCurrentStepID(), FName(TEXT("Done")));
	TestTrue(TEXT("Done unlocks the exit door"), Door->IsUnlocked());
	TestTrue(TEXT("An unlocked door offers its interaction"), Door->CanInteract());
	TestEqual(TEXT("Done leaves no dim on screen"),
		ABalhwajeomTutorialDirector::GetTutorialDimOpacity(Character), 0.0f);

	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
