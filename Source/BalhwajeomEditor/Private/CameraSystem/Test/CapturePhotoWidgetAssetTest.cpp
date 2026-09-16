#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "CameraSystem/BalhwajeomCapturePhotoWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "MultiShadowText.h"
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
	const UWidgetBlueprint* ContinuePromptBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_PhotoCheck.WBP_PhotoCheck"));
	if (!TestNotNull(TEXT("WBP_CapturePhoto exists"), Blueprint) ||
		!TestNotNull(TEXT("WBP_CapturePhoto has a widget tree"), Blueprint->WidgetTree.Get()) ||
		!TestNotNull(TEXT("capture-photo continue prompt exists"), ContinuePromptBlueprint) ||
		!TestNotNull(
			TEXT("capture-photo continue prompt has a widget tree"),
			ContinuePromptBlueprint->WidgetTree.Get()))
	{
		return false;
	}
	const UCanvasPanel* PromptRoot = Cast<UCanvasPanel>(
		ContinuePromptBlueprint->WidgetTree->RootWidget);
	const USizeBox* PromptContainer = Cast<USizeBox>(
		ContinuePromptBlueprint->WidgetTree->FindWidget(TEXT("SizeBox_0")));
	const UMultiShadowTextWidget* PromptLabel = Cast<UMultiShadowTextWidget>(
		ContinuePromptBlueprint->WidgetTree->FindWidget(TEXT("LabelText")));
	const UCanvasPanelSlot* PromptSlot = PromptContainer
		? Cast<UCanvasPanelSlot>(PromptContainer->Slot)
		: nullptr;
	TestNotNull(TEXT("continue prompt keeps a fullscreen canvas root"), PromptRoot);
	TestNotNull(TEXT("continue prompt keeps its authored container"), PromptContainer);
	TestNotNull(TEXT("continue prompt keeps its authored multi-shadow label"), PromptLabel);
	TestNotNull(TEXT("continue prompt container is positioned by the root canvas"), PromptSlot);
	if (PromptLabel)
	{
		TestEqual(
			TEXT("continue prompt explains the confirmation input"),
			PromptLabel->Text.ToString(),
			FString(TEXT("좌클릭하여 계속")));
	}
	if (PromptSlot)
	{
		const FAnchors Anchors = PromptSlot->GetAnchors();
		TestTrue(
			TEXT("continue prompt uses the bottom-center horizontal anchor"),
			FMath::IsNearlyEqual(Anchors.Minimum.X, 0.5));
		TestTrue(
			TEXT("continue prompt uses the bottom-center vertical anchor"),
			FMath::IsNearlyEqual(Anchors.Minimum.Y, 1.0));
		TestEqual(TEXT("continue prompt has a fixed anchor"), Anchors.Maximum, Anchors.Minimum);
		TestTrue(
			TEXT("continue prompt is centered on its anchor"),
			PromptSlot->GetAlignment().Equals(FVector2D(0.5f, 1.0f)));
		TestTrue(
			TEXT("continue prompt has the intended bottom margin"),
			PromptSlot->GetPosition().Equals(FVector2D(0.0f, -96.0f)));
		TestTrue(TEXT("continue prompt preserves its authored desired size"), PromptSlot->GetAutoSize());
	}

	TestTrue(
		TEXT("WBP_CapturePhoto derives from the runtime capture widget"),
		Blueprint->ParentClass &&
			Blueprint->ParentClass->IsChildOf(UBalhwajeomCapturePhotoWidget::StaticClass()));
	TestNotNull(TEXT("fullscreen dimmer exists"),
		Cast<UBorder>(Blueprint->WidgetTree->FindWidget(TEXT("ScreenDimmer"))));
	TestNotNull(TEXT("editable card root exists"),
		Cast<UCanvasPanel>(Blueprint->WidgetTree->FindWidget(TEXT("CardRoot"))));
	URetainerBox* CardComposite = Cast<URetainerBox>(
		Blueprint->WidgetTree->FindWidget(TEXT("CardComposite")));
	UCanvasPanel* CardVisualRoot = Cast<UCanvasPanel>(
		Blueprint->WidgetTree->FindWidget(TEXT("CardVisualRoot")));
	TestNotNull(TEXT("card composite retainer exists"), CardComposite);
	TestNotNull(TEXT("card visual root exists"), CardVisualRoot);
	TestNotNull(TEXT("captured photo image exists"),
		Cast<UImage>(Blueprint->WidgetTree->FindWidget(TEXT("CapturedPhotoImage"))));
	TestNotNull(TEXT("sentence text exists"),
		Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("SentenceTextBlock"))));
	TestNotNull(TEXT("analysis sentence builder exists"),
		Cast<UWrapBox>(Blueprint->WidgetTree->FindWidget(TEXT("SentenceBuilder"))));
	TestNotNull(TEXT("keyword list exists"),
		Cast<UVerticalBox>(Blueprint->WidgetTree->FindWidget(TEXT("KeywordList"))));
	const UCanvasPanel* Card = Cast<UCanvasPanel>(
		Blueprint->WidgetTree->FindWidget(TEXT("CardRoot")));
	const UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
	TestTrue(TEXT("card geometry is designer-editable on the root canvas"), CardSlot != nullptr);

	TestTrue(TEXT("retainer owns the complete card visual subtree"),
		CardComposite && CardComposite->GetContent() == CardVisualRoot);
	const UWidget* CardBackground = Blueprint->WidgetTree->FindWidget(TEXT("CardBackground"));
	const UWidget* CapturedPhoto = Blueprint->WidgetTree->FindWidget(TEXT("CapturedPhotoImage"));
	const UWidget* SentenceBackground = Blueprint->WidgetTree->FindWidget(TEXT("SentenceBackground"));
	const UWidget* KeywordList = Blueprint->WidgetTree->FindWidget(TEXT("KeywordList"));
	TestTrue(TEXT("card background is rendered inside the composite"),
		CardVisualRoot && CardBackground && CardBackground->GetParent() == CardVisualRoot);
	TestTrue(TEXT("captured photo is rendered inside the composite"),
		CardVisualRoot && CapturedPhoto && CapturedPhoto->GetParent() == CardVisualRoot);
	TestTrue(TEXT("sentence is rendered inside the composite"),
		CardVisualRoot && SentenceBackground && SentenceBackground->GetParent() == CardVisualRoot);
	TestTrue(TEXT("keywords remain outside the card composite"),
		Card && KeywordList && KeywordList->GetParent() == Card);

	return !HasAnyErrors();
}

#endif
