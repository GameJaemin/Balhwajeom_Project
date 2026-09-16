#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCapturePhotoWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class URetainerBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWrapBox;
class UFont;
class UWidget;

/**
 * Runtime behaviour for WBP_CapturePhoto. Layout and sizing stay in the Widget
 * Blueprint; this class supplies capture data and the AE-authored presentation motion.
 */
UCLASS(Abstract, Blueprintable)
class BALHWAJEOM_API UBalhwajeomCapturePhotoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void PresentCapture(
		UTexture2D* CapturedTexture,
		const FText& SentenceText,
		const TArray<FText>& GrantedKeywords,
		bool bIsAnalysisSentence);

	void ApplyPresentationTimeline(float LinearAlpha);
	bool IsPresentationReady() const { return bPresentationReady; }
	void ResetPresentation();

	float GetAnimationDuration() const;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CapturedPhotoImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SentenceTextBlock;

	/** Read-only reconstruction of a photo-analysis sentence with visible blank boxes. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> SentenceBuilder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> KeywordList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CardRoot;

	/** Flattens the card background, captured photo, and sentence before animation. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URetainerBox> CardComposite;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SentenceBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ScreenDimmer;

	/** Extended presentation timeline: 93 frames at 60 fps. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.01", Units = "s"))
	float AnimationDuration = 1.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.01", Units = "s"))
	float CardEntryDuration = 0.4166667f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.0", Units = "s"))
	float FirstKeywordDelay = 0.0833333f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.0", Units = "s"))
	float KeywordStagger = 0.1666667f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.01", Units = "s"))
	float KeywordEntryDuration = 0.4166667f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.01", Units = "s"))
	float KeywordFadeInDuration = 0.1666667f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.0", Units = "s"))
	float ExitStartTime = 1.1333333f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutStartTime = 1.3833333f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation")
	FVector2D EntryOffset = FVector2D(318.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation")
	float EntryRotation = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation")
	FVector2D ExitOffset = FVector2D(0.0f, 360.0f);

	/** Extra local-space distance beyond the bottom edge before the presentation ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|AE Animation",
		meta = (ClampMin = "0.0"))
	float ExitSafetyMargin = 32.0f;

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

	/** When set, each keyword pill uses this texture instead of KeywordBackgroundColor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Keyword Style")
	TObjectPtr<UTexture2D> KeywordBackgroundTexture;

	/** Background used when the captured photo has a keyword-analysis sentence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Theme")
	TObjectPtr<UTexture2D> AnalysisBackgroundTexture;

	/** Background used when the captured photo only has a natural-language description. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Theme")
	TObjectPtr<UTexture2D> NaturalBackgroundTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Theme")
	FLinearColor AnalysisPhotoTint = FLinearColor::White;

	/** #E0D8C8, matching the supplied natural-language photo guide. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Theme")
	FLinearColor NaturalPhotoTint = FLinearColor::FromSRGBColor(FColor(0xE0, 0xD8, 0xC8));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Theme", meta = (ClampMin = "1"))
	int32 AnalysisSentenceFontSize = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Capture Photo|Theme", meta = (ClampMin = "1"))
	int32 NaturalSentenceFontSize = 25;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FCapturePhotoFlightTest;
	friend class FCapturePhotoMultilineSentenceTest;
#endif
	static float CalculateExitDistance(
		float ViewportHeight,
		float ContentTop,
		float MinimumDistance,
		float SafetyMargin);
	void UpdateResolvedExitDistance();
	void ApplyEntryTransform(UWidget* Widget, float EntryAlpha);

	bool bHasGrantedKeywords = false;
	bool bPresentationReady = false;
	float ResolvedExitDistanceY = 360.0f;
};
