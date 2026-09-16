#include "CameraSystem/BalhwajeomEvidenceActor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Story/StoryStateSubsystem.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


/**
 * Walks the exact route a player takes to open the tutorial's exit door, through the
 * placed actor rather than through the subsystem directly:
 *
 *     F (dust) -> photograph all three -> complete family photo sentence -> door unlocks
 *
 * The test keeps the optional family conversations in the route and proves they cannot
 * unlock the door. Only the real photo-sentence validation path may do that.
 */
namespace TutorialDoorUnlockPathTest
{
	struct FFixture
	{
		FFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			if (World)
			{
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
	FTutorialDoorUnlockPathTest,
	"Balhwajeom.Tutorial.Room2.DoorUnlockPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialDoorUnlockPathTest::RunTest(const FString& Parameters)
{
	using namespace TutorialDoorUnlockPathTest;

	const FName ObjectIDs[3] = {
		TEXT("OBJ_01_001"), TEXT("OBJ_01_002"), TEXT("OBJ_01_003") };
	const FName PhotoIDs[3] = {
		TEXT("PHOTO_01_001"), TEXT("PHOTO_01_002"), TEXT("PHOTO_01_003") };
	const FName MemoryStateIDs[3] = {
		TEXT("STATE_01_001_MEMORY"), TEXT("STATE_01_002_MEMORY"), TEXT("STATE_01_003_MEMORY") };

	FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.StoryState();
	UBalhwajeomInvestigationSubsystem* Investigation = Fixture.Investigation();
	if (!TestNotNull(TEXT("Story state subsystem"), StoryState) ||
		!TestNotNull(TEXT("Investigation subsystem"), Investigation) ||
		!TestNotNull(TEXT("World"), Fixture.World))
	{
		return false;
	}

	// The exit door, gated exactly as room3 places it.
	AActor* DoorActor = Fixture.World->SpawnActor<AActor>();
	UDoorInteractionComponent* Door = NewObject<UDoorInteractionComponent>(DoorActor);
	DoorActor->AddInstanceComponent(Door);
	Door->RegisterComponent();
	const FGameplayTag FamilyPhotoSolvedTag = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.SentenceSolved.PHOTO_01_003"), false);
	if (!TestTrue(
		TEXT("Evidence.SentenceSolved.PHOTO_01_003 must be registered"),
		FamilyPhotoSolvedTag.IsValid()))
	{
		return false;
	}
	Door->UnlockRequiresTags.AddTag(FamilyPhotoSolvedTag);
	TestFalse(TEXT("The door starts locked"), Door->IsUnlocked());

	for (int32 Index = 0; Index < 3; ++Index)
	{
		ABalhwajeomEvidenceActor* Evidence =
			Fixture.World->SpawnActorDeferred<ABalhwajeomEvidenceActor>(
				ABalhwajeomEvidenceActor::StaticClass(), FTransform::Identity);
		if (!TestNotNull(TEXT("Evidence actor spawns"), Evidence))
		{
			return false;
		}
		Evidence->ConfigureInvestigationObject(ObjectIDs[Index]);
		Evidence->FinishSpawning(FTransform::Identity);

		// UWorld::HasBegunPlay() stays false without a GameState, so an actor spawned
		// after the world started does not get BeginPlay on its own here. Registration
		// with the investigation subsystem happens there.
		if (!Evidence->HasActorBegunPlay())
		{
			Evidence->DispatchBeginPlay();
		}

		const FGuid InstanceID = Evidence->GetEvidenceInstanceID();
		if (!TestTrue(
			FString::Printf(TEXT("%s registered with the investigation subsystem"),
				*ObjectIDs[Index].ToString()),
			InstanceID.IsValid()))
		{
			return false;
		}

		// --- F: dust the photo ---------------------------------------------------------
		FText Ignored;
		TestTrue(
			FString::Printf(TEXT("%s accepts the dust interaction"), *ObjectIDs[Index].ToString()),
			Evidence->RequestInvestigationInteraction(Ignored));

		// The cleaned photo answers F with a nudge, not with the family conversation:
		// photographing is what unlocks it, and the door must not open without that.
		TestTrue(
			FString::Printf(TEXT("%s still answers F once cleaned"), *ObjectIDs[Index].ToString()),
			Evidence->RequestInvestigationInteraction(Ignored));
		const FGameplayTag EarlyHeardTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Evidence.StoryHeard.%s"), *ObjectIDs[Index].ToString())),
			false);
		TestFalse(
			FString::Printf(TEXT("%s does not count as heard before the photo"),
				*ObjectIDs[Index].ToString()),
			StoryState->HasStateTagExact(EarlyHeardTag));

		// --- Photograph it -------------------------------------------------------------
		FCapturedPhotoRecord Record;
		Record.PhotoID = PhotoIDs[Index];
		Record.ObjectID = ObjectIDs[Index];
		Record.EvidenceInstanceID = InstanceID;
		Record.CapturedStateID = Evidence->GetCurrentStateID();
		Record.ImageRelativePath = TEXT("TutorialDoorUnlockPathTest/Photo.png");
		TestTrue(
			FString::Printf(TEXT("%s photo registers"), *PhotoIDs[Index].ToString()),
			Investigation->RegisterCapturedPhoto(Record));

		// Capturing is what should move the object into its memory state. If this is the
		// step that silently does nothing, the conversation never becomes available.
		TestEqual(
			FString::Printf(TEXT("%s advances to its memory state after capture"),
				*ObjectIDs[Index].ToString()),
			Evidence->GetCurrentStateID(), MemoryStateIDs[Index]);

		// --- F: listen to the family conversation --------------------------------------
		TestTrue(
			FString::Printf(TEXT("%s plays its family conversation"),
				*ObjectIDs[Index].ToString()),
			Evidence->RequestInvestigationInteraction(Ignored));

		const FGameplayTag HeardTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Evidence.StoryHeard.%s"),
				*ObjectIDs[Index].ToString())),
			false);
		TestTrue(
			FString::Printf(TEXT("hearing %s records its story-heard tag"),
				*ObjectIDs[Index].ToString()),
			StoryState->HasStateTagExact(HeardTag));

