// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceCameraHUD.h"

#include "BalhwajeomCapturePhotoWidget.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "BalhwajeomPhotoCameraComponent.h"
#include "BalhwajeomCameraPlayerController.h"
#include "BalhwajeomEvidenceActor.h"
#include "BalhwajeomEvidenceFocusGuideLayout.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Interaction/InspectionComponent.h"
#include "GameFramework/Pawn.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "MultiShadowText.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomEvidenceCameraHUD::ABalhwajeomEvidenceCameraHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultViewfinderWidget(
		TEXT("/Game/Balhwajeom/UI/HUD/WBP_CAM"));
	if (DefaultViewfinderWidget.Succeeded())
	{
		ViewfinderWidgetClass = DefaultViewfinderWidget.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> FocusGuideWidgetAsset(
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_EvidenceFocusGuide"));
	if (FocusGuideWidgetAsset.Succeeded())
	{
		FocusGuideWidgetClass = FocusGuideWidgetAsset.Class;
	}

	static ConstructorHelpers::FClassFinder<UBalhwajeomCapturePhotoWidget> CapturePhotoWidgetAsset(
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_CapturePhoto"));
	if (CapturePhotoWidgetAsset.Succeeded())
	{
		CapturePhotoWidgetClass = CapturePhotoWidgetAsset.Class;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> PhotoRequiredIconAsset(
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoRequired.T_EvidencePhotoRequired"));
	if (PhotoRequiredIconAsset.Succeeded())
	{
		PhotoRequiredIcon = PhotoRequiredIconAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> PhotoUnavailableIconAsset(
		TEXT("/Game/Balhwajeom/UI/Icons/DotIcon.DotIcon"));
	if (PhotoUnavailableIconAsset.Succeeded())
	{
		PhotoUnavailableIcon = PhotoUnavailableIconAsset.Object;
	}
}

void ABalhwajeomEvidenceCameraHUD::DrawHUD()
{
	if (bCaptureUIHiddenForScreenshot)
	{
		HideFocusGuideWidget();
		// The saved photo must be the clean frame, without the camera frame over it.
		HideViewfinderWidget();
		return;
	}

	Super::DrawHUD();
	UpdateCapturePhotoPresentation();

	const APawn* OwningPawn = PlayerOwner ? PlayerOwner->GetPawn() : nullptr;
	const UBalhwajeomPhotoCameraComponent* PhotoCamera =
		OwningPawn ? OwningPawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>() : nullptr;
	if (!Canvas || !PhotoCamera || !PhotoCamera->IsInCameraMode())
	{
		HideFocusGuideWidget();
		HideViewfinderWidget();
		// A flash that is still running finishes even if the camera was lowered,
		// so it can never leak into a later frame.
		DrawPhotoFlash();
		return;
	}

	UpdateViewfinder();

	FVector2D GuidePosition;
	bool bGuideCentered = false;
	FBalhwajeomCameraTargetInfo TargetInfo;
	float GuideOpacity = 0.0f;
	if (PhotoCamera->GetActiveFocusGuide(GuidePosition, bGuideCentered, TargetInfo, GuideOpacity))
	{
		if (!bHasDisplayedGuide)
		{
			DisplayedGuidePosition = GuidePosition;
			GuideTransitionStartPosition = GuidePosition;
			GuideTransitionElapsed = GuideCenterTransitionDuration;
			bHasDisplayedGuide = true;
			bPreviousGuideCentered = bGuideCentered;
		}
		else if (bGuideCentered != bPreviousGuideCentered)
		{
			GuideTransitionStartPosition = DisplayedGuidePosition;
			GuideTransitionElapsed = 0.0f;
			bPreviousGuideCentered = bGuideCentered;
		}

		float TransitionAlpha = 1.0f;
		if (GuideTransitionElapsed < GuideCenterTransitionDuration)
		{
			GuideTransitionElapsed += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
			const float LinearAlpha = FMath::Clamp(
				GuideTransitionElapsed / FMath::Max(GuideCenterTransitionDuration, KINDA_SMALL_NUMBER),
				0.0f,
				1.0f);
			TransitionAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, LinearAlpha, 2.0f);
			DisplayedGuidePosition = FMath::Lerp(GuideTransitionStartPosition, GuidePosition, TransitionAlpha);
		}
		else
		{
			DisplayedGuidePosition = GuidePosition;
		}

		const bool bCenterTransitionFinished =
			GuideTransitionElapsed >= GuideCenterTransitionDuration;
		const bool bShowCenteredText = bGuideCentered && bCenterTransitionFinished;

		UBalhwajeomInvestigationSubsystem* InvestigationSubsystem =
			GetWorld() && GetWorld()->GetGameInstance()
				? GetWorld()->GetGameInstance()->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
				: nullptr;
		const bool bUsesInvestigationData =
			TargetInfo.EvidenceInstanceID.IsValid() &&
			!TargetInfo.ObjectID.IsNone() &&
			!TargetInfo.StateID.IsNone();

		FEvidenceStateDefinition StateDefinition;
		const bool bResolvedInvestigationDefinitions =
			bUsesInvestigationData &&
			InvestigationSubsystem &&
			InvestigationSubsystem->GetEvidenceStateDefinition(
				TargetInfo.StateID,
				StateDefinition) &&
			StateDefinition.ObjectID == TargetInfo.ObjectID;

		const ABalhwajeomEvidenceActor* DisplayedEvidence =
			Cast<ABalhwajeomEvidenceActor>(PhotoCamera->GetDisplayedFocusTarget());
		const bool bShowInvestigationStatusIcon = bResolvedInvestigationDefinitions;
		const bool bShowLegacyCaptureSymbol = !bUsesInvestigationData;
		const bool bShowStatusIcon =
			bShowInvestigationStatusIcon || bShowLegacyCaptureSymbol;
		bool bCanCapture = false;
		bool bAlreadyCaptured = false;
		if (bShowInvestigationStatusIcon)
		{
			bCanCapture = StateDefinition.bCanCapture && !StateDefinition.PhotoID.IsNone();
			bAlreadyCaptured = bCanCapture &&
				InvestigationSubsystem->HasCapturedPhoto(StateDefinition.PhotoID);
		}
		else if (bShowLegacyCaptureSymbol)
		{
			bCanCapture = TargetInfo.bCanBeCaptured;
			bAlreadyCaptured = DisplayedEvidence
				? DisplayedEvidence->GetEvidenceData().bAlreadyCollected
				: TargetInfo.EvidenceData.bAlreadyCollected;
		}
		const bool bUsePhotoRequiredIcon =
			BalhwajeomEvidenceFocusGuideLayout::ShouldUsePhotoRequiredIcon(
				bCanCapture,
				bAlreadyCaptured);

		FText NearLabelText;
		if (bShowCenteredText)
		{
			if (bResolvedInvestigationDefinitions)
			{
				NearLabelText = StateDefinition.NearLabel;
			}
			else
			{
				const UInspectionComponent* Inspection = DisplayedEvidence
					? DisplayedEvidence->GetInspectionComponent()
					: nullptr;
				NearLabelText = Inspection ? Inspection->NearLabel : FText::GetEmpty();
			}
		}

		UpdateFocusGuideWidget(
			DisplayedGuidePosition,
			GuideOpacity,
			bShowStatusIcon,
			bUsePhotoRequiredIcon,
			NearLabelText);
	}
	else
	{
		bHasDisplayedGuide = false;
		GuideTransitionElapsed = 0.0f;
		HideFocusGuideWidget();
	}

	DrawPhotoFlash();
}

void ABalhwajeomEvidenceCameraHUD::DrawPhotoFlash()
{
	if (PhotoFlashRemaining <= 0.0f)
	{
		return;
	}

	if (Canvas)
	{
		const float Alpha = FMath::Clamp(
			PhotoFlashRemaining / FMath::Max(PhotoFlashDuration, KINDA_SMALL_NUMBER),
			0.0f,
			1.0f);
		DrawRect(FLinearColor(1.0f, 1.0f, 1.0f, Alpha), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
	}

	// Only frames that could actually show the flash consume it. DrawHUD returns
	// before this while the clean screenshot frame renders, so the player still
	// gets the whole flash once the captured pixels have been copied.
	PhotoFlashRemaining = FMath::Max(
		PhotoFlashRemaining - (GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f),
		0.0f);
}

void ABalhwajeomEvidenceCameraHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishCapturePhotoPresentation();
	if (CapturePhotoWidget)
	{
		CapturePhotoWidget->RemoveFromParent();
	}
	CapturePhotoWidget = nullptr;

	if (ViewfinderWidget)
	{
		ViewfinderWidget->RemoveFromParent();
	}
	ViewfinderWidget = nullptr;

	if (FocusGuideWidget)
	{
		FocusGuideWidget->RemoveFromParent();
	}
	FocusGuideWidget = nullptr;
	FocusGuideStatusImage = nullptr;
	FocusGuideLabelText = nullptr;

	Super::EndPlay(EndPlayReason);
}

bool ABalhwajeomEvidenceCameraHUD::EnsureFocusGuideWidget()
{
	if (FocusGuideWidget)
	{
		return true;
	}
	if (!PlayerOwner || !FocusGuideWidgetClass)
	{
		return false;
	}

	FocusGuideWidget = CreateWidget<UUserWidget>(PlayerOwner, FocusGuideWidgetClass);
	if (!FocusGuideWidget)
	{
		return false;
	}

	FocusGuideStatusImage = Cast<UImage>(
		FocusGuideWidget->GetWidgetFromName(TEXT("UseCamera")));
	FocusGuideLabelText = Cast<UMultiShadowTextWidget>(
		FocusGuideWidget->GetWidgetFromName(TEXT("LabelText")));
	FocusGuideWidget->AddToViewport(100);
	FocusGuideWidget->SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

void ABalhwajeomEvidenceCameraHUD::HideFocusGuideWidget()
{
	if (FocusGuideWidget)
	{
		FocusGuideWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ABalhwajeomEvidenceCameraHUD::UpdateFocusGuideWidget(
	const FVector2D& GuidePosition,
	const float GuideOpacity,
	const bool bShowStatusIcon,
	const bool bUsePhotoRequiredIcon,
	const FText& LabelText)
{
	const bool bShowLabel = !LabelText.IsEmptyOrWhitespace();
	if ((!bShowStatusIcon && !bShowLabel) || !EnsureFocusGuideWidget())
	{
		HideFocusGuideWidget();
		return;
	}

	if (FocusGuideStatusImage)
	{
		UTexture2D* StatusTexture = bUsePhotoRequiredIcon
			? PhotoRequiredIcon.Get()
			: PhotoUnavailableIcon.Get();
		BalhwajeomEvidenceFocusGuideLayout::ApplyStatusTexture(
			FocusGuideStatusImage,
			StatusTexture);
		FocusGuideStatusImage->SetVisibility(
			bShowStatusIcon && StatusTexture
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (FocusGuideLabelText)
	{
		FocusGuideLabelText->SetText(LabelText);
		FocusGuideLabelText->SetVisibility(
			bShowLabel
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	// UseCamera is a 67x50 image at the left edge of the widget. Offset it so
	// the icon remains centered on the guide point while LabelText extends right.
	FocusGuideWidget->SetPositionInViewport(
		BalhwajeomEvidenceFocusGuideLayout::CalculateWidgetPosition(GuidePosition),
		true);
	FocusGuideWidget->SetRenderOpacity(FMath::Clamp(GuideOpacity, 0.0f, 1.0f));
	FocusGuideWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ABalhwajeomEvidenceCameraHUD::TriggerPhotoFlash()
{
	PhotoFlashRemaining = FMath::Max(PhotoFlashDuration, 0.0f);
}

void ABalhwajeomEvidenceCameraHUD::TriggerEvidenceSavedAnimation(const FText& EvidenceName)
{
	TriggerCapturePhotoPresentation(nullptr, EvidenceName, {});
}

bool ABalhwajeomEvidenceCameraHUD::EnsureViewfinderWidget()
{
	if (ViewfinderWidget)
	{
		return true;
	}
	if (!PlayerOwner || !ViewfinderWidgetClass)
	{
		return false;
	}

	ViewfinderWidget = CreateWidget<UUserWidget>(PlayerOwner, ViewfinderWidgetClass);
	if (!ViewfinderWidget)
	{
		return false;
	}

	// Above the world and the exploration HUD, below the focus guide (100) and the
	// capture card (250) so those keep reading on top of the frame.
	ViewfinderWidget->AddToViewport(50);
	ViewfinderWidget->SetVisibility(ESlateVisibility::Collapsed);
	return true;
}


void ABalhwajeomEvidenceCameraHUD::HideViewfinderWidget()
{
	if (ViewfinderWidget)
	{
		ViewfinderWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}


void ABalhwajeomEvidenceCameraHUD::UpdateViewfinder()
{
	if (EnsureViewfinderWidget())
	{
		ViewfinderWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	if (!Canvas)
	{
		return;
	}

	// Fallback for a project that has not assigned the widget yet: just a centre
	// crosshair, enough to aim with.
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const FLinearColor OverlayColor(0.2f, 0.9f, 0.8f, 0.9f);
	constexpr float Gap = 7.0f;
	constexpr float Length = 12.0f;

	DrawLine(CenterX - Gap - Length, CenterY, CenterX - Gap, CenterY, OverlayColor, 2.0f);
	DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Length, CenterY, OverlayColor, 2.0f);
	DrawLine(CenterX, CenterY - Gap - Length, CenterX, CenterY - Gap, OverlayColor, 2.0f);
	DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Length, OverlayColor, 2.0f);
}


void ABalhwajeomEvidenceCameraHUD::SetCaptureUIHiddenForScreenshot(const bool bShouldHide)
{
	bCaptureUIHiddenForScreenshot = bShouldHide;
	if (bShouldHide)
	{
		HideFocusGuideWidget();
	}
}

bool ABalhwajeomEvidenceCameraHUD::EnsureCapturePhotoWidget()
{
	if (CapturePhotoWidget)
	{
		return true;
	}
	if (!PlayerOwner || !CapturePhotoWidgetClass)
	{
		return false;
	}

	CapturePhotoWidget = CreateWidget<UBalhwajeomCapturePhotoWidget>(
		PlayerOwner, CapturePhotoWidgetClass);
	if (!CapturePhotoWidget)
	{
		return false;
	}
	CapturePhotoWidget->AddToViewport(250);
	CapturePhotoWidget->SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

void ABalhwajeomEvidenceCameraHUD::TriggerCapturePhotoPresentation(
	UTexture2D* CapturedTexture,
	const FText& SentenceText,
	const TArray<FText>& GrantedKeywords,
	const bool bIsAnalysisSentence)
{
	if (!GetWorld() || !EnsureCapturePhotoWidget())
	{
		return;
	}

	CapturePhotoWidget->PresentCapture(
		CapturedTexture, SentenceText, GrantedKeywords, bIsAnalysisSentence);
	EvidenceSavedAnimationStartTime = GetWorld()->GetTimeSeconds();
	CapturePhotoLayoutWaitStartTime = EvidenceSavedAnimationStartTime;
	if (PlayerOwner && !bCapturePhotoMovementLocked)
	{
		PlayerOwner->SetIgnoreMoveInput(true);
		bCapturePhotoMovementLocked = true;
	}
}

void ABalhwajeomEvidenceCameraHUD::UpdateCapturePhotoPresentation()
{
	if (!CapturePhotoWidget || !GetWorld() || EvidenceSavedAnimationStartTime < 0.0f)
	{
		return;
	}

	// Do not start the hold/flight clock until fresh Slate geometry is ready.
	if (!CapturePhotoWidget->IsFlightReady())
	{
		// A missing/collapsed marker must never send the photo elsewhere or
		// leave movement locked indefinitely. Cancel instead of guessing a target.
		if (GetWorld()->GetTimeSeconds() - CapturePhotoLayoutWaitStartTime > 2.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Capture photo cancelled: check WBP_CapturePhoto TabFlyTarget and photo/keyword layout."));
			FinishCapturePhotoPresentation();
			return;
		}
		EvidenceSavedAnimationStartTime = GetWorld()->GetTimeSeconds();
		return;
	}
	const float Elapsed = GetWorld()->GetTimeSeconds() - EvidenceSavedAnimationStartTime;
	const float HoldDuration = CapturePhotoWidget->GetHoldDuration();
	if (Elapsed < HoldDuration)
	{
		return;
	}
	const float FlyDuration = FMath::Max(CapturePhotoWidget->GetFlyDuration(), KINDA_SMALL_NUMBER);
	const float FlyAlpha = FMath::Clamp((Elapsed - HoldDuration) / FlyDuration, 0.0f, 1.0f);
	CapturePhotoWidget->ApplyFlyToTab(FlyAlpha);
	if (FlyAlpha >= 1.0f)
	{
		FinishCapturePhotoPresentation();
	}
}

void ABalhwajeomEvidenceCameraHUD::FinishCapturePhotoPresentation()
{
	EvidenceSavedAnimationStartTime = -1.0f;
	if (CapturePhotoWidget)
	{
		CapturePhotoWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PlayerOwner && bCapturePhotoMovementLocked)
	{
		PlayerOwner->SetIgnoreMoveInput(false);
	}
	bCapturePhotoMovementLocked = false;

	// Photo keywords are already present in the subsystem at capture time. Reveal
	// their new HUD count only now, when the card has visually arrived at TAB.
	if (ABalhwajeomCameraPlayerController* CameraController =
		Cast<ABalhwajeomCameraPlayerController>(PlayerOwner))
	{
		CameraController->FlushPendingPhotoKeywordCount();
	}
}
