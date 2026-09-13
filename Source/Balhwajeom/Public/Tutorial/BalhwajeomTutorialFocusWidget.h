#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/BalhwajeomTutorialFlow.h"
#include "BalhwajeomTutorialFocusWidget.generated.h"


class UCanvasPanel;
class UImage;


/**
 * The tutorial dim and icon-highlight layer.
 *
 * It is added at ZOrder 5 by ABalhwajeomCameraPlayerController: above the player HUD
 * icons and the world, below WB_Interact. Dimming the screen therefore leaves the [F]
 * prompt and the centre dot fully readable, and the prompt is only shown while the
 * player is actually looking at a close, interactable target.
 *
 * The whole tree is built in C++, so no Widget Blueprint asset is required. A highlight
 * copies its brush and screen rectangle from the HUD icon it stands in for, which keeps
 * the two aligned no matter how the HUD is laid out. The blink itself is timed by
 * ABalhwajeomTutorialDirector, so the icon and the [F] prompt pulse together.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomTutorialFocusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomTutorialFocusWidget(const FObjectInitializer& ObjectInitializer);


protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * Candidate widget names for the photo camera icon inside the player HUD, tried in
	 * order. A list rather than a single name so renaming the HUD's images does not
	 * silently stop the highlight from appearing.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Focus")
	TArray<FName> PhotoCameraIconNames;

	/** Candidate widget names for the tablet icon inside the player HUD, tried in order. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Focus")
	TArray<FName> TabletIconNames;

	/** Interpolation speed of the dim. The prompt-following mode already fades with the prompt. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Focus", meta = (ClampMin = "0.1"))
	float DimInterpolationSpeed = 8.0f;

private:
	void BuildWidgetTree();

	/** Copies the source HUD icon's brush and screen rectangle onto Highlight. */
	void MirrorHudIcon(
		const FGeometry& MyGeometry,
		const TArray<FName>& SourceIconNames,
		UImage* Highlight,
		bool bShouldShow,
		float PulseOpacity);

	UUserWidget* GetPlayerHUD() const;

	/** First of the named widgets that actually exists in the player HUD, or null. */
	UImage* FindHudIcon(const TArray<FName>& SourceIconNames) const;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DimImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PhotoCameraHighlight;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TabletHighlight;

	float CurrentDimOpacity = 0.0f;
};
