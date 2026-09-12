#include "PlayerInteractionModeTestTypes.h"

#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


void UPlayerInteractionModeTestObserver::HandleInspectionDismissRequested()
{
	++DismissRequestCount;
}


namespace PlayerInteractionModeTests
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

		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		ABalhwajeomCameraCharacter* Character = nullptr;
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerInteractionModalModeDismissTest,
	"Balhwajeom.Interaction.Player.ModalModeDismissesInspection",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FPlayerInteractionModalModeDismissTest::RunTest(
	const FString& Parameters
)
{
	PlayerInteractionModeTests::FFixture Fixture;
	if (!TestTrue(TEXT("The player fixture should begin play"), Fixture.BeginPlay()))
	{
		return false;
	}

	UStoryStateSubsystem* StoryState =
		Fixture.GameInstance->GetSubsystem<UStoryStateSubsystem>();
	UPlayerInteractionComponent* PlayerInteraction =
		Fixture.Character->FindComponentByClass<UPlayerInteractionComponent>();
	if (!TestNotNull(TEXT("Story state subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Player interaction component should exist"), PlayerInteraction))
	{
		return false;
	}

	UPlayerInteractionModeTestObserver* Observer =
		NewObject<UPlayerInteractionModeTestObserver>(Fixture.GameInstance);
	PlayerInteraction->OnInspectionDismissRequested.AddDynamic(
		Observer,
		&UPlayerInteractionModeTestObserver::HandleInspectionDismissRequested
	);

	UInspectionComponent* Inspection =
		NewObject<UInspectionComponent>(Fixture.Character);
	PlayerInteraction->SetFocusedInspection(Inspection);

	StoryState->SetPlayerModeTag(
		BalhwajeomGameplayTags::Runtime_Player_Mode_PhotoCamera
	);

	TestNull(
		TEXT("Photo camera mode should clear the focused inspection"),
		PlayerInteraction->GetFocusedInspection()
	);
	TestEqual(
		TEXT("Photo camera mode should request one inspection dismissal"),
		Observer->DismissRequestCount,
		1
	);

	StoryState->SetPlayerModeTag(
		BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
	);
	TestEqual(
		TEXT("Exploration mode should not request another dismissal"),
		Observer->DismissRequestCount,
		1
	);

	PlayerInteraction->SetFocusedInspection(Inspection);
	StoryState->SetPlayerModeTag(
		BalhwajeomGameplayTags::Runtime_Player_Mode_Tablet
	);

	TestNull(
		TEXT("Tablet mode should clear the focused inspection"),
		PlayerInteraction->GetFocusedInspection()
	);
	TestEqual(
		TEXT("Tablet mode should request one additional inspection dismissal"),
		Observer->DismissRequestCount,
		2
	);

	return true;
}

#endif
