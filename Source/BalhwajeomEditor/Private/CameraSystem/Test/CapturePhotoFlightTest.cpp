#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CameraSystem/BalhwajeomCapturePhotoWidget.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "RenderingThread.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCapturePhotoFlightTest,
	"Balhwajeom.Camera.CapturePhotoFlight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapturePhotoFlightTest::RunTest(const FString& Parameters)
{
	UClass* WidgetClass = LoadClass<UBalhwajeomCapturePhotoWidget>(nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto.WBP_CapturePhoto_C"));
	auto* Widget = WidgetClass && GEditor
		? CreateWidget<UBalhwajeomCapturePhotoWidget>(GEditor->GetEditorWorldContext().World(), WidgetClass)
		: nullptr;
	if (!TestNotNull(TEXT("Live capture widget"), Widget)) return false;
	Widget->AddToRoot();
	FWidgetRenderer Renderer(true);
	auto SlateWidget = Widget->TakeWidget();
	for (const float DrawScale : {0.67f, 1.0f, 1.5f})
	{
		const FVector2D Size(1920, 1080);
		UTextureRenderTarget2D* Target = FWidgetRenderer::CreateTargetFor(Size, TF_Bilinear, true);
		Target->AddToRoot();
		auto Draw = [&]()
		{
			Widget->ForceLayoutPrepass();
			Renderer.DrawWidget(Target, SlateWidget, DrawScale, Size, 1.0f / 60.0f);
			FlushRenderingCommands();
		};
		auto Center = [](UWidget* Item)
		{
			const FGeometry G = Item->GetCachedGeometry();
			return FVector2D(G.LocalToAbsolute(G.GetLocalSize() * 0.5f));
		};
		// Reuse the same widget after its previous flight. Rebuild an initially
		// collapsed keyword list, then repeat with a different keyword count.
		for (int32 Capture = 0; Capture < 3; ++Capture)
		{
			TArray<FText> Words;
			for (int32 I = 0; I < Capture; ++I) Words.Add(FText::FromString(TEXT("Keyword")));
			Widget->PresentCapture(nullptr, FText::FromString(TEXT("Sentence")), Words);
			TestFalse(TEXT("Previous capture geometry is invalidated"), Widget->IsFlightReady());
			// Include an ancestor scale/translation to exercise local vs viewport units.
			FWidgetTransform Ancestor;
			Ancestor.Scale = FVector2D(0.8f, 1.1f);
			Ancestor.Translation = FVector2D(35, -18);
			Widget->CardRoot->SetRenderTransform(Ancestor);
			for (int32 Frame = 0; Frame < 5; ++Frame)
			{
				Draw();
			}
			if (!TestTrue(TEXT("Fresh rendered layout becomes ready"), Widget->IsFlightReady())) continue;
			const FVector2D Destination = Center(Widget->TabFlyTarget);
			const FVector2D KeywordOrigin = Capture ? Center(Widget->KeywordList) : FVector2D::ZeroVector;
			Widget->ApplyFlyToTab(Widget->FlyDuration / Widget->GetFlyDuration());
			// Make the final invisible image paintable so its actual rendered center can be measured.
			Widget->CapturedPhotoImage->SetRenderOpacity(1);
			Draw();
			TestTrue(FString::Printf(TEXT("Photo reaches marker scale=%g capture=%d"), DrawScale, Capture),
				Center(Widget->CapturedPhotoImage).Equals(Destination, 1.0f));
			if (Capture)
			{
				TestTrue(TEXT("Keywords remain at origin until photo arrives"),
					Center(Widget->KeywordList).Equals(KeywordOrigin, 1.0f));
				Widget->ApplyFlyToTab(1);
				Widget->KeywordList->SetRenderOpacity(1);
				Draw();
				TestTrue(FString::Printf(TEXT("Keywords reach same marker scale=%g capture=%d"), DrawScale, Capture),
					Center(Widget->KeywordList).Equals(Destination, 1.0f));
			}
			Widget->SetVisibility(ESlateVisibility::Collapsed);
			Draw();
		}
		Target->RemoveFromRoot();
	}
	Widget->RemoveFromRoot();
	return !HasAnyErrors();
}
#endif
