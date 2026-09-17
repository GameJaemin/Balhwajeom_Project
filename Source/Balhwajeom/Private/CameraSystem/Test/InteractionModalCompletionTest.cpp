#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "Engine/World.h"
#include "UI/BalhwajeomInteractionModalWidget.h"


struct FInteractionModalTestAccessor
{
	static void InstallModal(
		ABalhwajeomCameraPlayerController* Controller,
		UBalhwajeomInteractionModalWidget* Modal)
	{
		Controller->InteractionModalWidget = Modal;
	}

	static bool HasModal(const ABalhwajeomCameraPlayerController* Controller)
	{
		return Controller->InteractionModalWidget != nullptr;
	}
};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionModalCompletionBroadcastTest,
	"Balhwajeom.Camera.InteractionModal.CloseBroadcastsCompletion",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

bool FInteractionModalCompletionBroadcastTest::RunTest(const FString& Parameters)
{
	// No GameInstance is needed for this controller-local contract. Keeping the world
	// isolated also prevents unrelated project DataTable validation from polluting the test.
	UWorld* World = UWorld::CreateWorld(EWorldType::GamePreview, false);
	ABalhwajeomCameraPlayerController* Controller = World
		? World->SpawnActor<ABalhwajeomCameraPlayerController>()
		: nullptr;
	UBalhwajeomInteractionModalWidget* Modal = Controller
		? NewObject<UBalhwajeomInteractionModalWidget>(Controller)
		: nullptr;

	if (!TestNotNull(TEXT("Standalone world exists"), World) ||
		!TestNotNull(TEXT("Camera player controller exists"), Controller) ||
		!TestNotNull(TEXT("Interaction modal exists"), Modal))
	{
		if (World)
		{
			World->DestroyWorld(false);
		}
		return false;
	}

	int32 CompletionCount = 0;
	Controller->OnInteractionModalClosed.AddLambda([&CompletionCount]()
	{
		++CompletionCount;
	});
	FInteractionModalTestAccessor::InstallModal(Controller, Modal);

	Controller->CloseInteractionModal();

	TestEqual(
		TEXT("Closing a modal broadcasts one presentation completion"),
		CompletionCount,
		1);
	TestFalse(
		TEXT("The modal is released before completion listeners run"),
		FInteractionModalTestAccessor::HasModal(Controller));

	World->DestroyWorld(false);
	return true;
}

#endif
