// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BalhwajeomEvidenceCameraHUD.generated.h"

class UImage;
class UTexture2D;
class UTextBlock;
class UUserWidget;

/** Minimal functional camera overlay for the MVP. */
UCLASS()
class BALHWAJEOM_API ABalhwajeomEvidenceCameraHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABalhwajeomEvidenceCameraHUD();

	virtual void DrawHUD() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void TriggerPhotoFlash();

	/** Plays a short photo-card-to-gallery animation after a new evidence item is acquired. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Evidence")
	void TriggerEvidenceSavedAnimation(const FText& EvidenceName);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.01"))
	float PhotoFlashDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Guide", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float GuideCenterTransitionDuration = 0.25f;

	/** Total time for the captured photo card to settle into the gallery slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence", meta = (ClampMin = "0.2", ClampMax = "3.0"))
	float EvidenceSavedAnimationDuration = 0.8f;

private:
	void DrawEvidenceSavedAnimation();
	bool EnsureFocusGuideWidget();
	void HideFocusGuideWidget();
	void UpdateFocusGuideWidget(
		const FVector2D& GuidePosition,
		float GuideOpacity,
		bool bShowStatusIcon,
		bool bAlreadyCaptured,
		const FText& LabelText);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guide")
	TSubclassOf<UUserWidget> FocusGuideWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guide")
	TObjectPtr<UTexture2D> PhotoRequiredIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guide")
	TObjectPtr<UTexture2D> PhotoCapturedIcon;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> FocusGuideWidget;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FocusGuideStatusImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FocusGuideLabelText;

	float PhotoFlashEndTime = -1.0f;
	float EvidenceSavedAnimationStartTime = -1.0f;
	FString EvidenceSavedAnimationName;
	FVector2D DisplayedGuidePosition = FVector2D::ZeroVector;
	FVector2D GuideTransitionStartPosition = FVector2D::ZeroVector;
	float GuideTransitionElapsed = 0.0f;
	bool bHasDisplayedGuide = false;
	bool bPreviousGuideCentered = false;
};
