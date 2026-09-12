#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"
#include "Tablet/BalhwajeomTabletComponent.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


namespace PlayerRuntimeModeIntegrationTests
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
				Character = World->SpawnActor<ABalhwajeomCameraCharacter>();
			}
		}

		~FFixture()
		{
			if (!GameInstance)
			{
				return;
			}

			if (LocalPlayer)
			{
				LocalPlayer->PlayerRemoved();
			}
			GameInstance->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		bool BeginPlay()
		{
			if (!World || !Character)
			{
				return false;
			}

			if (!World->HasBegunPlay())
			{
				World->BeginPlay();
			}
			if (!Character->HasActorBegunPlay())
			{
				Character->DispatchBeginPlay();
			}
			return true;
		}

		bool AddLocalController()
		{
			if (!World || !Character)
			{
				return false;
			}

			Controller = World->SpawnActor<APlayerController>();
			LocalPlayer = NewObject<ULocalPlayer>(GEngine);
			if (!Controller || !LocalPlayer)
			{
				return false;
			}

			LocalPlayer->PlayerAdded(nullptr, 0);
			Controller->SetPlayer(LocalPlayer);
			Controller->Possess(Character);
			return Controller->IsLocalController();
		}

		UStoryStateSubsystem* GetStoryState() const
		{
			return GameInstance
				? GameInstance->GetSubsystem<UStoryStateSubsystem>()
				: nullptr;
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
				// FTimerManager only advances once per GFrameCounter. Manual automation
				// sub-ticks must emulate distinct engine frames (as in TimerManagerTests.cpp).
				++GFrameCounter;
				Elapsed += DeltaTime;
			}
		}

		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		ABalhwajeomCameraCharacter* Character = nullptr;
		APlayerController* Controller = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerRuntimeModeInitialExplorationTest,
	"Balhwajeom.StoryState.PlayerMode.Integration.InitialExploration",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerRuntimeModeInitialExplorationTest::RunTest(
	const FString& Parameters
)
{
	PlayerRuntimeModeIntegrationTests::FFixture Fixture;
	if (!TestTrue(TEXT("The character fixture should begin play"), Fixture.BeginPlay()))
	{
		return false;
	}

	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState))
	{
		return false;
	}

	TestTrue(
		TEXT("A newly started player should be in exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
		)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerRuntimeModePhotoCameraTransitionTest,
	"Balhwajeom.StoryState.PlayerMode.Integration.PhotoCameraTransition",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerRuntimeModePhotoCameraTransitionTest::RunTest(
	const FString& Parameters
)
{
	PlayerRuntimeModeIntegrationTests::FFixture Fixture;
	if (!TestTrue(TEXT("The character fixture should begin play"), Fixture.BeginPlay()))
	{
		return false;
	}

	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Fixture.Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Photo camera component should exist"), PhotoCamera))
	{
		return false;
	}

	PhotoCamera->ToggleCameraMode();
	TestTrue(
		TEXT("Requesting photo camera mode should start a transition"),
		PhotoCamera->IsCameraTransitioning()
	);
	Fixture.TickWorldFor(0.8f);
	TestTrue(
		TEXT("The camera switch timer should enter photo camera mode"),
		PhotoCamera->IsInCameraMode()
	);
	TestFalse(
		TEXT("The photo camera transition should finish"),
		PhotoCamera->IsCameraTransitioning()
	);

	TestTrue(
		TEXT("Entering the photo camera should select photo camera mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_PhotoCamera
		)
	);
	TestFalse(
		TEXT("Photo camera mode should replace exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
		)
	);

	PhotoCamera->RequestExitCameraMode();
	Fixture.TickWorldFor(0.8f);

	TestTrue(
		TEXT("Leaving the photo camera should restore exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
		)
	);
	TestFalse(
		TEXT("Exploration mode should replace photo camera mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_PhotoCamera
		)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerRuntimeModeTabletTransitionTest,
	"Balhwajeom.StoryState.PlayerMode.Integration.TabletTransition",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerRuntimeModeTabletTransitionTest::RunTest(
	const FString& Parameters
)
{
	AddExpectedError(
		TEXT("AddToPlayerScreen"),
		EAutomationExpectedErrorFlags::Contains,
		1
	);

	PlayerRuntimeModeIntegrationTests::FFixture Fixture;
	if (!TestTrue(TEXT("A local controller should be created"), Fixture.AddLocalController()) ||
		!TestTrue(TEXT("The character fixture should begin play"), Fixture.BeginPlay()))
	{
		return false;
	}

	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomTabletComponent* Tablet =
		Fixture.Character->FindComponentByClass<UBalhwajeomTabletComponent>();
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Tablet component should exist"), Tablet))
	{
		return false;
	}

	Tablet->RequestOpenTablet();
	if (!TestTrue(TEXT("The tablet should open"), Tablet->IsTabletOpen()))
	{
		return false;
	}

	TestTrue(
		TEXT("Opening the tablet should select tablet mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Tablet
		)
	);
	TestFalse(
		TEXT("Tablet mode should replace exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
		)
	);

	Tablet->DestroyComponent();

	TestTrue(
		TEXT("Closing the tablet during teardown should restore exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
		)
	);
	TestFalse(
		TEXT("Exploration mode should replace tablet mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Tablet
		)
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerRuntimeModePendingTabletTeardownTest,
	"Balhwajeom.StoryState.PlayerMode.Integration.PendingTabletTeardown",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerRuntimeModePendingTabletTeardownTest::RunTest(
	const FString& Parameters
)
{
	PlayerRuntimeModeIntegrationTests::FFixture Fixture;
	if (!TestTrue(TEXT("A local controller should be created"), Fixture.AddLocalController()) ||
		!TestTrue(TEXT("The character fixture should begin play"), Fixture.BeginPlay()))
	{
		return false;
	}

	UStoryStateSubsystem* StoryState = Fixture.GetStoryState();
	UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Fixture.Character->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	UBalhwajeomTabletComponent* Tablet =
		Fixture.Character->FindComponentByClass<UBalhwajeomTabletComponent>();
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Photo camera component should exist"), PhotoCamera) ||
		!TestNotNull(TEXT("Tablet component should exist"), Tablet))
	{
		return false;
	}

	PhotoCamera->ToggleCameraMode();
	Fixture.TickWorldFor(0.8f);
	if (!TestTrue(TEXT("The photo camera should be active"), PhotoCamera->IsInCameraMode()))
	{
		return false;
	}

	Tablet->RequestOpenTablet();
	TestFalse(
		TEXT("The tablet should wait for photo camera exit"),
		Tablet->IsTabletOpen()
	);

	Tablet->DestroyComponent();

	TestTrue(
		TEXT("Destroying a pending tablet should preserve the active photo camera mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_PhotoCamera
		)
	);
	TestFalse(
		TEXT("A tablet that never opened should not restore exploration mode"),
		StoryState->HasStateTagExact(
			BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
		)
	);

	return true;
}

#endif
