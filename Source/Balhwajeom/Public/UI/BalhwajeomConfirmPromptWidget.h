#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomConfirmPromptWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomConfirmPromptConfirmed);
DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomConfirmPromptCancelled);
DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomConfirmPromptFadeOutFinished);


/**
 * Full-screen yes/no question, dimmed and blurred like the tutorial overlay.
 *
 * Where the tutorial overlay closes on any key, this one waits for a mouse click on one of
 * two buttons -- the presenter turns the cursor on for it. It only draws and reports which
 * button was pressed; what "yes" means belongs to whoever put it on screen.
 *
 * The layout is built in C++ the same way the tutorial overlay builds its own, so no Widget
 * Blueprint is required. The class stays Blueprintable, and a WBP whose widgets are named to
 * match the BindWidgetOptional members below takes over the look if one is made.
 */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomConfirmPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills the two lines of text and fades the whole screen in from transparent. */
	void Present(const FText& InTitle, const FText& InMessage);

	/** Fades out, then broadcasts OnFadeOutFinished. Ignored while already fading out. */
	void BeginFadeOut();

	/** Broadcast the moment the button is pressed, before the fade out finishes. */
	FOnBalhwajeomConfirmPromptConfirmed OnConfirmed;
	FOnBalhwajeomConfirmPromptCancelled OnCancelled;
	FOnBalhwajeomConfirmPromptFadeOutFinished OnFadeOutFinished;

	/** False until the fade in has finished, so a click cannot land on a half-drawn screen. */
	bool CanAcceptInput() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Message;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Confirm;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Cancel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Confirm Prompt|Text")
	FText ConfirmLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Confirm Prompt|Text")
	FText CancelLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Confirm Prompt|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeInSeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Confirm Prompt|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutSeconds = 0.25f;

private:
	enum class EFadeState : uint8
	{
		Hidden,
		FadingIn,
		Shown,
		FadingOut
	};

	/** Builds the authored layout when this class is instantiated without a Widget Blueprint. */
	void EnsureFallbackLayout();

	EFadeState FadeState = EFadeState::Hidden;
	float FadeAlpha = 0.0f;
	bool bResultSent = false;
};
