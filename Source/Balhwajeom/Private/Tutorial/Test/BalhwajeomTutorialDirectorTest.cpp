#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"
#include "Tablet/BalhwajeomTabletComponent.h"
#include "Tutorial/BalhwajeomTutorialDirector.h"
#include "Tutorial/BalhwajeomTutorialFlow.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


/** Configures a deferred-spawned director before BeginPlay, so no flow auto-starts. */
struct FBalhwajeomTutorialDirectorTestAccessor
{
	static void Configure(
		ABalhwajeomTutorialDirector* Director,
		UBalhwajeomTutorialFlow* Flow)
	{
		Director->Flow = Flow;
		Director->bAutoStartOnBeginPlay = false;
	}
};


namespace TutorialDirectorTests
{
	static FGameplayTag StageRoot()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Test.Tutorial.Stage")));
	}

	static FGameplayTag StageOne()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Test.Tutorial.Stage.One")));
	}

	static FGameplayTag StageTwo()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Test.Tutorial.Stage.Two")));
	}

	static FGameplayTag TriggerOne()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Test.Tutorial.Trigger.One")));
	}

	static FGameplayTag TriggerTwo()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Test.Tutorial.Trigger.Two")));
	}

	static FGameplayTagQuery QueryFor(FGameplayTag Tag)
	{
		FGameplayTagContainer Container;
		Container.AddTag(Tag);
		return FGameplayTagQuery::MakeQuery_MatchAllTags(Container);
	}

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

		UStoryStateSubsystem* GetStoryState() const
		{
			return GameInstance ? GameInstance->GetSubsystem<UStoryStateSubsystem>() : nullptr;
		}

		/**
		 * AActor::ProcessEvent silently drops every UFUNCTION call, dynamic delegates
		 * included, until UWorld::AreActorsInitialized() is true. A fixture that skips
		 * InitializeActorsForPlay would see the director's subscription do nothing.
		 *
		 * UWorld::HasBegunPlay() stays false without a GameState, so the repeat guard
		 * is tracked here instead; calling UWorld::BeginPlay() twice trips an ensure
		 * inside the world subsystems.
		 */
		void EnsureWorldBegunPlay()
		{
			if (!World || bWorldBegunPlay)
			{
				return;
			}

			bWorldBegunPlay = true;
			const FURL URL;
			World->InitializeActorsForPlay(URL);
			World->BeginPlay();
		}

		ABalhwajeomCameraCharacter* SpawnCharacter()
		{
			if (!World)
			{
				return nullptr;
			}

			Character = World->SpawnActor<ABalhwajeomCameraCharacter>();
			EnsureWorldBegunPlay();
			if (Character && !Character->HasActorBegunPlay())
			{
				Character->DispatchBeginPlay();
			}
			return Character;
		}

		/** A director that does not auto-start, so a test controls exactly when the flow runs. */
		ABalhwajeomTutorialDirector* SpawnDirector(UBalhwajeomTutorialFlow* Flow)
		{
			if (!World)
			{
				return nullptr;
			}

			// The director reacts through dynamic delegates, which reach an actor only
			// once the world's actors are initialized.
			EnsureWorldBegunPlay();

			// Deferred so the flow and the auto-start switch are set before BeginPlay.
			ABalhwajeomTutorialDirector* Director =
				World->SpawnActorDeferred<ABalhwajeomTutorialDirector>(
					ABalhwajeomTutorialDirector::StaticClass(),
					FTransform::Identity);
			if (!Director)
			{
				return nullptr;
			}

			FBalhwajeomTutorialDirectorTestAccessor::Configure(Director, Flow);
			Director->FinishSpawning(FTransform::Identity);
			return Director;
		}

		void TickWorldFor(float Duration, float Step = 0.1f) const
		{
			if (!World || Duration <= 0.0f || Step <= 0.0f)
			{
				return;
			}

			float Elapsed = 0.0f;
			while (Elapsed < Duration)
			{
				const float DeltaTime = FMath::Min(Step, Duration - Elapsed);
				World->Tick(ELevelTick::LEVELTICK_All, DeltaTime);
				// FTimerManager advances once per GFrameCounter, so manual sub-ticks
				// must emulate distinct engine frames.
				++GFrameCounter;
				Elapsed += DeltaTime;
			}
		}

		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		ABalhwajeomCameraCharacter* Character = nullptr;
		bool bWorldBegunPlay = false;
	};

	/**
	 * Two steps: the first unlocks the photo camera and ends on TriggerOne,
	 * the second ends on TriggerTwo. Both locks are applied at start.
	 */
	static UBalhwajeomTutorialFlow* MakeTwoStepFlow()
	{
		UBalhwajeomTutorialFlow* Flow = NewObject<UBalhwajeomTutorialFlow>();
		Flow->StageRootTag = StageRoot();
		Flow->LocksOnStart.AddTag(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera);
		Flow->LocksOnStart.AddTag(BalhwajeomGameplayTags::Runtime_Lock_Tablet);

		FBalhwajeomTutorialStep First;
		First.StepID = TEXT("First");
		First.StageTag = StageOne();
		First.RemoveOnEnter.AddTag(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera);
		First.CompleteWhen = QueryFor(TriggerOne());
		Flow->Steps.Add(First);

		FBalhwajeomTutorialStep Second;
		Second.StepID = TEXT("Second");
		Second.StageTag = StageTwo();
		Second.CompleteWhen = QueryFor(TriggerTwo());
		Flow->Steps.Add(Second);

		return Flow;
	}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialPhotoCameraUnlockedByDefaultTest,
	"Balhwajeom.Tutorial.Lock.PhotoCameraUnlockedByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialPhotoCameraUnlockedByDefaultTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	ABalhwajeomCameraCharacter* Character = Fixture.SpawnCharacter();
	if (!TestNotNull(TEXT("Character should spawn"), Character))
	{
		return false;
	}

	UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	if (!TestNotNull(TEXT("Photo camera component should exist"), PhotoCamera))
	{
		return false;
	}

	// This is the property every existing level depends on: no lock tag is present,
	// so no level needs configuration for these abilities to keep working.
	TestFalse(
		TEXT("The photo camera is unlocked while no lock tag is active"),
		PhotoCamera->IsLockedByStoryState());

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialPhotoCameraBlockedByLockTagTest,
	"Balhwajeom.Tutorial.Lock.PhotoCameraBlockedByLockTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialPhotoCameraBlockedByLockTagTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	ABalhwajeomCameraCharacter* Character = Fixture.SpawnCharacter();
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	if (!TestNotNull(TEXT("Character should spawn"), Character) ||
		!TestNotNull(TEXT("Story state subsystem should exist"), StoryState))
	{
		return false;
	}

	UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	if (!TestNotNull(TEXT("Photo camera component should exist"), PhotoCamera))
	{
		return false;
	}

	StoryState->AddStateTag(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera);
	TestTrue(
		TEXT("The lock tag locks the photo camera"),
		PhotoCamera->IsLockedByStoryState());

	PhotoCamera->ToggleCameraMode();
	Fixture.TickWorldFor(0.8f);
	TestFalse(
		TEXT("A locked photo camera does not start a transition"),
		PhotoCamera->IsCameraTransitioning());
	TestFalse(
		TEXT("A locked photo camera never enters camera mode"),
		PhotoCamera->IsInCameraMode());
	TestTrue(
		TEXT("A blocked toggle leaves the player in exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration));

	StoryState->RemoveStateTag(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera);
	PhotoCamera->ToggleCameraMode();
	Fixture.TickWorldFor(0.8f);
	TestTrue(
		TEXT("Removing the lock tag restores camera mode"),
		PhotoCamera->IsInCameraMode());

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialPhotoCameraExitIsNeverBlockedTest,
	"Balhwajeom.Tutorial.Lock.PhotoCameraExitIsNeverBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialPhotoCameraExitIsNeverBlockedTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	ABalhwajeomCameraCharacter* Character = Fixture.SpawnCharacter();
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	if (!TestNotNull(TEXT("Character should spawn"), Character) ||
		!TestNotNull(TEXT("Story state subsystem should exist"), StoryState))
	{
		return false;
	}

	UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	if (!TestNotNull(TEXT("Photo camera component should exist"), PhotoCamera))
	{
		return false;
	}

	PhotoCamera->ToggleCameraMode();
	Fixture.TickWorldFor(0.8f);
	if (!TestTrue(TEXT("Camera mode should be entered first"), PhotoCamera->IsInCameraMode()))
	{
		return false;
	}

	// Locking while the player is already inside camera mode must not trap them there.
	StoryState->AddStateTag(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera);
	PhotoCamera->ToggleCameraMode();
	Fixture.TickWorldFor(0.8f);

	TestFalse(
		TEXT("A lock applied inside camera mode still allows leaving it"),
		PhotoCamera->IsInCameraMode());
	TestTrue(
		TEXT("Leaving restores exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialTabletIgnoresLockTagTest,
	"Balhwajeom.Tutorial.Lock.TabletIgnoresLockTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialTabletIgnoresLockTagTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	ABalhwajeomCameraCharacter* Character = Fixture.SpawnCharacter();
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	if (!TestNotNull(TEXT("Character should spawn"), Character) ||
		!TestNotNull(TEXT("Story state subsystem should exist"), StoryState))
	{
		return false;
	}

	UBalhwajeomTabletComponent* Tablet =
		Character->FindComponentByClass<UBalhwajeomTabletComponent>();
	if (!TestNotNull(TEXT("Tablet component should exist"), Tablet))
	{
		return false;
	}

	// The tablet is deliberately not progression-gated: Tab has to work at any point in
	// the tutorial, so a flow's Runtime.Lock.Tablet tag no longer refuses opening.
	StoryState->AddStateTag(BalhwajeomGameplayTags::Runtime_Lock_Tablet);

	Tablet->SetTabletInteractionEnabled(false);
	Tablet->ToggleTablet();
	TestFalse(
		TEXT("Cinematic suppression still refuses the toggle"),
		Tablet->IsTabletOpen());

	// SetTabletInteractionEnabled stays the one switch that suppresses the tablet.
	Tablet->SetTabletInteractionEnabled(true);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialDoorUnlockQueryTest,
	"Balhwajeom.Tutorial.Lock.DoorUnlockQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialDoorUnlockQueryTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("World should exist"), Fixture.World))
	{
		return false;
	}

	AActor* DoorActor = Fixture.World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Door actor should spawn"), DoorActor))
	{
		return false;
	}

	UDoorInteractionComponent* Door =
		NewObject<UDoorInteractionComponent>(DoorActor);
	DoorActor->AddInstanceComponent(Door);
	Door->RegisterComponent();

	// An unconfigured door stays openable, so existing levels are unaffected.
	TestTrue(TEXT("A door with an empty query is unlocked"), Door->IsUnlocked());
	TestTrue(TEXT("An unlocked door can be interacted with"), Door->CanInteract());

	Door->UnlockQuery = TutorialDirectorTests::QueryFor(TutorialDirectorTests::TriggerOne());
	TestFalse(TEXT("An unsatisfied query locks the door"), Door->IsUnlocked());
	TestTrue(
		TEXT("A closed locked door remains interactable so [F] can explain the gate"),
		Door->CanInteract());
	TestTrue(TEXT("A locked interaction attempt is handled"), Door->RequestInteraction());
	TestFalse(TEXT("A locked interaction attempt does not open the door"), Door->IsOpen());

	StoryState->AddStateTag(TutorialDirectorTests::TriggerOne());
	TestTrue(TEXT("A satisfied query unlocks the door"), Door->IsUnlocked());
	TestTrue(TEXT("An unlocked door can be interacted with"), Door->CanInteract());

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialDirectorAppliesLocksAndAdvancesTest,
	"Balhwajeom.Tutorial.Director.AppliesLocksAndAdvances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialDirectorAppliesLocksAndAdvancesTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomTutorialFlow* Flow = TutorialDirectorTests::MakeTwoStepFlow();
	ABalhwajeomTutorialDirector* Director = Fixture.SpawnDirector(Flow);
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Director should spawn"), Director))
	{
		return false;
	}

	Director->StartFlow();

	TestEqual(TEXT("The flow starts on its first step"),
		Director->GetCurrentStepID(), FName(TEXT("First")));
	TestTrue(TEXT("The first step's stage tag is set"),
		StoryState->HasStateTagExact(TutorialDirectorTests::StageOne()));

	// The first step's RemoveOnEnter releases the camera lock, so only the tablet stays locked.
	TestFalse(TEXT("The first step releases the photo camera lock"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera));
	TestTrue(TEXT("The tablet lock is still applied"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));

	StoryState->AddStateTag(TutorialDirectorTests::TriggerOne());
	TestEqual(TEXT("Satisfying the condition advances to the second step"),
		Director->GetCurrentStepID(), FName(TEXT("Second")));
	TestTrue(TEXT("The second step's stage tag replaces the first"),
		StoryState->HasStateTagExact(TutorialDirectorTests::StageTwo()));
	TestFalse(TEXT("Stage tags are exclusive"),
		StoryState->HasStateTagExact(TutorialDirectorTests::StageOne()));

	StoryState->AddStateTag(TutorialDirectorTests::TriggerTwo());
	TestFalse(TEXT("Completing the last step ends the flow"), Director->IsFlowActive());
	TestFalse(TEXT("Finishing releases the remaining tablet lock"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialDirectorFastForwardsSatisfiedStepsTest,
	"Balhwajeom.Tutorial.Director.FastForwardsSatisfiedSteps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialDirectorFastForwardsSatisfiedStepsTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomTutorialFlow* Flow = TutorialDirectorTests::MakeTwoStepFlow();
	ABalhwajeomTutorialDirector* Director = Fixture.SpawnDirector(Flow);
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Director should spawn"), Director))
	{
		return false;
	}

	// Progress that already happened before the flow starts, which is what a reload,
	// a debug restart, or entering the level out of order looks like.
	StoryState->AddStateTag(TutorialDirectorTests::TriggerOne());

	Director->StartFlow();

	TestEqual(TEXT("An already-satisfied first step is skipped on start"),
		Director->GetCurrentStepID(), FName(TEXT("Second")));
	TestFalse(TEXT("The skipped step still applied its unlock"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialDirectorAbortReleasesLocksTest,
	"Balhwajeom.Tutorial.Director.AbortReleasesLocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialDirectorAbortReleasesLocksTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomTutorialFlow* Flow = TutorialDirectorTests::MakeTwoStepFlow();

	// Even a flow that opted out of automatic release must not strand the player.
	Flow->bReleaseLocksOnFinish = false;

	ABalhwajeomTutorialDirector* Director = Fixture.SpawnDirector(Flow);
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Director should spawn"), Director))
	{
		return false;
	}

	Director->StartFlow();
	TestTrue(TEXT("The tablet lock is applied while the flow runs"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));

	Director->AbortFlow();

	TestFalse(TEXT("Aborting ends the flow"), Director->IsFlowActive());
	TestFalse(TEXT("Aborting releases the photo camera lock"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_PhotoCamera));
	TestFalse(TEXT("Aborting releases the tablet lock"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));

	// A finished flow must stop reacting to tags, or a later story tag would move it again.
	StoryState->AddStateTag(TutorialDirectorTests::TriggerOne());
	TestFalse(TEXT("An aborted flow does not resume"), Director->IsFlowActive());

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialDirectorSetLockActiveTest,
	"Balhwajeom.Tutorial.Director.SetLockActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialDirectorSetLockActiveTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomTutorialFlow* Flow = TutorialDirectorTests::MakeTwoStepFlow();
	ABalhwajeomTutorialDirector* Director = Fixture.SpawnDirector(Flow);
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Director should spawn"), Director))
	{
		return false;
	}

	Director->StartFlow();

	// Releasing an ability early, without editing the flow, is a supported authoring move.
	Director->SetLockActive(BalhwajeomGameplayTags::Runtime_Lock_Tablet, false);
	TestFalse(TEXT("SetLockActive(false) clears the lock mid-flow"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));

	Director->SetLockActive(BalhwajeomGameplayTags::Runtime_Lock_Tablet, true);
	TestTrue(TEXT("SetLockActive(true) reapplies the lock"),
		StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Lock_Tablet));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialNoDirectorIsSafeTest,
	"Balhwajeom.Tutorial.Director.NoDirectorIsSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialNoDirectorIsSafeTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	ABalhwajeomCameraCharacter* Character = Fixture.SpawnCharacter();
	if (!TestNotNull(TEXT("Character should spawn"), Character))
	{
		return false;
	}

	// Every level without a director goes down this path, including the whole main game.
	TestNull(TEXT("No director is found in a level without one"),
		ABalhwajeomTutorialDirector::GetTutorialDirector(Character));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialEvidenceStateTagTest,
	"Balhwajeom.Tutorial.EvidenceStateTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialEvidenceStateTagTest::RunTest(const FString& Parameters)
{
	TutorialDirectorTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomInvestigationSubsystem* Investigation =
		Fixture.GameInstance
			? Fixture.GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
			: nullptr;
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Investigation subsystem should exist"), Investigation))
	{
		return false;
	}

	const FGameplayTag ExpectedTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Evidence.State.STATE_TEST_TUTORIAL")));

	TestFalse(TEXT("The state tag is absent before any transition"),
		StoryState->HasStateTagExact(ExpectedTag));

	// A ChangeState interaction reaches the story state through this broadcast, which is
	// what turns "the dust is off the photo" into a condition other systems can query.
	Investigation->OnEvidenceStateChanged.Broadcast(
		FGuid::NewGuid(),
		FName(TEXT("STATE_TEST_PREVIOUS")),
		FName(TEXT("STATE_TEST_TUTORIAL")));

	TestTrue(TEXT("Entering a state adds Evidence.State.<StateID>"),
		StoryState->HasStateTagExact(ExpectedTag));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
