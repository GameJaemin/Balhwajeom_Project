#include "Tutorial/BalhwajeomTutorialOverlayLayout.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"

namespace
{
	const TCHAR* RegularFontPath =
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-4Regular_Font.Freesentation-4Regular_Font");
	const TCHAR* SemiBoldFontPath =
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-6SemiBold_Font.Freesentation-6SemiBold_Font");

	/** Keeps the inherited font when the Korean face is missing rather than losing the size. */
	FSlateFontInfo MakeOverlayFont(
		const FSlateFontInfo& InheritedFont,
		int32 Size,
		const TCHAR* FontPath = RegularFontPath)
	{
		FSlateFontInfo Font = InheritedFont;
		Font.Size = Size;
		if (UFont* KoreanFont = LoadObject<UFont>(nullptr, FontPath))
		{
			Font.FontObject = KoreanFont;
		}
		return Font;
	}
}

namespace BalhwajeomTutorialOverlayLayout
{
TArray<TPair<const TCHAR*, UWidget*>> FBoundWidgets::AsNamedPairs() const
{
	return {
		{ WidgetNames::ImageScale, ImageScale },
		{ WidgetNames::ImageRow, ImageRow },
		{ WidgetNames::Title, Title },
		{ WidgetNames::DescriptionBox, DescriptionBox },
		{ WidgetNames::Description, Description },
		{ WidgetNames::ContinuePrompt, ContinuePrompt }
	};
}


bool Build(UWidgetTree& Tree, FBoundWidgets& OutWidgets)
{
	if (Tree.RootWidget)
	{
		return false;
	}

	UCanvasPanel* RootCanvas = Tree.ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), WidgetNames::RootCanvas);
	Tree.RootWidget = RootCanvas;

	UOverlay* RootOverlay = Tree.ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), WidgetNames::RootOverlay);
	UCanvasPanelSlot* RootOverlaySlot = RootCanvas->AddChildToCanvas(RootOverlay);
	RootOverlaySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	RootOverlaySlot->SetOffsets(FMargin(0.0f));

	// Blurs whatever the game already drew. Its own content stays empty so the dim above it
	// can be tuned without changing what gets blurred.
	UBackgroundBlur* Blur = Tree.ConstructWidget<UBackgroundBlur>(
		UBackgroundBlur::StaticClass(), WidgetNames::BackgroundBlur);
	Blur->SetBlurStrength(6.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// A Border rather than an Image: an Image with no texture is not a reliable solid fill.
	UBorder* Dim = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), WidgetNames::Dim);
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* ContentBox = Tree.ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), WidgetNames::ContentBox);
	ContentBox->SetWidthOverride(ContentWidth);
	if (UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ContentColumn = Tree.ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), WidgetNames::ContentColumn);
	ContentBox->SetContent(ContentColumn);

	// DownOnly so a wide row of images shrinks instead of clipping. The whole row scales by
	// one factor, which keeps every image the same height.
	UScaleBox* ImageScale = Tree.ConstructWidget<UScaleBox>(
		UScaleBox::StaticClass(), WidgetNames::ImageScale);
	ImageScale->SetStretch(EStretch::ScaleToFit);
	ImageScale->SetStretchDirection(EStretchDirection::DownOnly);
	if (UVerticalBoxSlot* ImageScaleSlot = ContentColumn->AddChildToVerticalBox(ImageScale))
	{
		ImageScaleSlot->SetHorizontalAlignment(HAlign_Center);
		ImageScaleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	UHorizontalBox* ImageRow = Tree.ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), WidgetNames::ImageRow);
	ImageScale->SetContent(ImageRow);

	UTextBlock* Title = Tree.ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), WidgetNames::Title);
	Title->SetJustification(ETextJustify::Center);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Title->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	Title->SetShadowOffset(FVector2D(2.0f, 2.0f));
	Title->SetFont(MakeOverlayFont(Title->GetFont(), 34, SemiBoldFontPath));
	if (UVerticalBoxSlot* TitleSlot = ContentColumn->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	// MaxDesiredWidth, not WidthOverride: a short line then keeps its own width and the
	// centred slot really centres it, while a long line still wraps at the same limit.
	USizeBox* DescriptionBox = Tree.ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), WidgetNames::DescriptionBox);
	DescriptionBox->SetMaxDesiredWidth(DescriptionMaxWidth);
	if (UVerticalBoxSlot* DescriptionSlot = ContentColumn->AddChildToVerticalBox(DescriptionBox))
	{
		DescriptionSlot->SetHorizontalAlignment(HAlign_Center);
		DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 40.0f));
	}

	UTextBlock* Description = Tree.ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), WidgetNames::Description);
	Description->SetJustification(ETextJustify::Left);
	Description->SetAutoWrapText(true);
	Description->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Description->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	Description->SetShadowOffset(FVector2D(2.0f, 2.0f));
	Description->SetFont(MakeOverlayFont(Description->GetFont(), 26));
	Description->SetLineHeightPercentage(1.25f);
	DescriptionBox->SetContent(Description);

	UTextBlock* ContinuePrompt = Tree.ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), WidgetNames::ContinuePrompt);
	ContinuePrompt->SetText(NSLOCTEXT(
		"BalhwajeomTutorialOverlay", "ContinuePrompt", "아무 키나 눌러 계속"));
	ContinuePrompt->SetJustification(ETextJustify::Center);
	ContinuePrompt->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.9f)));
	ContinuePrompt->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	ContinuePrompt->SetShadowOffset(FVector2D(2.0f, 2.0f));
	ContinuePrompt->SetFont(MakeOverlayFont(ContinuePrompt->GetFont(), 20));
	if (UVerticalBoxSlot* PromptSlot = ContentColumn->AddChildToVerticalBox(ContinuePrompt))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
	}

	OutWidgets.ImageScale = ImageScale;
	OutWidgets.ImageRow = ImageRow;
	OutWidgets.Title = Title;
	OutWidgets.DescriptionBox = DescriptionBox;
	OutWidgets.Description = Description;
	OutWidgets.ContinuePrompt = ContinuePrompt;
	return true;
}
}
