#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "BalhwajeomTutorialOverlayWidget.generated.h"

class UHorizontalBox;
class UScaleBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
struct FTutorialOverlayDefinition;

DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomTutorialOverlayCloseRequested);
DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomTutorialOverlayFadeOutFinished);


/**
 * The full-screen tutorial explanation layer.
 *
 * It dims and blurs the game, then centres one row of images, a heading, one block of
 * prose and a blinking "press any key" line. The row is built here rather than in the Widget Blueprint
 * because a DT_TutorialOverlay row carries 0..n images.
 *
 * The widget only draws and reports that the player wants it gone. Deciding when to show it,
 * blocking player input while it is up, and recording CompletionTag afterwards all belong to
 * whatever presents it.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTutorialOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomTutorialOverlayWidget(const FObjectInitializer& ObjectInitializer);

	/** Fills the overlay from one DT_TutorialOverlay row and restarts the dismiss guard. */
	void Present(const FTutorialOverlayDefinition& Definition);

	/** Fades the whole overlay in from transparent and starts the input lock. */
	void BeginFadeIn();

	/** Fades out, then broadcasts OnFadeOutFinished. Ignored while already fading out. */
	void BeginFadeOut();

	/**
	 * How long after BeginFadeIn a key press is ignored. The presenter owns this number
	 * because it also owns the Slate processor that swallows the presses.
	 */
	void SetInputLockSeconds(float Seconds);

	/** The tag the presenter records once the player has closed this overlay. */
	FGameplayTag GetCompletionTag() const { return CompletionTag; }

	/** False until MinimumVisibleSeconds has passed, so the press that opened it cannot close it. */
	bool CanAcceptDismissInput() const;

	/** Triangle wave between Min and Max. Linear on purpose - a sine reads as a slow pulse. */
	static float ComputeBlinkOpacity(float Elapsed, float Period, float MinOpacity, float MaxOpacity);

	/** Draw size that gives every image the same height while keeping its own aspect ratio. */
	static FVector2D ComputeUniformImageSize(const FIntPoint& SourceSize, float TargetHeight);

	FOnBalhwajeomTutorialOverlayCloseRequested OnCloseRequested;
	FOnBalhwajeomTutorialOverlayFadeOutFinished OnFadeOutFinished;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	/** Collapsed together with its row when a definition carries no usable image. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScaleBox> SCB_Images;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_Images;

	/** Collapsed when a definition carries no heading. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Title;

	/** Collapsed when a definition has no text, so the prompt does not float alone. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SB_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_ContinuePrompt;

	/** Height every image in the row is forced to. Widths follow from each source aspect ratio. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Images",
		meta = (ClampMin = "1.0"))
	float ImageHeight = 250.0f;

	/** Gap between neighbouring images. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Images",
		meta = (ClampMin = "0.0"))
	float ImageSpacing = 24.0f;

	/** Leave empty to keep the line authored in the Widget Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Prompt")
	FText ContinuePromptText;

	/** Seconds for one full dim-to-bright-to-dim cycle of the prompt. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Prompt",
		meta = (ClampMin = "0.05"))
	float PromptBlinkPeriod = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Prompt",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PromptMinOpacity = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Prompt",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PromptMaxOpacity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Input",
		meta = (ClampMin = "0.0"))
	float MinimumVisibleSeconds = 2.0f;

	/**
	 * Held blank before the fade in begins. Input is already blocked during it.
	 *
	 * Without the beat the screen lands on the exact frame the moment happens -- the
	 * player right-clicks and the explanation is already in front of them, which reads
	 * as the game interrupting rather than responding.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Fade",
		meta = (ClampMin = "0.0"))
	float ShowDelaySeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Fade",
		meta = (ClampMin = "0.0"))
	float FadeInSeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Fade",
		meta = (ClampMin = "0.0"))
	float FadeOutSeconds = 0.25f;

	/** How quickly the prompt appears once the input lock expires. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay|Prompt",
		meta = (ClampMin = "0.0"))
	float PromptRevealSeconds = 0.25f;

private:
	enum class EFadeState : uint8
	{
		Hidden,

		/** Blocking input, but still blank: the ShowDelaySeconds beat. */
		Delayed,
		FadingIn,
		Shown,
		FadingOut
	};

	/** Builds the authored layout when this class is instantiated without a Widget Blueprint. */
	void EnsureFallbackLayout();
	void TickFade(float InDeltaTime);
	void TickPrompt(float InDeltaTime);
	void RebuildImageRow(const TArray<TSoftObjectPtr<UTexture2D>>& Images);
	void ApplyTitle(const FText& OverlayTitle);
	void ApplyDescription(const FText& OverlayText);
	void ApplyContinuePrompt();
	void RequestClose();

	FGameplayTag CompletionTag;
	float VisibleSeconds = 0.0f;
	float BlinkElapsed = 0.0f;
	EFadeState FadeState = EFadeState::Hidden;
	float DelayRemaining = 0.0f;
	float FadeAlpha = 0.0f;
	float PromptAlpha = 0.0f;
};
