#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMainMenuWidget.generated.h"

class UButton;
class UImage;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBalhwajeomStartRequested);

/** Runtime contract for the editable WBP_MainMenu designer asset. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Title")
	FBalhwajeomStartRequested OnStartRequested;

	UFUNCTION(BlueprintCallable, Category = "Title")
	void SetStartButtonEnabled(bool bEnabled);

	/** Assigns the looping title background video. Mirrors ABalhwajeomIntroFlowActor::SetIntroMediaAssets
	 * so the same headless editor-script pattern can wire these in. */
	UFUNCTION(BlueprintCallable, Category = "Title|Media")
	void SetBackgroundMediaAssets(UMediaSource* Source, UMediaPlayer* Player, UMediaTexture* Texture);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

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

private:
	UFUNCTION()
	void HandleStartClicked();

	void PlayBackgroundLoop();

	bool bStartAccepted = false;
};
