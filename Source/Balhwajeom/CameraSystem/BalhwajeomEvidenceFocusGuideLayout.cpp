#include "CameraSystem/BalhwajeomEvidenceFocusGuideLayout.h"

#include "Components/Image.h"

namespace BalhwajeomEvidenceFocusGuideLayout
{
	bool ShouldUsePhotoRequiredIcon(const bool bCanCapture, const bool bAlreadyCaptured)
	{
		return bCanCapture && !bAlreadyCaptured;
	}

	FVector2D CalculateWidgetPosition(const FVector2D& GuidePosition)
	{
		return GuidePosition + FVector2D(-33.5, -25.0);
	}

	void ApplyStatusTexture(UImage* StatusImage, UTexture2D* StatusTexture)
	{
		if (StatusImage)
		{
			StatusImage->SetBrushFromTexture(StatusTexture, false);
		}
	}

	FText ResolveLabelText(const FText& AuthoredLabel, const bool bNeedsCloserView)
	{
		return bNeedsCloserView
			? NSLOCTEXT(
				"BalhwajeomCamera",
				"MoveCloserOrZoom",
				"조금 더 가까이 가거나 확대해 보자.")
			: AuthoredLabel;
	}

	FText ResolveSubLabelText(
		const bool bShowCenteredText,
		const bool bCanCapture,
		const FText& AuthoredCaptureBlockedLabel)
	{
		if (!bShowCenteredText || bCanCapture)
		{
			return FText::GetEmpty();
		}
		return !AuthoredCaptureBlockedLabel.IsEmptyOrWhitespace()
			? AuthoredCaptureBlockedLabel
			: NSLOCTEXT(
				"BalhwajeomCamera",
				"CaptureNotNeeded",
				"이건 굳이 사진으로 남기지 않아도 될 것 같다.");
	}
}
