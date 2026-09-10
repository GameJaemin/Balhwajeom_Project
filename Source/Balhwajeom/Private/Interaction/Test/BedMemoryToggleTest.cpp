#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
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

	static FTransform GetPlayerAnchorTransform(const ABedMemoryActor* Bed)
	{
		return Bed->PlayerAnchor->GetComponentTransform();
	}

	static void SetTurnProgress(ABedMemoryActor* Bed, float NormalizedProgress)
	{
		Bed->PlayerTurnStartedAtSeconds = Bed->GetWorld()->GetTimeSeconds() -
			Bed->PlayerRotationDuration * NormalizedProgress;
		Bed->UpdatePlayerTurn();
	}

	static TArray<FName> BuildShuffleCycle(ABedMemoryActor* Bed, int32 VoiceCount)
	{
		Bed->VoiceCandidates.Reset();
		Bed->ShuffleBag.Reset();
		Bed->LastPlayedPhotoID = NAME_None;
		for (int32 Index = 0; Index < VoiceCount; ++Index)
		{
			FBedMemoryVoiceCandidate Candidate;
			Candidate.PhotoID = FName(*FString::Printf(TEXT("Photo_%d"), Index));
			Bed->VoiceCandidates.Add(Candidate);
		}
		Bed->RefillShuffleBag();

		TArray<FName> PlaybackOrder;
		while (!Bed->ShuffleBag.IsEmpty())
		{
			PlaybackOrder.Add(
				Bed->VoiceCandidates[Bed->ShuffleBag.Pop(EAllowShrinking::No)].PhotoID);
		}
		return PlaybackOrder;
	}

	static FName GetFirstOfNextShuffleCycle(ABedMemoryActor* Bed, FName LastPlayed)
	{
		Bed->LastPlayedPhotoID = LastPlayed;
		Bed->RefillShuffleBag();
		return Bed->VoiceCandidates[Bed->ShuffleBag.Pop(EAllowShrinking::No)].PhotoID;
	}

	static void ResetVoiceTestData(ABedMemoryActor* Bed)
	{
		Bed->VoiceCandidates.Reset();
		Bed->ShuffleBag.Reset();
		Bed->LastPlayedPhotoID = NAME_None;
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
	Controller->SpawnPlayerCameraManager();
	TestTrue(TEXT("Test controller is local"), Controller->IsLocalController());
	const TArray<FName> ShuffleCycle =
		FBedMemoryTestAccessor::BuildShuffleCycle(Bed, 8);
	TSet<FName> UniqueVoices;
	for (const FName PhotoID : ShuffleCycle)
	{
		UniqueVoices.Add(PhotoID);
	}
	TestEqual(TEXT("An eight-voice shuffle cycle contains eight entries"), ShuffleCycle.Num(), 8);
	TestEqual(
		TEXT("Every voice appears exactly once in a shuffle cycle"),
		UniqueVoices.Num(),
		8);
	TestNotEqual(
		TEXT("A new shuffle cycle does not repeat the previous cycle's final voice first"),
		FBedMemoryTestAccessor::GetFirstOfNextShuffleCycle(Bed, ShuffleCycle.Last()),
		ShuffleCycle.Last());
	FBedMemoryTestAccessor::ResetVoiceTestData(Bed);
	if (!World->HasBegunPlay())
	{
		World->BeginPlay();
	}
	World->Tick(LEVELTICK_All, 0.1f);
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
	const FTransform AnchorTransform =
		FBedMemoryTestAccessor::GetPlayerAnchorTransform(Bed);
	const FRotator OriginalControlRotation(12.0f, 73.0f, 0.0f);
	Controller->SetControlRotation(OriginalControlRotation);
	FBedMemoryTestAccessor::PressToggleInput(Bed);
	TestEqual(
		TEXT("First F enters camera-blended player alignment"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::AligningPlayer);
	TestTrue(
		TEXT("Interaction places the character at PlayerAnchor immediately"),
		Character->GetActorLocation().Equals(AnchorTransform.GetLocation(), 0.1f));
	const float AnchorYaw = AnchorTransform.Rotator().Yaw;
	TestTrue(
		TEXT("Character initially faces the PlayerAnchor direction toward the bed"),
		FMath::IsNearlyZero(
			FMath::FindDeltaAngleDegrees(AnchorYaw, Character->GetActorRotation().Yaw),
			0.1f));
	TestTrue(
		TEXT("Controller yaw is fixed to PlayerAnchor regardless of interaction direction"),
		FMath::IsNearlyZero(
			FMath::FindDeltaAngleDegrees(
				AnchorYaw, Controller->GetControlRotation().Yaw),
			0.1f));
	FBedMemoryTestAccessor::SetTurnProgress(Bed, 0.5f);
	TestTrue(
		TEXT("Character rotates 90 degrees halfway through its configured rotation time"),
		FMath::IsNearlyEqual(
			FMath::Abs(FMath::FindDeltaAngleDegrees(
				AnchorYaw, Character->GetActorRotation().Yaw)),
			90.0f,
			1.0f));
	TestTrue(
		TEXT("Controller follows the character during the turn"),
		FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
			Character->GetActorRotation().Yaw,
			Controller->GetControlRotation().Yaw), 0.1f));
	FBedMemoryTestAccessor::SetTurnProgress(Bed, 1.0f);
	World->Tick(LEVELTICK_All, 1.0f);
	TestEqual(
		TEXT("Camera blend finishes on the bed actor"),
		Controller->GetViewTarget(),
		static_cast<AActor*>(Bed));
	TestEqual(
		TEXT("Without captured photos, sitting continues to silent listening"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Listening);
	TestTrue(
		TEXT("Character finishes facing 180 degrees away from the bed"),
		FMath::IsNearlyEqual(
			FMath::Abs(FMath::FindDeltaAngleDegrees(
				AnchorYaw, Character->GetActorRotation().Yaw)),
			180.0f,
			0.1f));
	TestEqual(
		TEXT("Animation and audio preparation have not started"),
		Bed->GetAvailableVoiceCount(),
		0);
	TestFalse(
		TEXT("Sit label is hidden while resting"),
		FBedMemoryTestAccessor::IsInteractionLabelVisible(Bed));
	FBedMemoryTestAccessor::PressToggleInput(Bed);
	TestEqual(
		TEXT("Second collider-owned F restores Idle"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Idle);
	TestTrue(
		TEXT("Leaving restores the original controller rotation"),
		Controller->GetControlRotation().Equals(OriginalControlRotation, 0.1f));

	GameInstance->Shutdown();
	return true;
}

#endif
