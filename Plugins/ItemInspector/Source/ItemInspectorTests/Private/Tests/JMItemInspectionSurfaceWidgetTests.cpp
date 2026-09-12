#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "ItemInspection/JMItemInspectionPreviewActor.h"
#include "ItemInspection/JMItemInspectionSurfaceWidgetBase.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJMItemInspectionPaperSurfaceWidgetTest,
	"JM.ItemInspector.SurfaceWidget.PaperPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMItemInspectionPaperSurfaceWidgetTest::RunTest(const FString& Parameters)
{
	// Exercise the portable native surface fallback without importing paper sample assets.
	TStrongObjectPtr<UJMItemInspectionData> PaperDataOwner(NewObject<UJMItemInspectionData>());
	UJMItemInspectionData* PaperData = PaperDataOwner.Get();
	PaperData->Description = FText::FromString(TEXT("Surface widget regression fixture"));
	PaperData->PreviewMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Plane.Plane")));
	PaperData->SurfaceWidget.bEnabled = true;
	PaperData->SurfaceWidget.bUseDescriptionAsContent = true;
	PaperData->SurfaceWidget.DrawSize = FIntPoint(700, 1000);
	PaperData->SurfaceWidget.WidgetClass.Reset();
	PaperData->ViewSettings.PreviewScale = 1.0f;
	PaperData->ViewSettings.RotationSensitivity = 0.35f;
	PaperData->ViewSettings.bAllowPitchRotation = true;
	PaperData->ViewSettings.bAllowYawRotation = true;
	PaperData->ViewSettings.bAllowZoom = true;
	PaperData->ViewSettings.InitialZoom = 1.0f;
	PaperData->ViewSettings.MinZoom = 0.75f;
	PaperData->ViewSettings.MaxZoom = 2.0f;
	TestNotNull(TEXT("Paper inspection data loads"), PaperData);
	if (!PaperData)
	{
		return false;
	}

	TestTrue(TEXT("Paper surface widget is enabled"), PaperData->SurfaceWidget.bEnabled);
	TestTrue(TEXT("Paper uses Description as its single authored content source"), PaperData->SurfaceWidget.bUseDescriptionAsContent);
	TestFalse(TEXT("Paper Description contains note text"), PaperData->Description.IsEmpty());
	TestTrue(TEXT("Paper allows yaw rotation"), PaperData->ViewSettings.bAllowYawRotation);
	TestTrue(TEXT("Paper allows pitch rotation"), PaperData->ViewSettings.bAllowPitchRotation);
	TestEqual(TEXT("Surface draw size is safe"), PaperData->SurfaceWidget.GetSafeDrawSize(), FIntPoint(700, 1000));

	UClass* SurfaceWidgetClass = UJMItemInspectionSurfaceWidgetBase::StaticClass();
	TestNotNull(TEXT("WB_NoteMaster loads"), SurfaceWidgetClass);
	TestTrue(
		TEXT("WB_NoteMaster derives from the reusable surface widget base"),
		SurfaceWidgetClass && SurfaceWidgetClass->IsChildOf(UJMItemInspectionSurfaceWidgetBase::StaticClass()));

	UStaticMesh* PaperMesh = PaperData->PreviewMesh.LoadSynchronous();
	TestNotNull(TEXT("Paper preview mesh loads"), PaperMesh);
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	AJMItemInspectionPreviewActor* PreviewActor = World
		? World->SpawnActor<AJMItemInspectionPreviewActor>(FVector::ZeroVector, FRotator::ZeroRotator)
		: nullptr;
	TestNotNull(TEXT("Preview actor spawns"), PreviewActor);
	if (!PreviewActor || !PaperMesh)
	{
		return false;
	}

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(PreviewActor);
	RenderTarget->InitAutoFormat(512, 512);
	if (!TestTrue(TEXT("Paper preview configures"), PreviewActor->ConfigurePreview(PaperData, PaperMesh, RenderTarget, 45.0f, 0.1f)))
	{
		PreviewActor->Destroy();
		return false;
	}

	UWidgetComponent* SurfaceComponent = PreviewActor->FindComponentByClass<UWidgetComponent>();
	TestNotNull(TEXT("Preview owns a surface WidgetComponent"), SurfaceComponent);
	if (!SurfaceComponent)
	{
		return false;
	}

	TestTrue(TEXT("Surface widget is visible"), SurfaceComponent->IsVisible());
	TestEqual(TEXT("Surface widget class matches Paper data"), SurfaceComponent->GetWidgetClass().Get(), SurfaceWidgetClass);
	UJMItemInspectionSurfaceWidgetBase* SurfaceWidget = Cast<UJMItemInspectionSurfaceWidgetBase>(SurfaceComponent->GetWidget());
	TestNotNull(TEXT("Surface widget instance initializes"), SurfaceWidget);
	if (SurfaceWidget)
	{
		TestEqual(TEXT("Surface receives the Paper Description"), SurfaceWidget->GetSurfaceContent().ToString(), PaperData->Description.ToString());
	}

	USceneComponent* PreviewPivot = nullptr;
	TInlineComponentArray<USceneComponent*> SceneComponents(PreviewActor);
	for (USceneComponent* Component : SceneComponents)
	{
		if (Component && Component->GetFName() == TEXT("PreviewPivot"))
		{
			PreviewPivot = Component;
			break;
		}
	}
	TestNotNull(TEXT("Preview pivot exists"), PreviewPivot);
	TestEqual(TEXT("Surface widget follows the same rotating pivot as the mesh"), SurfaceComponent->GetAttachParent(), PreviewPivot);
	if (PreviewPivot)
	{
		FJMItemInspectionTransitionSource TransitionSource;
		TransitionSource.bIsValid = true;
		TransitionSource.bIsOnScreen = true;
		TransitionSource.PreviewRelativeRotation = FRotator(12.0f, 37.0f, 0.0f).Quaternion();
		PreviewActor->BeginEnterTransition(TransitionSource);
		PreviewActor->UpdateEnterTransition(0.0f);
		TestTrue(
			TEXT("Entrance transition starts from the sampled source rotation"),
			PreviewPivot->GetRelativeRotation().Quaternion().Equals(TransitionSource.PreviewRelativeRotation, KINDA_SMALL_NUMBER));

		UStaticMeshComponent* PreviewMeshComponent = PreviewActor->FindComponentByClass<UStaticMeshComponent>();
		USceneCaptureComponent2D* SceneCapture = PreviewActor->FindComponentByClass<USceneCaptureComponent2D>();
		const float StartCameraDistance = SceneCapture ? FMath::Abs(SceneCapture->GetRelativeLocation().X) : 0.0f;
		PreviewActor->UpdateEnterTransition(0.5f);
		PreviewActor->CompleteEnterTransition();
		TestTrue(
			TEXT("Entrance transition commits the Data Asset initial rotation"),
			PreviewPivot->GetRelativeRotation().Quaternion().Equals(PaperData->ViewSettings.InitialRotation.Quaternion(), KINDA_SMALL_NUMBER));
		if (PreviewMeshComponent)
		{
			TestTrue(
				TEXT("Entrance transition commits the Data Asset preview scale"),
				PreviewMeshComponent->GetRelativeScale3D().Equals(FVector(PaperData->ViewSettings.PreviewScale), KINDA_SMALL_NUMBER));
		}
		if (SceneCapture)
		{
			TestNotEqual(
				TEXT("Entrance transition moves from its start zoom to the authored zoom"),
				StartCameraDistance,
				static_cast<float>(FMath::Abs(SceneCapture->GetRelativeLocation().X)));
		}

		const FQuat BeforeRotation = PreviewPivot->GetRelativeRotation().Quaternion();
		PreviewActor->RotatePreview(30.0f, 10.0f);
		const FQuat AfterRotation = PreviewPivot->GetRelativeRotation().Quaternion();
		TestFalse(TEXT("Paper and surface rotate during inspection"), BeforeRotation.Equals(AfterRotation));

		FJMItemInspectionTransitionSource ExitSource;
		ExitSource.bIsValid = true;
		ExitSource.bIsOnScreen = true;
		ExitSource.PreviewRelativeRotation = FRotator(-8.0f, -24.0f, 3.0f).Quaternion();
		PreviewActor->BeginExitTransition(ExitSource);
		PreviewActor->UpdateExitTransition(0.5f);
		PreviewActor->CompleteExitTransition();
		TestTrue(
			TEXT("Exit transition returns to the sampled source rotation"),
			PreviewPivot->GetRelativeRotation().Quaternion().Equals(ExitSource.PreviewRelativeRotation, KINDA_SMALL_NUMBER));
		if (PreviewMeshComponent)
		{
			TestTrue(
				TEXT("Exit transition returns to the entrance staging scale"),
				PreviewMeshComponent->GetRelativeScale3D().Equals(
					FVector(PaperData->ViewSettings.PreviewScale * 0.85f),
					KINDA_SMALL_NUMBER));
		}
	}

	PreviewActor->Destroy();
	return true;
}

#endif
