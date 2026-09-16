#if WITH_DEV_AUTOMATION_TESTS

#include "CameraSystem/BalhwajeomCameraPlayerController.h"

#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

struct FInteractionReticleTestAccessor
{
	static void Configure(
		ABalhwajeomCameraPlayerController* Controller,
		UImage* Reticle,
		UTexture2D* DotTexture,
		UTexture2D* MagnifierTexture)
	{
		Controller->InteractionReticleWidget = Reticle;
		Controller->InteractionReticleDotTexture = DotTexture;
		Controller->InteractionReticleMagnifierTexture = MagnifierTexture;
	}

	static void Refresh(
		ABalhwajeomCameraPlayerController* Controller,
		bool bHasValidInteractionTarget)
	{
		Controller->RefreshInteractionReticle(bHasValidInteractionTarget);
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionReticleSwitchesWithAvailabilityTest,
	"Balhwajeom.Camera.InteractionPrompt.ReticleSwitchesWithAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionReticleSwitchesWithAvailabilityTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABalhwajeomCameraPlayerController* Controller =
		World->SpawnActor<ABalhwajeomCameraPlayerController>();
	UImage* Reticle = NewObject<UImage>(Controller);
	UTexture2D* DotTexture = NewObject<UTexture2D>(Controller);
	UTexture2D* MagnifierTexture = NewObject<UTexture2D>(Controller);
	if (!TestNotNull(TEXT("Controller should spawn"), Controller) ||
		!TestNotNull(TEXT("Reticle image should be created"), Reticle))
	{
		GameInstance->Shutdown();
		return false;
	}

	FInteractionReticleTestAccessor::Configure(
		Controller, Reticle, DotTexture, MagnifierTexture);

	FInteractionReticleTestAccessor::Refresh(Controller, false);
	TestEqual(
		TEXT("Unavailable interaction uses the dot texture"),
		Reticle->GetBrush().GetResourceObject(),
		static_cast<UObject*>(DotTexture));

	FInteractionReticleTestAccessor::Refresh(Controller, true);
	TestEqual(
		TEXT("Available interaction uses the magnifier texture"),
		Reticle->GetBrush().GetResourceObject(),
		static_cast<UObject*>(MagnifierTexture));

	FInteractionReticleTestAccessor::Refresh(Controller, false);
	TestEqual(
		TEXT("Losing interaction availability restores the dot texture"),
		Reticle->GetBrush().GetResourceObject(),
		static_cast<UObject*>(DotTexture));

	GameInstance->Shutdown();
	return true;
}

#endif
