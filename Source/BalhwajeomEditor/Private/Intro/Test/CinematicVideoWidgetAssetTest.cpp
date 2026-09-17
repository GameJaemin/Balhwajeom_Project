#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "UI/BalhwajeomCinematicVideoWidget.h"
#include "WidgetBlueprint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCinematicVideoSkipButtonAssetTest,
	"Balhwajeom.Intro.CinematicVideo.SkipButtonAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCinematicVideoSkipButtonAssetTest::RunTest(const FString& Parameters)
{
	const UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Title/WBP_CinematicVideo.WBP_CinematicVideo"));
	if (!TestNotNull(TEXT("WBP_CinematicVideo should load"), Blueprint) ||
		!TestNotNull(TEXT("WBP_CinematicVideo should have a widget tree"),
			Blueprint ? Blueprint->WidgetTree.Get() : nullptr))
	{
		return false;
	}

	TestTrue(
		TEXT("WBP_CinematicVideo derives from the runtime cinematic widget"),
		Blueprint->ParentClass &&
			Blueprint->ParentClass->IsChildOf(
				UBalhwajeomCinematicVideoWidget::StaticClass()));

	UWidgetTree* Tree = Blueprint->WidgetTree;
	const UCanvasPanel* Root = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("VideoRoot")));
	const UWidget* Background = Tree->FindWidget(TEXT("VideoBackground"));
	const UImage* Video = Cast<UImage>(Tree->FindWidget(TEXT("IMG_Video")));
	TestNotNull(TEXT("the video root canvas exists"), Root);
	TestNotNull(TEXT("the opaque backing plate exists"), Background);
	TestNotNull(TEXT("the video image exists"), Video);

	// SetBackgroundVisible() and SetMediaTexture() reach these through
	// BindWidgetOptional, which only binds widgets exposed as Blueprint variables.
	TestTrue(TEXT("the backing plate is bindable"), Background && Background->bIsVariable);
	TestTrue(TEXT("the video image is bindable"), Video && Video->bIsVariable);

	// The skip control is generated, not hand-placed. CreateIntroFlowAssets() clears
	// this widget's tree and rebuilds it, so a button added only in the designer
	// disappears the next time that script runs -- which is what this guards.
	UButton* Skip = Cast<UButton>(Tree->FindWidget(TEXT("BTN_Skip")));
	if (!TestNotNull(TEXT("WBP_CinematicVideo should contain BTN_Skip"), Skip))
	{
		return false;
	}
	TestTrue(TEXT("the skip button is bindable"), Skip->bIsVariable);

	// Scaling chain: the button lives on its own 1920x1080 design layer so it scales
	// with the viewport instead of following the engine's default DPI curve.
	const UCanvasPanel* SkipCanvas = Cast<UCanvasPanel>(Skip->GetParent());
	const USizeBox* SkipDesign = SkipCanvas
		? Cast<USizeBox>(SkipCanvas->GetParent())
		: nullptr;
	const UScaleBox* SkipScale = SkipDesign
		? Cast<UScaleBox>(SkipDesign->GetParent())
		: nullptr;
	if (!TestNotNull(TEXT("the skip button sits on its own canvas"), SkipCanvas) ||
		!TestNotNull(TEXT("the skip canvas sits in a design-size box"), SkipDesign) ||
		!TestNotNull(TEXT("the design box sits in a scale box"), SkipScale))
	{
		return false;
	}

	TestEqual(TEXT("the design layer is authored at 1920 wide"),
		SkipDesign->GetWidthOverride(), 1920.0f);
	TestEqual(TEXT("the design layer is authored at 1080 tall"),
		SkipDesign->GetHeightOverride(), 1080.0f);
	TestEqual(TEXT("the scale layer fits the viewport"),
		SkipScale->GetStretch(), EStretch::ScaleToFit);
	TestTrue(TEXT("the scale layer is a child of the video root"),
		SkipScale->GetParent() == Root);

	// The layer spans the movie, so only the button itself may take clicks; the title
	// screen stays live under a clip that plays over it.
	TestEqual(TEXT("the scale layer does not take clicks"),
		SkipScale->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("the design layer does not take clicks"),
		SkipDesign->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("the skip canvas does not take clicks"),
		SkipCanvas->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	// Anchored to the top-right corner of the design layer.
	const UCanvasPanelSlot* SkipSlot = Cast<UCanvasPanelSlot>(Skip->Slot);
	if (TestNotNull(TEXT("the skip button sits on a canvas slot"), SkipSlot))
	{
		const FAnchors Anchors = SkipSlot->GetAnchors();
		TestTrue(TEXT("the skip button is anchored to the top-right"),
			Anchors.Minimum.Equals(FVector2D(1.0f, 0.0f)) &&
			Anchors.Maximum.Equals(FVector2D(1.0f, 0.0f)));
		TestTrue(TEXT("the skip button is aligned from its top-right"),
			SkipSlot->GetAlignment().Equals(FVector2D(1.0f, 0.0f)));
		TestTrue(TEXT("the skip button is inset from the corner"),
			SkipSlot->GetPosition().X < 0.0f && SkipSlot->GetPosition().Y > 0.0f);
	}

	// Every state draws the same Image brush. The engine's default button brushes are
	// bordered boxes, so a state left on its default is what puts a square outline
	// around the art; state feedback has to come from tint alone.
	const FButtonStyle& Style = Skip->GetStyle();

	const UTexture2D* SkipTexture = LoadObject<UTexture2D>(
		nullptr, TEXT("/Game/Balhwajeom/UI/Title/skip_button.skip_button"));
	if (TestNotNull(TEXT("the skip button art should load"), SkipTexture) && SkipSlot)
	{
		// Native art size keeps the button from being stretched out of its aspect.
		// GetImportedSize() is the authored source size; GetSizeX/Y() report the
		// resident mip, which is 0 or a small mip in a commandlet.
		const FIntPoint ImportedSize = SkipTexture->GetImportedSize();
		const FVector2D ArtSize(ImportedSize.X, ImportedSize.Y);
		TestTrue(TEXT("the skip button art reports an imported size"),
			ImportedSize.GetMin() > 0);
		TestTrue(
			*FString::Printf(
				TEXT("the skip button is sized to its art (slot %s, art %s, brush %s)"),
				*SkipSlot->GetSize().ToString(),
				*ArtSize.ToString(),
				*Style.Normal.ImageSize.ToString()),
			SkipSlot->GetSize().Equals(ArtSize, 0.5f));
	}

	const TArray<TPair<FString, const FSlateBrush*>> StateBrushes =
	{
		{ TEXT("normal"), &Style.Normal },
		{ TEXT("hovered"), &Style.Hovered },
		{ TEXT("pressed"), &Style.Pressed },
		{ TEXT("disabled"), &Style.Disabled }
	};
	for (const TPair<FString, const FSlateBrush*>& State : StateBrushes)
	{
		TestEqual(
			*FString::Printf(TEXT("the %s brush draws as a plain image"), *State.Key),
			State.Value->DrawAs,
			ESlateBrushDrawType::Image);
		TestTrue(
			*FString::Printf(TEXT("the %s brush uses the skip art"), *State.Key),
			State.Value->GetResourceObject() == SkipTexture);
	}

	TestTrue(TEXT("the normal state adds no padding"),
		Style.NormalPadding.GetDesiredSize().IsNearlyZero());
	TestTrue(TEXT("the pressed state adds no padding"),
		Style.PressedPadding.GetDesiredSize().IsNearlyZero());

	// A focusable button draws Slate's focus rectangle over the art.
	TestFalse(TEXT("the skip button is not keyboard focusable"), Skip->GetIsFocusable());

	return !HasAnyErrors();
}

#endif
