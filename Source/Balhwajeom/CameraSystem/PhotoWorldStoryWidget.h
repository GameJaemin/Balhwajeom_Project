#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "PhotoWorldStoryWidget.generated.h"

class UTextBlock;

/** Minimal native fallback widget. A Widget Blueprint subclass can replace its visual styling. */
UCLASS(BlueprintType, Blueprintable)
class BALHWAJEOM_API UPhotoWorldStoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPhotoWorldStoryWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Photo Story")
	void SetStoryText(const FText& NewText);

	UFUNCTION(BlueprintPure, Category = "Photo Story|Visual")
	float GetThirdPersonFontSizeMultiplier() const { return ThirdPersonScaleMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Photo Story|Visual")
	float GetThirdPersonScaleTransitionDuration() const { return ThirdPersonScaleTransitionDuration; }

	int32 GetStoryFontSize() const;
	void SetStoryFontSize(int32 NewFontSize);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StoryText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual")
	FSlateFontInfo StoryFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual")
	FLinearColor StoryColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual")
	FLinearColor StoryShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual")
	FVector2D StoryShadowOffset = FVector2D(2.0f, 2.0f);

	/** Retained for existing assets. Automatic wrapping is disabled; only explicit newlines are used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual",
		meta = (ClampMin = "100.0", UIMin = "100.0"))
	float StoryWrapWidth = 720.0f;

	/** Multiplies the Designer StoryText font size after the player returns from photo mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual",
		meta = (ClampMin = "1.0", UIMin = "1.0", DisplayName = "Third Person Font Size Multiplier"))
	float ThirdPersonScaleMultiplier = 1.8f;

	/** Duration of the smooth scale-up when photo mode returns to third person. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s",
			DisplayName = "Third Person Font Size Transition Duration"))
	float ThirdPersonScaleTransitionDuration = 0.25f;
};
