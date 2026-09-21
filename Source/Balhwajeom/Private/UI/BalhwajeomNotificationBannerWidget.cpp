#include "UI/BalhwajeomNotificationBannerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/BalhwajeomNotificationBanner.h"
#include "UI/BalhwajeomUIFonts.h"


void UBalhwajeomNotificationBannerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureFallbackLayout();

	// The banner appears while the player is still playing, so it must never sit between
	// them and the world they are clicking on.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.0f);
}

void UBalhwajeomNotificationBannerWidget::EnsureFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		// A Widget Blueprint already supplied a tree; its BindWidgetOptional members win.
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	SB_Banner = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SB_Banner"));
	SB_Banner->SetWidthOverride(BannerWidth);
	if (UCanvasPanelSlot* BannerSlot = RootCanvas->AddChildToCanvas(SB_Banner))
	{
		// Anchored to the top centre and sized to content, so the plate is only as tall as
		// the line of text inside it.
		BannerSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		BannerSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		BannerSlot->SetAutoSize(true);
		BannerSlot->SetPosition(FVector2D(0.0f, TopMargin));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("VB_Banner"));
	SB_Banner->SetContent(Column);

	// The rules are their own Borders rather than a border brush on the plate: that keeps
	// them exactly EdgeLineThickness tall and stops them wrapping around the sides.
	UBorder* TopLine = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("BRD_TopLine"));
	TopLine->SetBrushColor(EdgeLineColor);
	TopLine->SetPadding(FMargin(0.0f, EdgeLineThickness * 0.5f));
	Column->AddChildToVerticalBox(TopLine);

	BRD_Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BRD_Body"));
	BRD_Body->SetBrushColor(BodyColor);
	BRD_Body->SetPadding(FMargin(24.0f, 10.0f));
	Column->AddChildToVerticalBox(BRD_Body);

	TXT_Message = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TXT_Message"));
	TXT_Message->SetJustification(ETextJustify::Center);
	TXT_Message->SetFont(BalhwajeomUIFonts::MakeKoreanFont(TXT_Message->GetFont(), MessageFontSize));
	TXT_Message->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	BRD_Body->SetContent(TXT_Message);

	UBorder* BottomLine = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("BRD_BottomLine"));
	BottomLine->SetBrushColor(EdgeLineColor);
	BottomLine->SetPadding(FMargin(0.0f, EdgeLineThickness * 0.5f));
	Column->AddChildToVerticalBox(BottomLine);
}

void UBalhwajeomNotificationBannerWidget::Show(const FText& Message)
{
	if (TXT_Message)
	{
		TXT_Message->SetText(Message);
	}

	// Restarted rather than queued: a second notification replaces the line in place.
	Elapsed = 0.0f;
	bPlaying = true;
	SetRenderOpacity(0.0f);
}

void UBalhwajeomNotificationBannerWidget::NativeTick(
	const FGeometry& MyGeometry,
	const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPlaying)
	{
		return;
	}

	Elapsed += InDeltaTime;
	SetRenderOpacity(BalhwajeomNotificationBanner::ResolveOpacity(
		Elapsed, FadeInSeconds, HoldSeconds, FadeOutSeconds));

	if (BalhwajeomNotificationBanner::IsFinished(
		Elapsed, FadeInSeconds, HoldSeconds, FadeOutSeconds))
	{
		bPlaying = false;
		SetRenderOpacity(0.0f);
		// Left in the viewport rather than removed: the presenter reuses one banner, and a
		// fully transparent HitTestInvisible widget costs nothing to keep.
	}
}
