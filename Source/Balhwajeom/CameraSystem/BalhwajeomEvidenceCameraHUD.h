// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomCapturePhotoPresentationState.h"
#include "GameFramework/HUD.h"
#include "BalhwajeomEvidenceCameraHUD.generated.h"

class UImage;
class UMultiShadowTextWidget;
class UTexture2D;
class UUserWidget;
class UBalhwajeomCapturePhotoWidget;
class FCapturePhotoDismissInputProcessor;

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

	/** Hides every camera overlay while the clean screenshot frame is rendered. */
	void SetCaptureUIHiddenForScreenshot(bool bShouldHide);

	/** Plays a short photo-card-to-gallery animation after a new evidence item is acquired. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Evidence")
	void TriggerEvidenceSavedAnimation(const FText& EvidenceName);

	/** True from the moment the capture card appears until it has flown to TAB. */
	UFUNCTION(BlueprintPure, Category = "Camera|Evidence")
	bool IsCapturePhotoPresentationActive() const
	{
		return CapturePhotoPresentationState.IsActive();
	}

	/**
	 * True only while the card still needs the player to acknowledge it. The exit animation is
	 * deliberately excluded: once the card is committed to leaving there is no reason to keep
	 * refusing a camera-mode exit, and refusing it there cost the player a dead ~0.4s in which
	 * a TAB press did nothing.
	 */
	UFUNCTION(BlueprintPure, Category = "Camera|Evidence")
	bool IsCapturePhotoPresentationLockingCameraMode() const;

	/** Consumes the press while the capture result owns the screen. Returns whether it did. */
	bool HandleCapturePhotoDismissInput();

	/** Confirms only while the centered capture result is awaiting confirmation. */
	bool TryConfirmCapturePhotoPresentation();

	/** Shows the captured image, its sentence, and newly granted keywords before flying to TAB. */
	void TriggerCapturePhotoPresentation(
		UTexture2D* CapturedTexture,
		const FText& SentenceText,
		const TArray<FText>& GrantedKeywords,
		bool bIsAnalysisSentence = false);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.01"))
	float PhotoFlashDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Guide", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float GuideCenterTransitionDuration = 0.25f;

	/**
	 * Widget shown over the viewport while the camera is raised, i.e. the UI the player
	 * gets on right click. Leave unset to fall back to the plain drawn crosshair.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Viewfinder")
	TSubclassOf<UUserWidget> ViewfinderWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence",
		meta = (ClampMin = "0.01", Units = "s"))
	float CapturePhotoPromptFadeDuration = 0.1666667f;

private:
	/** Shows the viewfinder widget, or draws the fallback crosshair when none is set. */
	void UpdateViewfinder();
	bool EnsureViewfinderWidget();
	void HideViewfinderWidget();

	/** Full-screen shutter flash. Its clock only advances on frames it can draw. */
	void DrawPhotoFlash();

	void UpdateCapturePhotoPresentation();
	bool EnsureCapturePhotoWidget();
	bool EnsureCapturePhotoPromptWidget();
	void UpdateCapturePhotoPrompt(double Now);
	void FinishCapturePhotoPresentation();

	/** Slate-level press interception, live only while the card owns the screen. */
	void RegisterCapturePhotoDismissInput();
	void UnregisterCapturePhotoDismissInput();
	bool EnsureFocusGuideWidget();
	void HideFocusGuideWidget();
	void UpdateFocusGuideWidget(
		const FVector2D& GuidePosition,
		float GuideOpacity,
		bool bShowStatusIcon,
		bool bUsePhotoRequiredIcon,
		const FText& LabelText,
		const FText& SubLabelText);

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guide")
	TSubclassOf<UUserWidget> FocusGuideWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guide")
	TObjectPtr<UTexture2D> PhotoRequiredIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guide")
	TObjectPtr<UTexture2D> PhotoUnavailableIcon;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ViewfinderWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> FocusGuideWidget;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FocusGuideStatusImage;

	UPROPERTY(Transient)
	TObjectPtr<UMultiShadowTextWidget> FocusGuideLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UMultiShadowTextWidget> FocusGuideSubLabelText;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Evidence")
	TSubclassOf<UBalhwajeomCapturePhotoWidget> CapturePhotoWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomCapturePhotoWidget> CapturePhotoWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Evidence")
	TSubclassOf<UUserWidget> CapturePhotoPromptWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CapturePhotoPromptWidget;

	/** Shutter flash time left, in seconds. Frozen while the screenshot frame renders. */
	float PhotoFlashRemaining = 0.0f;
	float CapturePhotoLayoutWaitStartTime = -1.0f;
	FCapturePhotoPresentationState CapturePhotoPresentationState;
	TSharedPtr<FCapturePhotoDismissInputProcessor> CapturePhotoDismissInputProcessor;
	bool bCapturePhotoMovementLocked = false;
	bool bCaptureUIHiddenForScreenshot = false;
	FVector2D DisplayedGuidePosition = FVector2D::ZeroVector;
	FVector2D GuideTransitionStartPosition = FVector2D::ZeroVector;
	float GuideTransitionElapsed = 0.0f;
	bool bHasDisplayedGuide = false;
	bool bPreviousGuideCentered = false;
};