		TestFalse(
			TEXT("Optional family conversations do not unlock the door"),
			Door->IsUnlocked());
	}

	FSentenceDefinition FamilyPhotoSentence;
	if (!TestTrue(
		TEXT("The family-photo sentence should exist"),
		Investigation->GetSentenceDefinition(TEXT("SENT_01_PHOTO_001"), FamilyPhotoSentence)))
	{
		return false;
	}

	FSentenceSubmission Submission;
	for (const FSentenceWordSlot& Slot : FamilyPhotoSentence.WordSlots)
	{
		TestTrue(
			FString::Printf(TEXT("The frame captures should acquire %s"),
				*Slot.CorrectWordID.ToString()),
			Investigation->HasAcquiredWord(Slot.CorrectWordID));
		Submission.SubmittedWords.Add({Slot.SlotIndex, Slot.CorrectWordID});
	}

	FText ResultText;
	bool bSolved = false;
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		bSolved = Investigation->ValidateSentence(
			TEXT("SENT_01_PHOTO_001"), Submission, ResultText);
	}

	TestTrue(TEXT("The three frame keywords complete the family photo"), bSolved);
	TestTrue(TEXT("Completing the family photo records its story tag"),
		StoryState->HasStateTagExact(FamilyPhotoSolvedTag));
	TestTrue(TEXT("Completing the family photo unlocks the door"), Door->IsUnlocked());
	TestTrue(TEXT("An unlocked door offers its interaction"), Door->CanInteract());
	TestTrue(TEXT("The first unlocked-door interaction is accepted"), Door->RequestInteraction());
	TestTrue(TEXT("The first interaction starts opening the door"), Door->IsOpening());

	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
