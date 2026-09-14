#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "CameraSystem/BalhwajeomEvidenceActor.h"
#include "Components/BoxComponent.h"
#include "Components/Image.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "ItemInspection/JMInspectableComponent.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "Kismet/GameplayStatics.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"
#include "UObject/Script.h"


struct FEvidenceActorTestAccessor
{
	static void Enable3DInspection(ABalhwajeomEvidenceActor* Evidence)
	{
		Evidence->bEnable3DInspection = true;
	}
	static void SetEvidenceInstanceID(
		ABalhwajeomEvidenceActor* Evidence,
		const FGuid InstanceID)
	{
		Evidence->EvidenceInstanceID = InstanceID;
	}

	static void SetDistanceState(
		ABalhwajeomEvidenceActor* Evidence,
		const EPlayerInspectionDistanceState DistanceState)
	{
		Evidence->HandlePlayerDistanceStateChanged(DistanceState);
	}

	static void SetCanBeCaptured(
		ABalhwajeomEvidenceActor* Evidence,
		const bool bCanBeCaptured)
	{
		Evidence->bCanBeCaptured = bCanBeCaptured;
	}

	static UObject* GetStatusIconResource(ABalhwajeomEvidenceActor* Evidence)
	{
		if (!Evidence->ObjectLabelWidget)
		{
			return nullptr;
		}

		Evidence->ObjectLabelWidget->InitWidget();
		UUserWidget* LabelWidget = Evidence->ObjectLabelWidget->GetUserWidgetObject();
		const UImage* StatusImage = LabelWidget
			? Cast<UImage>(LabelWidget->GetWidgetFromName(TEXT("UseCamera")))
			: nullptr;
		return StatusImage ? StatusImage->GetBrush().GetResourceObject() : nullptr;
	}

	static UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem(
		const ABalhwajeomEvidenceActor* Evidence)
	{
		return Evidence->GetInvestigationSubsystem();
	}

	static FGuid GetEvidenceInstanceID(const ABalhwajeomEvidenceActor* Evidence)
	{
		return Evidence->EvidenceInstanceID;
	}

	static bool IsPhotoCaptureHandlerBound(
		const UBalhwajeomInvestigationSubsystem* Investigation,
		ABalhwajeomEvidenceActor* Evidence)
	{
		return Investigation->OnPhotoCaptured.IsAlreadyBound(
			Evidence,
			&ABalhwajeomEvidenceActor::HandlePhotoCaptured);
	}

	static void SetRequiredActivationTag(
		ABalhwajeomEvidenceActor* Evidence,
		FGameplayTag RequiredTag)
	{
		Evidence->RequiredActivationTag = RequiredTag;
	}

	static void RefreshProgressionAvailability(ABalhwajeomEvidenceActor* Evidence)
	{
		Evidence->RefreshProgressionAvailability();
	}

	static bool IsProgressionAvailable(const ABalhwajeomEvidenceActor* Evidence)
	{
		return Evidence->bProgressionAvailable;
	}

	static ECollisionEnabled::Type GetCameraBoundsCollision(
		const ABalhwajeomEvidenceActor* Evidence)
	{
		return Evidence->CameraTargetBounds
			? Evidence->CameraTargetBounds->GetCollisionEnabled()
			: ECollisionEnabled::NoCollision;
	}

