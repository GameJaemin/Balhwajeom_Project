#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCinematicVideoWidget.generated.h"

class UButton;
class UImage;
class UMediaTexture;

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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Video;

	/** Optional. Add a button named BTN_Skip in the WBP to enable the skip control. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Skip;

private:
	UFUNCTION()
	void HandleSkipClicked();

	float FadeInStartOpacity = 1.0f;
	float FadeInDuration = 0.0f;
	float FadeInElapsed = 0.0f;
	bool bFadingIn = false;
};
