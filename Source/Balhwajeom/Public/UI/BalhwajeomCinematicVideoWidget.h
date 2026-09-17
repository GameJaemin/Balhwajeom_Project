#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCinematicVideoWidget.generated.h"

class UButton;
class UImage;
class UMediaTexture;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCinematicSkipRequestedSignature);

/** Full-screen surface used by the intro controller for pre-rendered movies. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomCinematicVideoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void SetMediaTexture(UMediaTexture* MediaTexture);

	/**
	 * Shows or hides BTN_Skip. The intro controller disables it the moment a skip is accepted so the
	 * control does not linger on screen while the fade-out plays over the still-visible movie.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void SetSkipEnabled(bool bEnabled);

	/** Broadcast when the viewer clicks BTN_Skip, if present in the WBP. */
	UPROPERTY(BlueprintAssignable, Category = "Cinematic Video")
	FCinematicSkipRequestedSignature OnSkipRequested;

	/** Fades RenderOpacity from its current value up to 1 over Duration seconds. Intended for a
	 * widget added at opacity 0 while its media source was still opening, so the first real frame
	 * eases in instead of popping to full opacity the instant playback starts. */
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void FadeIn(float Duration);

	/** Shows or hides VideoBackground, the opaque plate the WBP draws behind IMG_Video. Hidden for a
	 * clip that plays over something still on screen (the title-start ripple over the title screen),
	 * where that plate would black out everything the movie's own picture doesn't cover. */
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void SetBackgroundVisible(bool bVisible);

	/** Keeps the widget at 0 opacity until MediaTexture actually holds a decoded frame, then runs
	 * FadeIn(Duration). A media player reports its source as opened well before the first frame
	 * exists, and an empty UMediaTexture draws its (black) clear colour, so fading in on "opened"
	 * puts a black plate over whatever is still on screen for that gap. Gives up and fades in anyway
	 * after MaxWaitSeconds so a source that never produces picture cannot leave this stuck invisible. */
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void FadeInWhenMediaReady(UMediaTexture* MediaTexture, float Duration, float MaxWaitSeconds = 1.0f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Video;

	/** Optional. Add a button named BTN_Skip in the WBP to enable the skip control. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Skip;

	/** Optional. The opaque backing plate behind IMG_Video (named VideoBackground in the WBP), so
	 * SetBackgroundVisible can take it out of the way. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> VideoBackground;

private:
	UFUNCTION()
	void HandleSkipClicked();

	/** Set by FadeInWhenMediaReady; polled in NativeTick until the texture reports a frame. */
	TWeakObjectPtr<UMediaTexture> PendingRevealTexture;
	float PendingRevealDuration = 0.0f;
	float PendingRevealMaxWait = 0.0f;
	float PendingRevealWaited = 0.0f;
	bool bWaitingForMediaFrame = false;

	float FadeInStartOpacity = 1.0f;
	float FadeInDuration = 0.0f;
	float FadeInElapsed = 0.0f;
	bool bFadingIn = false;
};
