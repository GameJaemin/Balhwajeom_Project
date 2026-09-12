// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceCameraHUD.h"

#include "BalhwajeomCapturePhotoWidget.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "BalhwajeomPhotoCameraComponent.h"
#include "BalhwajeomEvidenceActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Interaction/InspectionComponent.h"
#include "GameFramework/Pawn.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomEvidenceCameraHUD::ABalhwajeomEvidenceCameraHUD()
{
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

	static ConstructorHelpers::FObjectFinder<UTexture2D> PhotoCapturedIconAsset(
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoCaptured.T_EvidencePhotoCaptured"));
	if (PhotoCapturedIconAsset.Succeeded())
	{
		PhotoCapturedIcon = PhotoCapturedIconAsset.Object;
	}
}

void ABalhwajeomEvidenceCameraHUD::DrawHUD()
{
	if (bCaptureUIHiddenForScreenshot)
	{
		HideFocusGuideWidget();
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
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const FLinearColor OverlayColor(0.2f, 0.9f, 0.8f, 0.9f);
	constexpr float Gap = 7.0f;
	constexpr float Length = 12.0f;

	DrawLine(CenterX - Gap - Length, CenterY, CenterX - Gap, CenterY, OverlayColor, 2.0f);
	DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Length, CenterY, OverlayColor, 2.0f);
	DrawLine(CenterX, CenterY - Gap - Length, CenterX, CenterY - Gap, OverlayColor, 2.0f);
	DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Length, OverlayColor, 2.0f);

	DrawLine(30.0f, 30.0f, Canvas->ClipX - 30.0f, 30.0f, OverlayColor, 1.0f);
	DrawLine(Canvas->ClipX - 30.0f, 30.0f, Canvas->ClipX - 30.0f, Canvas->ClipY - 30.0f, OverlayColor, 1.0f);
	DrawLine(Canvas->ClipX - 30.0f, Canvas->ClipY - 30.0f, 30.0f, Canvas->ClipY - 30.0f, OverlayColor, 1.0f);
	DrawLine(30.0f, Canvas->ClipY - 30.0f, 30.0f, 30.0f, OverlayColor, 1.0f);

	DrawText(TEXT("SMARTPHONE CAMERA  |  LMB: TAKE PHOTO"), OverlayColor,
		45.0f, 42.0f, GEngine->GetSmallFont(), 1.0f, false);
	DrawText(TEXT("WASD: PAN  |  WHEEL: ZOOM  |  RMB / ESC: EXIT"), OverlayColor,
		45.0f, 62.0f, GEngine->GetSmallFont(), 1.0f, false);

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
		const bool bShowInvestigationCaptureSymbol =
			bResolvedInvestigationDefinitions &&
			StateDefinition.bCanCapture &&
			!StateDefinition.PhotoID.IsNone();
		const bool bShowLegacyCaptureSymbol = !bUsesInvestigationData;
		const bool bShowStatusIcon =
			bShowInvestigationCaptureSymbol || bShowLegacyCaptureSymbol;
		bool bAlreadyCaptured = false;
		if (bShowStatusIcon)
		{
			bAlreadyCaptured = bShowInvestigationCaptureSymbol
				? InvestigationSubsystem->HasCapturedPhoto(StateDefinition.PhotoID)
				: DisplayedEvidence && DisplayedEvidence->GetEvidenceData().bAlreadyCollected;
		}

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
			bAlreadyCaptured,
			NearLabelText);
	}
	else
	{
		bHasDisplayedGuide = false;
		GuideTransitionElapsed = 0.0f;
		HideFocusGuideWidget();
	}

	if (PhotoFlashEndTime > 0.0f && GetWorld())
	{
		const float Remaining = PhotoFlashEndTime - GetWorld()->GetTimeSeconds();
		if (Remaining > 0.0f)
		{
			const float Alpha = FMath::Clamp(Remaining / PhotoFlashDuration, 0.0f, 1.0f);
			DrawRect(FLinearColor(1.0f, 1.0f, 1.0f, Alpha), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
		}
	}
}

void ABalhwajeomEvidenceCameraHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishCapturePhotoPresentation();
	if (CapturePhotoWidget)
	{
		CapturePhotoWidget->RemoveFromParent();
	}
	CapturePhotoWidget = nullptr;

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
	FocusGuideLabelText = Cast<UTextBlock>(
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
	const bool bAlreadyCaptured,
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
		UTexture2D* StatusTexture = bAlreadyCaptured
			? PhotoCapturedIcon.Get()
			: PhotoRequiredIcon.Get();
		FocusGuideStatusImage->SetBrushFromTexture(StatusTexture, true);
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

	// UseCamera is a 50x50 image at the left edge of the widget. Offset it so
	// the icon remains centered on the guide point while LabelText extends right.
	FocusGuideWidget->SetPositionInViewport(
		GuidePosition + FVector2D(-25.0f, -25.0f),
		true);
	FocusGuideWidget->SetRenderOpacity(FMath::Clamp(GuideOpacity, 0.0f, 1.0f));
	FocusGuideWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ABalhwajeomEvidenceCameraHUD::TriggerPhotoFlash()
{
	if (GetWorld())
	{
		PhotoFlashEndTime = GetWorld()->GetTimeSeconds() + PhotoFlashDuration;
	}
}

void ABalhwajeomEvidenceCameraHUD::TriggerEvidenceSavedAnimation(const FText& EvidenceName)
{
	TriggerCapturePhotoPresentation(nullptr, EvidenceName, {});
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
	const TArray<FText>& GrantedKeywords)
{
	if (!GetWorld() || !EnsureCapturePhotoWidget())
	{
		return;
	}

	CapturePhotoWidget->PresentCapture(CapturedTexture, SentenceText, GrantedKeywords);
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
}
