#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCapturePhotoWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UFont;
class UWidget;

/**
 * Runtime behaviour for WBP_CapturePhoto.  Layout and sizing stay in the Widget
 * Blueprint; this class only supplies capture data and the fly-to-TAB motion.
 */
UCLASS(Abstract, Blueprintable)
class BALHWAJEOM_API UBalhwajeomCapturePhotoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void PresentCapture(
		UTexture2D* CapturedTexture,
		const FText& SentenceText,
		const TArray<FText>& GrantedKeywords);

	void ApplyFlyToTab(float LinearAlpha);
	bool IsFlightReady() const { return bAnimationOriginsCached; }
	void ResetPresentation();

	float GetHoldDuration() const { return HoldDuration; }
	float GetFlyDuration() const;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CapturedPhotoImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SentenceTextBlock;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> KeywordList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CardRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SentenceBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ScreenDimmer;

	/** Designer-authored destination marker inside WBP_CapturePhoto. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> TabFlyTarget;

	/** Time the full-size photo information remains readable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Timing",
		meta = (DisplayName = "Photo Info Display Duration", ClampMin = "0.0", UIMin = "0.0", UIMax = "10.0", Units = "s",
			ToolTip = "How long the full-size captured photo, sentence, and keywords remain on screen before flying to TAB."))
	float HoldDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Timing",
		meta = (DisplayName = "Photo Fly To Tab Duration", ClampMin = "0.05", UIMin = "0.05", UIMax = "3.0", Units = "s",
			ToolTip = "How long the captured image takes to shrink and move into the right-side TAB HUD."))
	float FlyDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Timing",
		meta = (DisplayName = "Keyword Follow Delay", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", Units = "s",
			ToolTip = "Delay after the photo reaches TAB before acquired keywords begin following it."))
	float KeywordFollowDelay = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Timing",
		meta = (DisplayName = "Keyword Fly To Tab Duration", ClampMin = "0.05", UIMin = "0.05", UIMax = "3.0", Units = "s",
			ToolTip = "How long the acquired keyword list takes to follow the photo into TAB."))
	float KeywordFlyDuration = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Animation", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TabTargetScale = 0.12f;

	/** Optional font asset used by every dynamically generated keyword label. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Keyword Style")
	TObjectPtr<UFont> KeywordFontAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Keyword Style",
		meta = (ClampMin = "1", UIMin = "8", UIMax = "72"))
	int32 KeywordFontSize = 25;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Keyword Style")
	FLinearColor KeywordTextColor = FLinearColor::Black;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Keyword Style")
	FLinearColor KeywordBackgroundColor = FLinearColor(0.96f, 0.96f, 0.96f, 1.0f);

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FCapturePhotoFlightTest;
#endif
	void ApplyWidgetFly(UWidget* Widget, const FVector2D& LocalTravel, float Alpha);

	bool bHasGrantedKeywords = false;
	bool bAnimationOriginsCached = false;
	int32 LayoutTicksRemaining = 2;
	FVector2D PhotoLocalTravel = FVector2D::ZeroVector;
	FVector2D KeywordLocalTravel = FVector2D::ZeroVector;
};
