#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomNotificationBannerWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
class UVerticalBox;


/**
 * One line of text across the top of the screen that fades itself away.
 *
 * Unlike the tutorial overlay this never stops the game and never takes input: it is a
 * nudge the player reads while still playing ("문이 열리는 소리가 난 것 같다.."), not a
 * screen they have to dismiss. The whole widget is HitTestInvisible for that reason.
 *
 * The layout is built in C++ the same way the tutorial overlay builds its own, so no
 * Widget Blueprint is required. The class stays Blueprintable, and a WBP whose widgets are
 * named to match the BindWidgetOptional members below takes over the look if one is made.
 */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomNotificationBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows Message, restarting the fade schedule from the beginning.
	 *
	 * A banner already on screen is replaced rather than queued behind: this is the one
	 * line at the top of the screen, and stacking would break what it is.
	 */
	void Show(const FText& Message);

	/** True until the schedule has run out, i.e. while the banner still has something to draw. */
	bool IsPlaying() const { return bPlaying; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SB_Banner;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_Body;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Message;

	/** Distance from the top of the screen to the banner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Layout",
		meta = (ClampMin = "0.0"))
	float TopMargin = 64.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Layout",
		meta = (ClampMin = "1.0"))
	float BannerWidth = 640.0f;

	/** Thickness of the white rules above and below the text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Layout",
		meta = (ClampMin = "0.0"))
	float EdgeLineThickness = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Layout",
		meta = (ClampMin = "8", ClampMax = "48"))
	int32 MessageFontSize = 20;

	/** The translucent plate behind the text. Alpha is the 30% the design calls for. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Style")
	FLinearColor BodyColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.3f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Style")
	FLinearColor EdgeLineColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeInSeconds = 0.25f;

	/** How long the line stays fully opaque, i.e. excluding the two fades around it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float HoldSeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutSeconds = 0.6f;

private:
	/** Builds the authored layout when this class is instantiated without a Widget Blueprint. */
	void EnsureFallbackLayout();

	float Elapsed = 0.0f;
	bool bPlaying = false;
};
