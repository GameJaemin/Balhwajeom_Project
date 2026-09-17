#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TextWidgetTypes.h"
#include "Tutorial/BalhwajeomTutorialOverlayLayout.h"
#include "Tutorial/BalhwajeomTutorialOverlayWidget.h"
#include "WidgetBlueprint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayWidgetBlueprintTest,
	"Balhwajeom.Tutorial.Overlay.WidgetBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlayWidgetBlueprintTest::RunTest(const FString& Parameters)
{
	namespace Layout = BalhwajeomTutorialOverlayLayout;

	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
		nullptr, TEXT("/Game/Balhwajeom/UI/HUD/WBP_TutorialOverlay.WBP_TutorialOverlay"));
	if (!TestNotNull(TEXT("WBP_TutorialOverlay should load"), Blueprint) ||
		!TestNotNull(TEXT("WBP_TutorialOverlay should have a widget tree"),
			Blueprint ? Blueprint->WidgetTree.Get() : nullptr))
	{
		return false;
	}

	// Without the runtime parent the image row would never be built, because a Widget
	// Blueprint on its own cannot turn 0..n DataTable images into children.
	TestTrue(TEXT("WBP_TutorialOverlay should be parented to its runtime class"),
		Blueprint->ParentClass == UBalhwajeomTutorialOverlayWidget::StaticClass());

	UWidgetTree* Tree = Blueprint->WidgetTree;
	UScaleBox* ImageScale = Tree->FindWidget<UScaleBox>(FName(Layout::WidgetNames::ImageScale));
	UHorizontalBox* ImageRow =
		Tree->FindWidget<UHorizontalBox>(FName(Layout::WidgetNames::ImageRow));
	UTextBlock* Title = Tree->FindWidget<UTextBlock>(FName(Layout::WidgetNames::Title));
	USizeBox* DescriptionBox =
		Tree->FindWidget<USizeBox>(FName(Layout::WidgetNames::DescriptionBox));
	UTextBlock* Description =
		Tree->FindWidget<UTextBlock>(FName(Layout::WidgetNames::Description));
	UTextBlock* ContinuePrompt =
		Tree->FindWidget<UTextBlock>(FName(Layout::WidgetNames::ContinuePrompt));

	if (!TestNotNull(TEXT("SCB_Images should exist and be a ScaleBox"), ImageScale) ||
		!TestNotNull(TEXT("HB_Images should exist and be a HorizontalBox"), ImageRow) ||
		!TestNotNull(TEXT("TXT_Title should exist and be a TextBlock"), Title) ||
		!TestNotNull(TEXT("SB_Description should exist and be a SizeBox"), DescriptionBox) ||
		!TestNotNull(TEXT("TXT_Description should exist and be a TextBlock"), Description) ||
		!TestNotNull(TEXT("TXT_ContinuePrompt should exist and be a TextBlock"), ContinuePrompt))
	{
		return false;
	}

	// BindWidgetOptional only finds widgets that are Blueprint variables. Unticking "Is
	// Variable" in the designer would leave the overlay blank with no compile error.
	const TArray<TPair<const TCHAR*, const UWidget*>> BoundWidgets = {
		{ Layout::WidgetNames::ImageScale, ImageScale },
		{ Layout::WidgetNames::ImageRow, ImageRow },
		{ Layout::WidgetNames::Title, Title },
		{ Layout::WidgetNames::DescriptionBox, DescriptionBox },
		{ Layout::WidgetNames::Description, Description },
		{ Layout::WidgetNames::ContinuePrompt, ContinuePrompt }
	};
	for (const TPair<const TCHAR*, const UWidget*>& Bound : BoundWidgets)
	{
		TestTrue(
			FString::Printf(TEXT("%s should be exposed as a variable"), Bound.Key),
			Bound.Value->bIsVariable);
	}

	const FProperty* JustificationProperty =
		UTextLayoutWidget::StaticClass()->FindPropertyByName(TEXT("Justification"));
	if (TestNotNull(TEXT("Justification should be reflected"), JustificationProperty))
	{
		const uint8* Justification =
			JustificationProperty->ContainerPtrToValuePtr<uint8>(Description);
		TestTrue(TEXT("TXT_Description should stay left aligned"),
			Justification && *Justification == static_cast<uint8>(ETextJustify::Left));
	}
	TestTrue(TEXT("TXT_Description should wrap"), Description->GetAutoWrapText());

	// Images, heading, prose, prompt - the heading sits under the images by design.
	const UPanelWidget* Column =
		Cast<UPanelWidget>(Tree->FindWidget(FName(Layout::WidgetNames::ContentColumn)));
	if (TestNotNull(TEXT("The content column should exist"), Column))
	{
		TestEqual(TEXT("The image row should come first"),
			Column->GetChildIndex(ImageScale), 0);
		TestEqual(TEXT("TXT_Title should sit between the images and the prose"),
			Column->GetChildIndex(Title), 1);
		TestEqual(TEXT("SB_Description should follow the heading"),
			Column->GetChildIndex(DescriptionBox), 2);
		TestEqual(TEXT("TXT_ContinuePrompt should close the column"),
			Column->GetChildIndex(ContinuePrompt), 3);
	}

	// A WidthOverride here would left-shift every short line inside a fixed-width box.
	TestTrue(TEXT("SB_Description should cap its width"),
		DescriptionBox->IsMaxDesiredWidthOverride());
	TestFalse(TEXT("SB_Description should not force a fixed width"),
		DescriptionBox->IsWidthOverride());

	TestTrue(TEXT("The image row should shrink rather than clip"),
		ImageScale->GetStretch() == EStretch::ScaleToFit);
	TestTrue(TEXT("The image row should never be scaled up"),
		ImageScale->GetStretchDirection() == EStretchDirection::DownOnly);

	TestTrue(TEXT("TXT_ContinuePrompt should carry the fixed continue line"),
		!ContinuePrompt->GetText().IsEmptyOrWhitespace());

	TestNotNull(TEXT("The overlay should blur the background"),
		Tree->FindWidget(FName(Layout::WidgetNames::BackgroundBlur)));
	TestNotNull(TEXT("The overlay should dim the background"),
		Tree->FindWidget(FName(Layout::WidgetNames::Dim)));
	return true;
}

#endif
