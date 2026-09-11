#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "CameraSystem/BalhwajeomCapturePhotoWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "WidgetBlueprint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapturePhotoWidgetAssetTest,
	"Balhwajeom.Camera.CapturePhotoWidgetAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapturePhotoWidgetAssetTest::RunTest(const FString& Parameters)
{
	const UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto.WBP_CapturePhoto"));
	if (!TestNotNull(TEXT("WBP_CapturePhoto exists"), Blueprint) ||
		!TestNotNull(TEXT("WBP_CapturePhoto has a widget tree"), Blueprint->WidgetTree.Get()))
	{
		return false;
	}

	TestTrue(
		TEXT("WBP_CapturePhoto derives from the runtime capture widget"),
		Blueprint->ParentClass &&
			Blueprint->ParentClass->IsChildOf(UBalhwajeomCapturePhotoWidget::StaticClass()));
	TestNotNull(TEXT("fullscreen dimmer exists"),
		Cast<UBorder>(Blueprint->WidgetTree->FindWidget(TEXT("ScreenDimmer"))));
	TestNotNull(TEXT("editable card root exists"),
		Cast<UCanvasPanel>(Blueprint->WidgetTree->FindWidget(TEXT("CardRoot"))));
	TestNotNull(TEXT("captured photo image exists"),
		Cast<UImage>(Blueprint->WidgetTree->FindWidget(TEXT("CapturedPhotoImage"))));
	TestNotNull(TEXT("sentence text exists"),
		Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("SentenceTextBlock"))));
	TestNotNull(TEXT("keyword list exists"),
		Cast<UVerticalBox>(Blueprint->WidgetTree->FindWidget(TEXT("KeywordList"))));
	TestNotNull(TEXT("designer-authored TabFlyTarget exists"),
		Cast<USizeBox>(Blueprint->WidgetTree->FindWidget(TEXT("TabFlyTarget"))));

	const UCanvasPanel* Card = Cast<UCanvasPanel>(
		Blueprint->WidgetTree->FindWidget(TEXT("CardRoot")));
	const UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
	TestTrue(TEXT("card geometry is designer-editable on the root canvas"), CardSlot != nullptr);

	return !HasAnyErrors();
}

#endif
