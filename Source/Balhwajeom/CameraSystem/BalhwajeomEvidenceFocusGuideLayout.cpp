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
}
