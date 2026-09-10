#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/BedMemoryActor.h"

struct FBedMemoryTestAccessor
{
	static void SimulateColliderBeginOverlap(ABedMemoryActor* Bed, APawn* Pawn)
	{
		const FHitResult SweepResult;
		Bed->HandleInteractionBeginOverlap(
			nullptr, Pawn, nullptr, INDEX_NONE, false, SweepResult);
	}

	static bool HasColliderOwnedInput(const ABedMemoryActor* Bed)
	{
		return Bed->RestInputComponent != nullptr;
	}

	static bool IsInteractionLabelVisible(const ABedMemoryActor* Bed)
	{
		return Bed->InteractionWidget && Bed->InteractionWidget->IsVisible();
	}

	static void SimulateColliderEndOverlap(ABedMemoryActor* Bed, APawn* Pawn)
	{
		Bed->HandleInteractionEndOverlap(
			nullptr, Pawn, nullptr, INDEX_NONE);
	}

	static void PressToggleInput(ABedMemoryActor* Bed)
	{
		Bed->EarliestExitTimeSeconds = 0.0;
		Bed->HandleToggleInput();
	}

	static FVector GetPlayerAnchorLocation(const ABedMemoryActor* Bed)
	{
		return Bed->PlayerAnchor->GetComponentLocation();
	}

	static void EvaluateApproach(ABedMemoryActor* Bed)
	{
		Bed->UpdateApproach();
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBedMemoryColliderToggleTest,
	"Balhwajeom.Interaction.BedMemory.ColliderToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBedMemoryColliderToggleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABedMemoryActor* Bed = World->SpawnActor<ABedMemoryActor>(
		FVector::ZeroVector, FRotator::ZeroRotator);
	ABalhwajeomCameraCharacter* Character =
		World->SpawnActor<ABalhwajeomCameraCharacter>(
			FVector(100.0f, 0.0f, 96.0f), FRotator::ZeroRotator);
	APlayerController* Controller =
		World->SpawnActor<APlayerController>(
			FVector::ZeroVector, FRotator::ZeroRotator);
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);

	if (!TestNotNull(TEXT("Bed should spawn"), Bed) ||
		!TestNotNull(TEXT("Character should spawn"), Character) ||
		!TestNotNull(TEXT("Controller should spawn"), Controller) ||
		!TestNotNull(TEXT("Local player should be created"), LocalPlayer))
	{
		GameInstance->Shutdown();
		return false;
	}

	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Character);
	TestTrue(TEXT("Test controller is local"), Controller->IsLocalController());
	if (!World->HasBegunPlay())
	{
		World->BeginPlay();
	}
	World->Tick(LEVELTICK_All, 0.1f);
	// The standalone test world has no floor. Force the normal runtime movement
	// mode so RequestDirectMove follows its grounded walking path.
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	TestFalse(
		TEXT("Sit label is hidden before collider overlap"),
		FBedMemoryTestAccessor::IsInteractionLabelVisible(Bed));
	FBedMemoryTestAccessor::SimulateColliderBeginOverlap(Bed, Character);

	TestTrue(
		TEXT("Collider BeginOverlap registers the character"),
		Bed->IsPawnWithinInteractionZone(Character));
	TestTrue(
		TEXT("Collider BeginOverlap installs the collider-owned F input"),
		FBedMemoryTestAccessor::HasColliderOwnedInput(Bed));
	TestTrue(
		TEXT("Sit label is visible while the character overlaps the collider"),
		FBedMemoryTestAccessor::IsInteractionLabelVisible(Bed));
	FBedMemoryTestAccessor::SimulateColliderEndOverlap(Bed, Character);
	TestFalse(
		TEXT("Sit label hides on collider EndOverlap"),
		FBedMemoryTestAccessor::IsInteractionLabelVisible(Bed));
	FBedMemoryTestAccessor::SimulateColliderBeginOverlap(Bed, Character);
	const FVector AnchorLocation =
		FBedMemoryTestAccessor::GetPlayerAnchorLocation(Bed);
	FBedMemoryTestAccessor::PressToggleInput(Bed);
	TestEqual(
		TEXT("First collider-owned F begins walking to PlayerAnchor"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Approaching);
	TestTrue(
		TEXT("Approach does not teleport a distant character immediately"),
		FVector::Dist2D(
			Character->GetActorLocation(),
			AnchorLocation) > 10.0f);
	TestTrue(
		TEXT("Automatic approach sends a direct movement request toward PlayerAnchor"),
		Character->GetCharacterMovement()->RequestedVelocity.Size2D() > 0.0f);
	Character->SetActorLocation(AnchorLocation);
	FBedMemoryTestAccessor::EvaluateApproach(Bed);
	TestEqual(
		TEXT("Reaching PlayerAnchor continues to Listening without animation or audio"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Listening);
	TestFalse(
		TEXT("Sit label is hidden while resting"),
		FBedMemoryTestAccessor::IsInteractionLabelVisible(Bed));
	FBedMemoryTestAccessor::PressToggleInput(Bed);
	TestEqual(
		TEXT("Second collider-owned F restores Idle"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Idle);

	GameInstance->Shutdown();
	return true;
}

#endif
