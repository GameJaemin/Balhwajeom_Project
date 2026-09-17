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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayPromptBlinkTest,
	"Balhwajeom.Tutorial.Overlay.PromptBlink",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlayPromptBlinkTest::RunTest(const FString& Parameters)
{
	using FOverlay = UBalhwajeomTutorialOverlayWidget;
	const float Period = 1.2f;
	const float Min = 0.25f;
	const float Max = 1.0f;

	TestEqual(TEXT("A fresh prompt starts dim"),
		FOverlay::ComputeBlinkOpacity(0.0f, Period, Min, Max), Min);
	TestEqual(TEXT("Half a period reaches full brightness"),
		FOverlay::ComputeBlinkOpacity(Period * 0.5f, Period, Min, Max), Max);
	TestEqual(TEXT("A full period is back to dim"),
		FOverlay::ComputeBlinkOpacity(Period, Period, Min, Max), Min);
	TestEqual(TEXT("The blink repeats on the next cycle"),
		FOverlay::ComputeBlinkOpacity(Period * 1.5f, Period, Min, Max), Max);

	// Linear, not eased: a quarter period must land exactly halfway.
	TestEqual(TEXT("A quarter period is exactly halfway up"),
		FOverlay::ComputeBlinkOpacity(Period * 0.25f, Period, Min, Max), (Min + Max) * 0.5f);
	TestEqual(TEXT("Three quarters of a period is exactly halfway down"),
		FOverlay::ComputeBlinkOpacity(Period * 0.75f, Period, Min, Max), (Min + Max) * 0.5f);

	for (int32 Step = 0; Step <= 40; ++Step)
	{
		const float Elapsed = Period * 3.0f * static_cast<float>(Step) / 40.0f;
		const float Opacity = FOverlay::ComputeBlinkOpacity(Elapsed, Period, Min, Max);
		if (Opacity < Min - KINDA_SMALL_NUMBER || Opacity > Max + KINDA_SMALL_NUMBER)
		{
			AddError(FString::Printf(
				TEXT("Blink opacity left its range at %.3fs: %.3f"), Elapsed, Opacity));
			break;
		}
	}

	// A zero period would divide by zero; a readable prompt is the safe answer.
	TestEqual(TEXT("A zero period leaves the prompt fully lit"),
		FOverlay::ComputeBlinkOpacity(0.5f, 0.0f, Min, Max), Max);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayImageSizeTest,
	"Balhwajeom.Tutorial.Overlay.UniformImageSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlayImageSizeTest::RunTest(const FString& Parameters)
{
	using FOverlay = UBalhwajeomTutorialOverlayWidget;
	const float Height = 250.0f;

	const FVector2D Wide = FOverlay::ComputeUniformImageSize(FIntPoint(1920, 1080), Height);
	const FVector2D Tall = FOverlay::ComputeUniformImageSize(FIntPoint(512, 1024), Height);
	const FVector2D Square = FOverlay::ComputeUniformImageSize(FIntPoint(256, 256), Height);

	// FVector2D components are double in UE5, so the comparison is made in one type.
	TestEqual(TEXT("A wide source keeps the shared height"),
		static_cast<float>(Wide.Y), Height);
	TestEqual(TEXT("A tall source keeps the shared height"),
		static_cast<float>(Tall.Y), Height);
	TestEqual(TEXT("A square source keeps the shared height"),
		static_cast<float>(Square.Y), Height);

	TestTrue(TEXT("A 16:9 source stays 16:9"),
		FMath::IsNearlyEqual(static_cast<float>(Wide.X), Height * 1920.0f / 1080.0f, 0.01f));
	TestTrue(TEXT("A 1:2 source stays 1:2"),
		FMath::IsNearlyEqual(static_cast<float>(Tall.X), Height * 0.5f, 0.01f));
	TestTrue(TEXT("A square source stays square"),
		FMath::IsNearlyEqual(static_cast<float>(Square.X), Height, 0.01f));

	TestTrue(TEXT("A source with no height is rejected instead of dividing by zero"),
		FOverlay::ComputeUniformImageSize(FIntPoint(256, 0), Height).IsNearlyZero());
	TestTrue(TEXT("A zero target height is rejected"),
		FOverlay::ComputeUniformImageSize(FIntPoint(256, 256), 0.0f).IsNearlyZero());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayLayoutContractTest,
	"Balhwajeom.Tutorial.Overlay.LayoutContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/** Justification has no public getter, so the contract is read off the reflected property. */
	bool IsJustified(const UTextBlock* TextBlock, ETextJustify::Type Expected)
	{
		const FProperty* JustificationProperty =
			UTextLayoutWidget::StaticClass()->FindPropertyByName(TEXT("Justification"));
		if (!TextBlock || !JustificationProperty)
		{
			return false;
		}

		const uint8* Value = JustificationProperty->ContainerPtrToValuePtr<uint8>(TextBlock);
		return Value && *Value == static_cast<uint8>(Expected);
	}

	bool IsLeftJustified(const UTextBlock* TextBlock)
	{
		return IsJustified(TextBlock, ETextJustify::Left);
	}

	bool IsCentreJustified(const UTextBlock* TextBlock)
	{
		return IsJustified(TextBlock, ETextJustify::Center);
	}
}

bool FTutorialOverlayLayoutContractTest::RunTest(const FString& Parameters)
{
	namespace Layout = BalhwajeomTutorialOverlayLayout;

	UWidgetTree* Tree = NewObject<UWidgetTree>(GetTransientPackage());
	Layout::FBoundWidgets Widgets;
	if (!TestTrue(TEXT("The layout should build into an empty tree"),
			Layout::Build(*Tree, Widgets)) ||
		!TestTrue(TEXT("The layout should produce every bound widget"), Widgets.IsComplete()))
	{
		return false;
	}

	TestFalse(TEXT("Building twice must not silently duplicate the tree"),
		Layout::Build(*Tree, Widgets));

	// These names are what BindWidgetOptional matches on, so a rename is a silent break.
	for (const TPair<const TCHAR*, UWidget*>& Bound : Widgets.AsNamedPairs())
	{
		TestTrue(
			FString::Printf(TEXT("%s should keep its bound name"), Bound.Key),
			Bound.Value && Bound.Value->GetFName() == FName(Bound.Key));
	}

	// The approved column order: images, heading, prose, prompt.
	const UPanelWidget* Column =
		Cast<UPanelWidget>(Tree->FindWidget(FName(Layout::WidgetNames::ContentColumn)));
	if (TestNotNull(TEXT("The content column should exist"), Column))
	{
		TestEqual(TEXT("The column holds exactly the four authored sections"),
			Column->GetChildrenCount(), 4);
		TestEqual(TEXT("The image row comes first"),
			Column->GetChildIndex(Widgets.ImageScale), 0);
		TestEqual(TEXT("The heading sits between the images and the prose"),
			Column->GetChildIndex(Widgets.Title), 1);
		TestEqual(TEXT("The prose follows the heading"),
			Column->GetChildIndex(Widgets.DescriptionBox), 2);
		TestEqual(TEXT("The prompt closes the column"),
			Column->GetChildIndex(Widgets.ContinuePrompt), 3);
	}

	TestTrue(TEXT("The heading is centred"), IsCentreJustified(Widgets.Title));

	// Left-aligned glyphs inside a block that is itself centred.
	TestTrue(TEXT("The description is left aligned"), IsLeftJustified(Widgets.Description));
	TestTrue(TEXT("The description wraps"), Widgets.Description->GetAutoWrapText());

	// MaxDesiredWidth rather than WidthOverride is what lets a short line stay centred.
	TestTrue(TEXT("The description box caps its width instead of forcing one"),
		Widgets.DescriptionBox->IsMaxDesiredWidthOverride());
	TestFalse(TEXT("The description box must not force a fixed width"),
		Widgets.DescriptionBox->IsWidthOverride());
	TestEqual(TEXT("The description wraps at the authored limit"),
		Widgets.DescriptionBox->GetMaxDesiredWidth(), Layout::DescriptionMaxWidth);

	// DownOnly, so a single small image is never blown up to fill the column.
	TestTrue(TEXT("A wide image row shrinks rather than clips"),
		Widgets.ImageScale->GetStretch() == EStretch::ScaleToFit);
	TestTrue(TEXT("The image row is never scaled up"),
		Widgets.ImageScale->GetStretchDirection() == EStretchDirection::DownOnly);

	TestTrue(TEXT("The prompt carries the fixed continue line"),
		!Widgets.ContinuePrompt->GetText().IsEmptyOrWhitespace());

	TestNotNull(TEXT("The background is blurred"),
		Tree->FindWidget(FName(Layout::WidgetNames::BackgroundBlur)));
	TestNotNull(TEXT("The background is dimmed"),
		Tree->FindWidget(FName(Layout::WidgetNames::Dim)));
	return true;
}

#endif
