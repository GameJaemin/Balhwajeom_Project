#include "BalhwajeomCameraViewfinderWidget.h"

#include "BalhwajeomPhotoCameraZoom.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"


void UBalhwajeomCameraViewfinderWidget::ApplyZoomAlpha(
	const float ZoomAlpha,
	const float DeltaSeconds)
{
	const float TargetPosition = BalhwajeomPhotoCameraZoom::ResolveZoomBarPosition(
		ZoomAlpha, MinZoomBarPosition, MaxZoomBarPosition);

	if (!bHasDisplayedZoomBarPosition || ZoomBarInterpSpeed <= 0.0f)
	{
		DisplayedZoomBarPosition = TargetPosition;
		bHasDisplayedZoomBarPosition = true;
	}
	else
	{
		DisplayedZoomBarPosition = FMath::FInterpTo(
			DisplayedZoomBarPosition, TargetPosition, DeltaSeconds, ZoomBarInterpSpeed);
	}

	ApplyZoomBarPosition();
}


void UBalhwajeomCameraViewfinderWidget::ResetZoomBar()
{
	bHasDisplayedZoomBarPosition = false;
}


void UBalhwajeomCameraViewfinderWidget::ApplyZoomBarPosition()
{
	// Written onto the authored canvas slot rather than a render transform, so the values
	// in the details panel are the same numbers the designer reads in the Widget
	// Designer. A Bar parented to something other than a canvas simply does not move.
	UCanvasPanelSlot* BarSlot = Bar ? Cast<UCanvasPanelSlot>(Bar->Slot) : nullptr;
	if (!BarSlot)
	{
		return;
	}

	FVector2D Position = BarSlot->GetPosition();
	Position.Y = DisplayedZoomBarPosition;
	BarSlot->SetPosition(Position);
}
