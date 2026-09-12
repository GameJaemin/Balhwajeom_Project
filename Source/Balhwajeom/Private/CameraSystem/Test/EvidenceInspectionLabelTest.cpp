#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "CameraSystem/BalhwajeomEvidenceActor.h"
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

};


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

#endif
