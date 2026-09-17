#include "CameraSystem/BalhwajeomCameraPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"


#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGameplayCursorHiddenForMouseLookTest,
	"Balhwajeom.Camera.PlayerController.HidesCursorForMouseLook",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


/**
 * The controller must come up with the cursor hidden.
 *
 * This is not cosmetic. The viewport only keeps the mouse captured while the cursor is
 * hidden, and the Turn/LookUp axes are fed by that capture, so a cursor left visible
 * turns looking around into click-and-drag in both third person and camera mode. That
 * regressed once already when this line was removed, which is why it is pinned here.
 *
 * The two other places that hide it (this class's BeginPlay, and the intro actor handing
 * control to the player) are only reachable through UGameplayStatics::GetPlayerController,
 * which finds nothing in a standalone test world with no local player. Those carry
 * comments saying the same thing instead.
 */
bool FGameplayCursorHiddenForMouseLookTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	const ABalhwajeomCameraPlayerController* PlayerController =
		World->SpawnActor<ABalhwajeomCameraPlayerController>();
	if (TestNotNull(TEXT("The player controller should spawn"), PlayerController))
	{
		TestFalse(
			TEXT("The controller hides the cursor so the viewport can capture the mouse"),
			PlayerController->bShowMouseCursor);
	}

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
