#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCameraViewfinderWidget.generated.h"

class UImage;


/**
 * Runtime behaviour for WBP_CAM. Layout stays in the Widget Blueprint; this class slides
 * the zoom bar to match the photo camera's current field of view.
 */
UCLASS(Abstract, Blueprintable)
class BALHWAJEOM_API UBalhwajeomCameraViewfinderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Eases the zoom bar toward the position for this zoom amount. The first call after
	 * the viewfinder appears places the bar outright, so raising the camera never shows
	 * the bar sliding over from the previous trip's zoom.
	 *
	 * @param ZoomAlpha      0 at the widest field of view, 1 at the narrowest.
	 * @param DeltaSeconds   Frame time for the ease.
	 */
	void ApplyZoomAlpha(float ZoomAlpha, float DeltaSeconds);

	/** Forgets the displayed position, so the next ApplyZoomAlpha() places the bar. */
	void ResetZoomBar();

protected:
	/** Slides vertically with the zoom. Optional: WBP_CAM works without it. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Bar;

	/** Bar's vertical slot position at the widest field of view, i.e. no zoom. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Viewfinder|Zoom Bar")
	float MinZoomBarPosition = 97.0f;

	/** Bar's vertical slot position at the narrowest field of view, i.e. full zoom. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Viewfinder|Zoom Bar")
	float MaxZoomBarPosition = -103.0f;

	/**
	 * How quickly the bar settles on its target; higher is faster, 0 snaps.
	 * The zoom itself moves in whole CameraZoomStep degrees, so this ease is what turns
	 * those steps into a continuous slide rather than a jump per wheel notch.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Viewfinder|Zoom Bar",
		meta = (ClampMin = "0.0"))
	float ZoomBarInterpSpeed = 12.0f;

private:
	void ApplyZoomBarPosition();

	float DisplayedZoomBarPosition = 0.0f;
	bool bHasDisplayedZoomBarPosition = false;
};
