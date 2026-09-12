#include "ItemInspection/JMItemInspectionSurfaceWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

void UJMItemInspectionSurfaceWidgetBase::NativePreConstruct()
{
	BuildDefaultWidgetTreeIfNeeded();
	Super::NativePreConstruct();
	ApplySurfaceContent();
}

void UJMItemInspectionSurfaceWidgetBase::NativeConstruct()
{
	BuildDefaultWidgetTreeIfNeeded();
	Super::NativeConstruct();
	ApplySurfaceContent();
}

void UJMItemInspectionSurfaceWidgetBase::SetSurfaceContent(const FText& InContent)
{
	SurfaceContent = InContent;
	BuildDefaultWidgetTreeIfNeeded();
	ApplySurfaceContent();
	OnSurfaceContentChanged(InContent);
}

void UJMItemInspectionSurfaceWidgetBase::BuildDefaultWidgetTreeIfNeeded()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SurfaceRoot"));
	SurfaceContentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SurfaceContentText"));
	SurfaceContentText->SetAutoWrapText(true);
	SurfaceContentText->SetJustification(ETextJustify::Left);
	SurfaceContentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.035f, 0.025f, 0.018f, 1.0f)));
	FSlateFontInfo Font = SurfaceContentText->GetFont();
	Font.Size = 34;
	SurfaceContentText->SetFont(Font);

	if (UOverlaySlot* TextSlot = RootOverlay->AddChildToOverlay(SurfaceContentText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Fill);
		TextSlot->SetPadding(FMargin(72.0f, 80.0f));
	}
	WidgetTree->RootWidget = RootOverlay;
}

void UJMItemInspectionSurfaceWidgetBase::ApplySurfaceContent()
{
	if (SurfaceContentText)
	{
		SurfaceContentText->SetText(SurfaceContent);
	}
}
