#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMainMenuWidget.generated.h"

class UButton;
class UImage;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBalhwajeomStartRequested);

/** Runtime contract for the editable WBP_MainMenu designer asset. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Title")
	FBalhwajeomStartRequested OnStartRequested;

	/** Played the moment BTN_Start is clicked, before OnStartRequested fires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	TSoftObjectPtr<USoundBase> ButtonClickSound;

	UFUNCTION(BlueprintCallable, Category = "Title")
	void SetStartButtonEnabled(bool bEnabled);

	/** Assigns the looping title background video. Mirrors ABalhwajeomIntroFlowActor::SetIntroMediaAssets
	 * so the same headless editor-script pattern can wire these in. */
	UFUNCTION(BlueprintCallable, Category = "Title|Media")
	void SetBackgroundMediaAssets(UMediaSource* Source, UMediaPlayer* Player, UMediaTexture* Texture);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Start;

	/** Full-screen image behind BTN_Start; receives BackgroundMediaTexture as its brush resource. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Background;

	/** Looping title-screen background video. All three must be valid for playback to start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Media")
	TObjectPtr<UMediaSource> BackgroundMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Media")
	TObjectPtr<UMediaPlayer> BackgroundMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Media")
	TObjectPtr<UMediaTexture> BackgroundMediaTexture;

	/** BTN_Start's opacity while the cursor is away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title|Hover",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartIdleOpacity = 1.0f;

	/** BTN_Start's opacity while it is hovered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title|Hover",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartHoverOpacity = 0.5f;

	/** Seconds for a complete fade between the two. Zero snaps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Title|Hover",
		meta = (ClampMin = "0.0", Units = "s"))
	float StartHoverFadeDuration = 0.18f;

private:
	UFUNCTION()
	void HandleStartClicked();

	void PlayBackgroundLoop();

	/**
	 * Makes every button state draw the Normal brush. The designer asset authored a
	 * separate Hovered texture, and Slate swapping it on the hover edge is what read as
	 * flickering; the eased opacity in NativeTick is the feedback now.
	 */
	void UnifyStartButtonStates();

	bool bStartAccepted = false;

	/** 0 while the cursor is away, 1 while hovered; eased before it becomes opacity. */
	float StartHoverAlpha = 0.0f;
};
