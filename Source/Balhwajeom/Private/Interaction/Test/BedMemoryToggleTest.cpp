#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Blueprint/UserWidget.h"
#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCameraCharacter.h"
#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/Image.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/BedMemoryActor.h"

struct FBedMemoryTestAccessor
{
	static ECollisionEnabled::Type GetBedMeshCollisionEnabled(const ABedMemoryActor* Bed)
	{
		return Bed->BedMesh->GetCollisionEnabled();
	}

	static ECollisionResponse GetBedMeshPawnResponse(const ABedMemoryActor* Bed)
	{
		return Bed->BedMesh->GetCollisionResponseToChannel(ECC_Pawn);
	}

	static ECollisionResponse GetInteractionPawnResponse(const ABedMemoryActor* Bed)
	{
		return Bed->InteractionCollision->GetCollisionResponseToChannel(ECC_Pawn);
	}

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

	static void AdvanceBedHUDCrossFade(
		ABalhwajeomCameraPlayerController* Controller,
		float DeltaSeconds)
	{
		Controller->UpdateBedMemoryHUD(DeltaSeconds);
	}

	static bool IsInteractionCameraIconCollapsed(const ABedMemoryActor* Bed)
	{
		if (!Bed->InteractionWidget)
		{
			return false;
		}
		Bed->InteractionWidget->InitWidget();
		UUserWidget* Widget = Bed->InteractionWidget->GetUserWidgetObject();
		const UImage* CameraIcon = Widget
			? Cast<UImage>(Widget->GetWidgetFromName(TEXT("UseCamera")))
			: nullptr;
		return CameraIcon && CameraIcon->GetVisibility() == ESlateVisibility::Collapsed;
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

	static void SetExitAnchorTransform(ABedMemoryActor* Bed, const FTransform& Transform)
	{
		Bed->ExitAnchor->SetWorldTransform(Transform);
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

	static void SetAnimations(
		ABedMemoryActor* Bed,
		UAnimSequenceBase* Sit,
		UAnimMontage* SeatedIdle)
	{
		Bed->SitAnimation = Sit;
		Bed->SeatedIdleMontage = SeatedIdle;
	}

	static void FinishSitAnimation(ABedMemoryActor* Bed)
	{
		Bed->GetWorldTimerManager().ClearTimer(Bed->TransitionTimer);
		Bed->FinishEntering();
	}

	static void FinishStandAnimation(ABedMemoryActor* Bed)
	{
		Bed->GetWorldTimerManager().ClearTimer(Bed->TransitionTimer);
		Bed->FinishExiting();
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
	ABalhwajeomCameraPlayerController* Controller =
		World->SpawnActor<ABalhwajeomCameraPlayerController>(
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

	TestEqual(
		TEXT("Bed mesh collision is enabled for solid furniture"),
		FBedMemoryTestAccessor::GetBedMeshCollisionEnabled(Bed),
		ECollisionEnabled::QueryAndPhysics);
	TestEqual(
		TEXT("Bed mesh blocks the player pawn"),
		FBedMemoryTestAccessor::GetBedMeshPawnResponse(Bed),
		ECR_Block);
	TestEqual(
		TEXT("Interaction collider remains an overlap trigger for the player pawn"),
		FBedMemoryTestAccessor::GetInteractionPawnResponse(Bed),
		ECR_Overlap);

	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Character);
	Controller->SpawnPlayerCameraManager();
	TestTrue(TEXT("Test controller is local"), Controller->IsLocalController());

	USkeletalMesh* DogMesh = LoadObject<USkeletalMesh>(
		nullptr, TEXT("/Game/Balhwajeom/Characters/Player/Dog_Human.Dog_Human"));
	UAnimSequenceBase* OriginalIdle = LoadObject<UAnimSequenceBase>(
		nullptr, TEXT("/Game/Balhwajeom/Characters/Player/Dog_Idle_Anim.Dog_Idle_Anim"));
	UAnimSequenceBase* SitAnimation = LoadObject<UAnimSequenceBase>(
		nullptr, TEXT("/Game/Balhwajeom/Characters/Player/Dogseat_Stand.Dogseat_Stand"));
	UAnimMontage* SeatedIdle = LoadObject<UAnimMontage>(
		nullptr, TEXT("/Game/Balhwajeom/Characters/Player/Dogseat_Idle_Montage.Dogseat_Idle_Montage"));
	if (!TestNotNull(TEXT("Dog mesh should load"), DogMesh) ||
		!TestNotNull(TEXT("Original idle animation should load"), OriginalIdle) ||
		!TestNotNull(TEXT("Sit animation should load"), SitAnimation) ||
		!TestNotNull(TEXT("Seated idle montage should load"), SeatedIdle))
	{
		GameInstance->Shutdown();
		return false;
	}
	Character->GetMesh()->SetSkeletalMeshAsset(DogMesh);
	Character->GetMesh()->PlayAnimation(OriginalIdle, true);
	FBedMemoryTestAccessor::SetAnimations(Bed, SitAnimation, SeatedIdle);
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
	TestNotNull(
		TEXT("Player controller owns one WB_HUD2 instance"),
		Controller->GetBedMemoryHUD());
	TestFalse(
		TEXT("WB_HUD2 starts inactive during exploration"),
		Controller->IsBedMemoryHUDActive());
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
	TestTrue(
		TEXT("Bed sit label hides the evidence camera icon"),
		FBedMemoryTestAccessor::IsInteractionCameraIconCollapsed(Bed));
	FBedMemoryTestAccessor::SimulateColliderEndOverlap(Bed, Character);
	TestFalse(
		TEXT("Sit label hides on collider EndOverlap"),
		FBedMemoryTestAccessor::IsInteractionLabelVisible(Bed));
	FBedMemoryTestAccessor::SimulateColliderBeginOverlap(Bed, Character);
	const FTransform AnchorTransform =
		FBedMemoryTestAccessor::GetPlayerAnchorTransform(Bed);
	const FVector ExitLocation(250.0f, -175.0f, 96.0f);
	const float ExitYaw = 137.0f;
	FBedMemoryTestAccessor::SetExitAnchorTransform(
		Bed, FTransform(FRotator(0.0f, ExitYaw, 0.0f), ExitLocation));
	const FRotator OriginalControlRotation(12.0f, 73.0f, 0.0f);
	Controller->SetControlRotation(OriginalControlRotation);
	FBedMemoryTestAccessor::PressToggleInput(Bed);
	TestTrue(
		TEXT("Entering the bed requests the WB_HUD to WB_HUD2 cross-fade"),
		Controller->IsBedMemoryHUDActive());
	FBedMemoryTestAccessor::AdvanceBedHUDCrossFade(Controller, 0.2f);
	const float EnteringBedHUDOpacity = Controller->GetBedMemoryHUD()
		? Controller->GetBedMemoryHUD()->GetRenderOpacity()
		: 0.0f;
	TestTrue(
		TEXT("WB_HUD2 fades in gradually instead of appearing instantly"),
		EnteringBedHUDOpacity > 0.0f && EnteringBedHUDOpacity < 1.0f);
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
	TestEqual(
		TEXT("Finishing the turn starts the sit animation"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Entering);
	UAnimSingleNodeInstance* SitInstance = Character->GetMesh()->GetSingleNodeInstance();
	TestNotNull(TEXT("Sit animation uses a single-node instance"), SitInstance);
	if (SitInstance)
	{
		TestEqual(
			TEXT("The configured sit animation is playing"),
			SitInstance->GetCurrentAsset(),
			static_cast<UAnimationAsset*>(SitAnimation));
	}
	FBedMemoryTestAccessor::FinishSitAnimation(Bed);
	TestEqual(
		TEXT("Camera blend finishes on the bed actor"),
		Controller->GetViewTarget(),
		static_cast<AActor*>(Bed));
	TestEqual(
		TEXT("After sitting, no captured photos produces silent listening"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Listening);
	UAnimSingleNodeInstance* SeatedInstance = Character->GetMesh()->GetSingleNodeInstance();
	TestNotNull(TEXT("Seated idle uses a single-node instance"), SeatedInstance);
	if (SeatedInstance)
	{
		TestEqual(
			TEXT("The configured seated idle animation is playing"),
			SeatedInstance->GetCurrentAsset(),
			static_cast<UAnimationAsset*>(SeatedIdle));
		TestTrue(TEXT("Seated idle loops"), SeatedInstance->IsLooping());
	}
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
		TEXT("Second F starts standing by reversing the sit animation"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Exiting);
	UAnimSingleNodeInstance* StandInstance = Character->GetMesh()->GetSingleNodeInstance();
	TestNotNull(TEXT("Stand animation uses a single-node instance"), StandInstance);
	if (StandInstance)
	{
		TestEqual(
			TEXT("Standing reuses the configured sit animation"),
			StandInstance->GetCurrentAsset(),
			static_cast<UAnimationAsset*>(SitAnimation));
		TestTrue(TEXT("Standing animation plays in reverse"), StandInstance->GetPlayRate() < 0.0f);
	}
	FBedMemoryTestAccessor::FinishStandAnimation(Bed);
	TestFalse(
		TEXT("Finishing the bed exit requests the WB_HUD2 to WB_HUD cross-fade"),
		Controller->IsBedMemoryHUDActive());
	FBedMemoryTestAccessor::AdvanceBedHUDCrossFade(Controller, 0.1f);
	TestTrue(
		TEXT("WB_HUD2 fades out gradually after the bed exit"),
		Controller->GetBedMemoryHUD() &&
		Controller->GetBedMemoryHUD()->GetRenderOpacity() < EnteringBedHUDOpacity);
	TestEqual(
		TEXT("Second collider-owned F restores Idle"),
		Bed->GetBedMemoryState(),
		EBedMemoryState::Idle);
	TestTrue(
		TEXT("Leaving places the character at ExitAnchor"),
		Character->GetActorLocation().Equals(ExitLocation, 0.1f));
	TestTrue(
		TEXT("Leaving applies ExitAnchor yaw to the character"),
		FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
			ExitYaw, Character->GetActorRotation().Yaw), 0.1f));
	TestTrue(
		TEXT("Leaving applies ExitAnchor yaw to the controller"),
		FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
			ExitYaw, Controller->GetControlRotation().Yaw), 0.1f));
	TestTrue(
		TEXT("Leaving preserves the original controller pitch"),
		FMath::IsNearlyEqual(
			Controller->GetControlRotation().Pitch,
			OriginalControlRotation.Pitch,
			0.1f));
	UAnimSingleNodeInstance* RestoredInstance = Character->GetMesh()->GetSingleNodeInstance();
	TestNotNull(TEXT("Original animation instance is restored"), RestoredInstance);
	if (RestoredInstance)
	{
		TestEqual(
			TEXT("Original locomotion animation is restored"),
			RestoredInstance->GetCurrentAsset(),
			static_cast<UAnimationAsset*>(OriginalIdle));
		TestTrue(TEXT("Original locomotion remains looping"), RestoredInstance->IsLooping());
	}

	GameInstance->Shutdown();
	return true;
}

#endif
