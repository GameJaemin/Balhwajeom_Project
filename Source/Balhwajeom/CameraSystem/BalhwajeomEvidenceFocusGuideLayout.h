#pragma once

#include "CoreMinimal.h"

class UImage;
class UTexture2D;

namespace BalhwajeomEvidenceFocusGuideLayout
{
	/** Only a capture-enabled, not-yet-captured target uses the camera icon. */
	bool ShouldUsePhotoRequiredIcon(bool bCanCapture, bool bAlreadyCaptured);

	/** Returns the viewport position that places the center of the 67x50 icon on GuidePosition. */
	FVector2D CalculateWidgetPosition(const FVector2D& GuidePosition);

	/** Swaps the status texture without replacing the WBP-authored image size. */
	void ApplyStatusTexture(UImage* StatusImage, UTexture2D* StatusTexture);
}