	static bool IsStoryStateHandlerBound(
		const UStoryStateSubsystem* StoryState,
		ABalhwajeomEvidenceActor* Evidence)
	{
		return StoryState->OnStateTagAdded.IsAlreadyBound(
			Evidence,
			&ABalhwajeomEvidenceActor::HandleStoryStateTagChanged);
	}

};


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceNonCapturableUsesDotIconTest,
	"Balhwajeom.Camera.Evidence.ObjectLabel.NonCapturableUsesDotIcon",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceNonCapturableUsesDotIconTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABalhwajeomEvidenceActor* Evidence = World->SpawnActor<ABalhwajeomEvidenceActor>();
	if (!TestNotNull(TEXT("Evidence actor should spawn"), Evidence))
	{
		GameInstance->Shutdown();
		return false;
	}

	FEvidenceActorTestAccessor::SetCanBeCaptured(Evidence, false);
	FEvidenceActorTestAccessor::SetDistanceState(
		Evidence,
		EPlayerInspectionDistanceState::Far);

	UTexture2D* DotIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/DotIcon.DotIcon"));
	TestNotNull(TEXT("Non-capturable icon should be imported"), DotIcon);
	TestEqual(
		TEXT("Non-capturable evidence should use the dot icon"),
		FEvidenceActorTestAccessor::GetStatusIconResource(Evidence),
		static_cast<UObject*>(DotIcon));

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceInspectionFarLabelTest,
	"Balhwajeom.Camera.Evidence.InspectionLabel.FarUsesIconOnly",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceInspectionFarLabelTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Far evidence should remain visible for its status icon"),
		ABalhwajeomEvidenceActor::ShouldDisplayInspectionLabel(
			EPlayerInspectionDistanceState::Far,
			FText::GetEmpty())
	);

	TestTrue(
		TEXT("Uncollected Far evidence should leave text empty because the icon shows status"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Far,
			FText::GetEmpty()
		).IsEmptyOrWhitespace()
	);

	TestTrue(
		TEXT("Collected Far evidence should leave text empty because the icon shows status"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Far,
			FText::GetEmpty()
		).IsEmptyOrWhitespace()
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceInspectionNonFarLabelTest,
	"Balhwajeom.Camera.Evidence.InspectionLabel.EmptyNonFarHidden",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceInspectionNonFarLabelTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("Middle evidence with no authored label should remain hidden"),
		ABalhwajeomEvidenceActor::ShouldDisplayInspectionLabel(
			EPlayerInspectionDistanceState::Middle,
			FText::GetEmpty())
	);

	TestTrue(
		TEXT("Middle evidence with an authored label should be visible"),
		ABalhwajeomEvidenceActor::ShouldDisplayInspectionLabel(
			EPlayerInspectionDistanceState::Middle,
			FText::FromString(TEXT("사진")))
	);

	TestFalse(
		TEXT("Out-of-range evidence should be hidden"),
		ABalhwajeomEvidenceActor::ShouldDisplayInspectionLabel(
			EPlayerInspectionDistanceState::OutOfRange,
			FText::FromString(TEXT("사진")))
	);

	TestTrue(
		TEXT("Middle evidence with no authored label should remain hidden"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Middle,
			FText::GetEmpty()
		).IsEmptyOrWhitespace()
	);

	TestTrue(
		TEXT("Close evidence with no authored label should remain hidden"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Close,
			FText::GetEmpty()
		).IsEmptyOrWhitespace()
	);

	TestTrue(
		TEXT("Whitespace-only Middle labels should remain hidden"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Middle,
			FText::FromString(TEXT("   "))
		).IsEmptyOrWhitespace()
	);

	TestEqual(
		TEXT("Middle evidence with an authored label should show only the label"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Middle,
			FText::FromString(TEXT("사진"))
		).ToString(),
		FString(TEXT("사진"))
	);

	TestEqual(
		TEXT("Collected Close evidence should show only the label"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::Close,
			FText::FromString(TEXT("사진"))
		).ToString(),
		FString(TEXT("사진"))
	);

	TestTrue(
		TEXT("OutOfRange should never produce inspection label content"),
		ABalhwajeomEvidenceActor::FormatInspectionLabel(
			EPlayerInspectionDistanceState::OutOfRange,
			FText::FromString(TEXT("사진"))
		).IsEmptyOrWhitespace()
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidencePhotoCaptureRefreshesIconTest,
	"Balhwajeom.Camera.Evidence.ObjectLabel.PhotoCaptureRefreshesIcon",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidencePhotoCaptureRefreshesIconTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABalhwajeomEvidenceActor* Evidence = World->SpawnActorDeferred<ABalhwajeomEvidenceActor>(
		ABalhwajeomEvidenceActor::StaticClass(),
		FTransform::Identity);
	if (!TestNotNull(TEXT("Evidence actor should spawn"), Evidence))
	{
		GameInstance->Shutdown();
		return false;
	}

	const FGuid EvidenceInstanceID = FGuid::NewGuid();
	FEvidenceActorTestAccessor::SetEvidenceInstanceID(Evidence, EvidenceInstanceID);
	FEvidenceActorTestAccessor::Enable3DInspection(Evidence);
	Evidence->ConfigureInvestigationObject(TEXT("OBJ_01_001"));
	UGameplayStatics::FinishSpawningActor(Evidence, FTransform::Identity);
	if (!World->HasBegunPlay())
	{
		World->BeginPlay();
	}
	if (!Evidence->HasActorBegunPlay())
	{
		Evidence->DispatchBeginPlay();
	}
	// This test exercises the camera-to-check transition independently of the
	// configured object's current data-authored capture availability.
	FEvidenceActorTestAccessor::SetCanBeCaptured(Evidence, true);
	UJMInspectableComponent* ItemInspection = Evidence->GetItemInspectionComponent();
	TestNotNull(TEXT("Evidence should own the 3D inspection adapter"), ItemInspection);
	if (ItemInspection)
	{
		TestTrue(TEXT("Enabled Evidence should expose 3D inspection"), ItemInspection->bInspectionEnabled);
		TestNotNull(TEXT("Evidence should build runtime inspection data"), ItemInspection->InspectionData.Get());
		if (ItemInspection->InspectionData)
		{
			const UStaticMeshComponent* EvidenceMesh = Evidence->FindComponentByClass<UStaticMeshComponent>();
			TestNotNull(TEXT("Evidence mesh should exist"), EvidenceMesh);
			TestTrue(TEXT("Evidence mesh should become the fallback preview mesh"),
				ItemInspection->InspectionData->PreviewMesh.Get() == (EvidenceMesh ? EvidenceMesh->GetStaticMesh().Get() : nullptr));
		}
	}

	FEvidenceActorTestAccessor::SetDistanceState(
		Evidence,
		EPlayerInspectionDistanceState::Far);

	UTexture2D* RequiredIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoRequired.T_EvidencePhotoRequired"));
	UTexture2D* CapturedIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoCaptured.T_EvidencePhotoCaptured"));
	TestEqual(
		TEXT("Uncaptured evidence should use the camera icon"),
		FEvidenceActorTestAccessor::GetStatusIconResource(Evidence),
		static_cast<UObject*>(RequiredIcon));

	UBalhwajeomInvestigationSubsystem* Investigation =
		GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>();
	TestNotNull(TEXT("Investigation subsystem should exist"), Investigation);
	if (Investigation)
	{
		TestTrue(TEXT("Evidence actor should still be valid"), IsValid(Evidence));
		TestFalse(TEXT("Evidence actor should not be destroying"), Evidence->IsActorBeingDestroyed());
		TestEqual(
			TEXT("Evidence actor should retain the configured instance ID"),
			FEvidenceActorTestAccessor::GetEvidenceInstanceID(Evidence),
			EvidenceInstanceID);
		TestEqual(
			TEXT("Evidence actor should resolve the same investigation subsystem"),
			FEvidenceActorTestAccessor::GetInvestigationSubsystem(Evidence),
			Investigation);
		TestTrue(
			TEXT("Evidence actor should bind to photo capture events during BeginPlay"),
			FEvidenceActorTestAccessor::IsPhotoCaptureHandlerBound(
				Investigation, Evidence));

		FEditorScriptExecutionGuard ScriptExecutionGuard;
		FCapturedPhotoRecord OtherPhoto;
		OtherPhoto.EvidenceInstanceID = FGuid::NewGuid();
		Investigation->OnPhotoCaptured.Broadcast(OtherPhoto);
		TestFalse(
			TEXT("Another evidence photo should not mark this actor collected"),
			Evidence->GetEvidenceData().bAlreadyCollected);

		FCapturedPhotoRecord MatchingPhoto;
		MatchingPhoto.EvidenceInstanceID = EvidenceInstanceID;
		Investigation->OnPhotoCaptured.Broadcast(MatchingPhoto);
		TestTrue(
			TEXT("A matching photo event should mark this actor collected"),
			Evidence->GetEvidenceData().bAlreadyCollected);
		TestEqual(
			TEXT("A matching photo event should immediately swap to the check icon"),
			FEvidenceActorTestAccessor::GetStatusIconResource(Evidence),
			static_cast<UObject*>(CapturedIcon));
	}

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidencePhotoGalleryResetRefreshesCapturedStateTest,
	"Balhwajeom.Camera.Evidence.PhotoGalleryReset.RefreshesCapturedState",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidencePhotoGalleryResetRefreshesCapturedStateTest::RunTest(
	const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABalhwajeomEvidenceActor* Evidence =
		World->SpawnActorDeferred<ABalhwajeomEvidenceActor>(
			ABalhwajeomEvidenceActor::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Evidence actor should spawn"), Evidence))
	{
		GameInstance->Shutdown();
		return false;
	}
	Evidence->ConfigureInvestigationObject(TEXT("OBJ_01_005"));
	UGameplayStatics::FinishSpawningActor(Evidence, FTransform::Identity);
	if (!World->HasBegunPlay())
	{
		World->BeginPlay();
	}
	if (!Evidence->HasActorBegunPlay())
	{
		Evidence->DispatchBeginPlay();
	}

	Evidence->MarkAsCollected();
	TestTrue(
		TEXT("The evidence actor should begin in a captured visual state"),
		Evidence->GetEvidenceData().bAlreadyCollected);

	UBalhwajeomInvestigationSubsystem* Investigation =
		GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>();
	bool bResetSucceeded = false;
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		bResetSucceeded = Investigation->ResetPersistentPhotoGallery();
	}
	TestTrue(
		TEXT("The photo gallery reset should succeed"),
		bResetSucceeded);
	TestFalse(
		TEXT("A gallery reset should refresh the evidence actor to not captured"),
		Evidence->GetEvidenceData().bAlreadyCollected);

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceProgressionGateTest,
	"Balhwajeom.Camera.Evidence.ProgressionGate.DisablesAndRestoresInteraction",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceProgressionGateTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	UStoryStateSubsystem* StoryState =
		GameInstance->GetSubsystem<UStoryStateSubsystem>();
	if (!TestNotNull(TEXT("StoryState subsystem should exist"), StoryState))
	{
		GameInstance->Shutdown();
		return false;
	}
	StoryState->ClearStateTags();

	ABalhwajeomEvidenceActor* Evidence =
		World->SpawnActorDeferred<ABalhwajeomEvidenceActor>(
			ABalhwajeomEvidenceActor::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Evidence actor should spawn"), Evidence))
	{
		GameInstance->Shutdown();
		return false;
	}

	FEvidenceActorTestAccessor::Enable3DInspection(Evidence);
	Evidence->ConfigureInvestigationObject(TEXT("OBJ_01_004"));
	UGameplayStatics::FinishSpawningActor(Evidence, FTransform::Identity);
	if (!World->HasBegunPlay())
	{
		World->BeginPlay();
	}
	if (!Evidence->HasActorBegunPlay())
	{
		Evidence->DispatchBeginPlay();
	}
	FEvidenceActorTestAccessor::SetRequiredActivationTag(
		Evidence,
		BalhwajeomGameplayTags::Story_Chapter_01_Phase_01_Completed);
	FEvidenceActorTestAccessor::RefreshProgressionAvailability(Evidence);

	TestFalse(
		TEXT("Evidence requiring an absent phase tag should be progression-locked"),
		FEvidenceActorTestAccessor::IsProgressionAvailable(Evidence));
	TestTrue(
		TEXT("A progression lock should keep the visible mesh enabled"),
		Evidence->FindComponentByClass<UStaticMeshComponent>()->IsVisible());
	TestTrue(
		TEXT("A progression lock should keep the visible mesh collision enabled"),
		Evidence->FindComponentByClass<UStaticMeshComponent>()->GetCollisionEnabled() !=
			ECollisionEnabled::NoCollision);
	TestFalse(
		TEXT("A progression-locked evidence actor should reject F interaction"),
		Evidence->CanRequestInvestigationInteraction());
	TestFalse(
		TEXT("A progression-locked evidence actor should not expose 3D inspection"),
		Evidence->GetItemInspectionComponent()->bInspectionEnabled);
	TestEqual(
		TEXT("A progression lock should disable the interaction-only camera bounds"),
		FEvidenceActorTestAccessor::GetCameraBoundsCollision(Evidence),
		ECollisionEnabled::NoCollision);

	FBalhwajeomCameraTargetInfo TargetInfo;
	TestFalse(
		TEXT("A progression-locked evidence actor should reject camera targeting"),
		Evidence->RequestCameraTargetInfo_Implementation(TargetInfo));

	TestTrue(
		TEXT("Evidence should bind to story tag additions during BeginPlay"),
		FEvidenceActorTestAccessor::IsStoryStateHandlerBound(StoryState, Evidence));
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		TestTrue(
			TEXT("The required phase completion tag should be newly added"),
			StoryState->AddStateTag(
				BalhwajeomGameplayTags::Story_Chapter_01_Phase_01_Completed));
	}

	TestTrue(
		TEXT("Adding the required phase tag should unlock the same actor immediately"),
		FEvidenceActorTestAccessor::IsProgressionAvailable(Evidence));
	TestTrue(
		TEXT("Unlocking should restore authored 3D inspection"),
		Evidence->GetItemInspectionComponent()->bInspectionEnabled);
	TestEqual(
		TEXT("Unlocking should restore the camera bounds collision"),
		FEvidenceActorTestAccessor::GetCameraBoundsCollision(Evidence),
		ECollisionEnabled::QueryOnly);
	TestTrue(
		TEXT("Unlocked evidence should be available to the photo camera"),
		Evidence->RequestCameraTargetInfo_Implementation(TargetInfo));

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceProgressionObstacleClearTest,
	"Balhwajeom.Camera.Evidence.ProgressionObstacle.ClearsAndUnlocksNextPhase",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceProgressionObstacleClearTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	UStoryStateSubsystem* StoryState =
		GameInstance->GetSubsystem<UStoryStateSubsystem>();
	if (!TestNotNull(TEXT("StoryState subsystem should exist"), StoryState))
	{
		GameInstance->Shutdown();
		return false;
	}
	StoryState->ClearStateTags();

	const FGameplayTag Phase01Completed =
		BalhwajeomGameplayTags::Story_Chapter_01_Phase_01_Completed;
	const FGameplayTag Phase02Unlocked =
		BalhwajeomGameplayTags::Story_Chapter_01_Phase_02_Unlocked;
	const FGameplayTag Phase02Completed =
		BalhwajeomGameplayTags::Story_Chapter_01_Phase_02_Completed;
	const FGameplayTag Phase03Unlocked =
		BalhwajeomGameplayTags::Story_Chapter_01_Phase_03_Unlocked;
	TestTrue(
		TEXT("Chapter 01 phase 02 unlocked tag should be registered"),
		Phase02Unlocked.IsValid());
	TestTrue(
		TEXT("Chapter 01 phase 03 unlocked tag should be registered"),
		Phase03Unlocked.IsValid());

	ABalhwajeomEvidenceActor* Obstacle =
		World->SpawnActorDeferred<ABalhwajeomEvidenceActor>(
			ABalhwajeomEvidenceActor::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Progression obstacle should spawn"), Obstacle))
	{
		GameInstance->Shutdown();
		return false;
	}

	Obstacle->ConfigureInvestigationObject(TEXT("Obstacle_Phase01"));
	UGameplayStatics::FinishSpawningActor(Obstacle, FTransform::Identity);
	if (!World->HasBegunPlay())
	{
		World->BeginPlay();
	}
	if (!Obstacle->HasActorBegunPlay())
	{
		Obstacle->DispatchBeginPlay();
	}

	FText InteractionText;
	TestTrue(
		TEXT("Before phase completion the obstacle should keep its normal repeatable interaction"),
		Obstacle->RequestInvestigationInteraction(InteractionText));
	TestEqual(
		TEXT("Before phase completion the obstacle should explain why it remains"),
		InteractionText.ToString(),
		FString(TEXT("아직 모든 의문이 해결되지 않았다.")));
	TestFalse(
		TEXT("The obstacle should remain visible before its completion condition"),
		Obstacle->IsHidden());
	TestTrue(
		TEXT("The obstacle should continue blocking movement before it is cleared"),
		Obstacle->GetActorEnableCollision());

	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		StoryState->AddStateTag(Phase01Completed);
	}

	InteractionText = FText::GetEmpty();
	TestTrue(
		TEXT("After phase completion F interaction should clear the obstacle"),
		Obstacle->RequestInvestigationInteraction(InteractionText));
	TestTrue(
		TEXT("Clearing the obstacle should not show the previous inspection text"),
		InteractionText.IsEmpty());
	TestTrue(
		TEXT("A cleared obstacle should be hidden"),
		Obstacle->IsHidden());
	TestFalse(
		TEXT("A cleared obstacle should stop blocking movement"),
		Obstacle->GetActorEnableCollision());
	TestTrue(
		TEXT("Clearing the phase 01 obstacle should unlock phase 02"),
		StoryState->HasStateTagExact(Phase02Unlocked));
	TestFalse(
		TEXT("A cleared phase 01 obstacle should reject repeated interaction"),
		Obstacle->RequestInvestigationInteraction(InteractionText));

	ABalhwajeomEvidenceActor* Phase02Obstacle =
		World->SpawnActorDeferred<ABalhwajeomEvidenceActor>(
			ABalhwajeomEvidenceActor::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Phase 02 progression obstacle should spawn"), Phase02Obstacle))
	{
		GameInstance->Shutdown();
		return false;
	}
	Phase02Obstacle->ConfigureInvestigationObject(TEXT("Obstacle_Phase02"));
	UGameplayStatics::FinishSpawningActor(Phase02Obstacle, FTransform::Identity);
	if (!Phase02Obstacle->HasActorBegunPlay())
	{
		Phase02Obstacle->DispatchBeginPlay();
	}

	InteractionText = FText::GetEmpty();
	TestTrue(
		TEXT("Before phase 02 completion the second obstacle should keep its normal interaction"),
		Phase02Obstacle->RequestInvestigationInteraction(InteractionText));
	TestEqual(
		TEXT("Before phase 02 completion the second obstacle should explain why it remains"),
		InteractionText.ToString(),
		FString(TEXT("아직 모든 의문이 해결되지 않았다.")));

	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		StoryState->AddStateTag(Phase02Completed);
	}
	InteractionText = FText::GetEmpty();
	TestTrue(
		TEXT("After phase 02 completion F interaction should clear the second obstacle"),
		Phase02Obstacle->RequestInvestigationInteraction(InteractionText));
	TestTrue(
		TEXT("The cleared phase 02 obstacle should be hidden"),
		Phase02Obstacle->IsHidden());
	TestFalse(
		TEXT("The cleared phase 02 obstacle should stop blocking movement"),
		Phase02Obstacle->GetActorEnableCollision());
	TestTrue(
		TEXT("Clearing the phase 02 obstacle should unlock phase 03"),
		StoryState->HasStateTagExact(Phase03Unlocked));

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
