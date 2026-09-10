// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceCameraHUD.h"

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
	Super::DrawHUD();

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

		FLinearColor GuideColor = FLinearColor::White;
		GuideColor.A = GuideOpacity;
		UBalhwajeomInvestigationSubsystem* InvestigationSubsystem =
			GetWorld() && GetWorld()->GetGameInstance()
				? GetWorld()->GetGameInstance()->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
				: nullptr;
		const bool bUsesInvestigationData =
			TargetInfo.EvidenceInstanceID.IsValid() &&
			!TargetInfo.ObjectID.IsNone() &&
			!TargetInfo.StateID.IsNone();

		FEvidenceDefinition EvidenceDefinition;
		FEvidenceStateDefinition StateDefinition;
		const bool bResolvedInvestigationDefinitions =
			bUsesInvestigationData &&
			InvestigationSubsystem &&
			InvestigationSubsystem->GetEvidenceDefinition(
				TargetInfo.ObjectID,
				EvidenceDefinition) &&
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

			float TextY = 95.0f;
			const float TextX = Canvas->ClipX - 420.0f;
			const FText DisplayName = bResolvedInvestigationDefinitions
				? EvidenceDefinition.ObjectName
				: TargetInfo.EvidenceData.EvidenceName;
			DrawText(
				DisplayName.ToString(),
				GuideColor,
				TextX,
				TextY,
				GEngine->GetMediumFont(),
				1.0f,
				false);
			TextY += 30.0f;

			if (!bResolvedInvestigationDefinitions)
			{
				for (const FText& InformationStage : TargetInfo.InformationStages)
				{
					DrawText(
						InformationStage.ToString(),
						FLinearColor::White,
						TextX,
						TextY,
						GEngine->GetSmallFont(),
						1.0f,
						false);
					TextY += 22.0f;
				}
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

	DrawEvidenceSavedAnimation();

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
	if (GetWorld())
	{
		EvidenceSavedAnimationStartTime = GetWorld()->GetTimeSeconds();
		EvidenceSavedAnimationName = EvidenceName.ToString();
	}
}

void ABalhwajeomEvidenceCameraHUD::DrawEvidenceSavedAnimation()
{
	if (!Canvas || !GetWorld() || EvidenceSavedAnimationStartTime < 0.0f)
	{
		return;
	}

	const float Duration = FMath::Max(EvidenceSavedAnimationDuration, KINDA_SMALL_NUMBER);
	const float Elapsed = GetWorld()->GetTimeSeconds() - EvidenceSavedAnimationStartTime;
	if (Elapsed >= Duration)
	{
		EvidenceSavedAnimationStartTime = -1.0f;
		EvidenceSavedAnimationName.Reset();
		return;
	}

	const float NormalizedTime = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	// Keep the freshly captured card in the center briefly, then ease it into the
	// lower-right gallery slot. The final fade makes the slot feel like it accepted it.
	constexpr float HoldRatio = 0.15f;
	const float MoveLinearAlpha = FMath::Clamp(
		(NormalizedTime - HoldRatio) / (1.0f - HoldRatio), 0.0f, 1.0f);
	const float MoveAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, MoveLinearAlpha, 2.0f);
	const float FadeAlpha = NormalizedTime > 0.82f
		? 1.0f - FMath::Clamp((NormalizedTime - 0.82f) / 0.18f, 0.0f, 1.0f)
		: 1.0f;

	const float StartWidth = FMath::Min(Canvas->ClipX * 0.46f, 560.0f);
	const FVector2D StartSize(StartWidth, StartWidth * 0.625f);
	const FVector2D EndSize(88.0f, 55.0f);
	const FVector2D StartCenter(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const FVector2D EndCenter(Canvas->ClipX - 88.0f, Canvas->ClipY - 78.0f);
	const FVector2D CardSize = FMath::Lerp(StartSize, EndSize, MoveAlpha);
	const FVector2D CardCenter = FMath::Lerp(StartCenter, EndCenter, MoveAlpha);
	const FVector2D CardTopLeft = CardCenter - CardSize * 0.5f;

	// Gallery destination stays visible behind the moving card during the animation.
	const FVector2D SlotTopLeft = EndCenter - EndSize * 0.5f - FVector2D(5.0f, 5.0f);
	DrawText(TEXT("GALLERY"), FLinearColor(0.75f, 0.85f, 0.85f, FadeAlpha),
		SlotTopLeft.X, SlotTopLeft.Y - 19.0f, GEngine->GetSmallFont(), 0.75f, false);
	DrawRect(FLinearColor(0.02f, 0.04f, 0.05f, 0.7f * FadeAlpha),
		SlotTopLeft.X, SlotTopLeft.Y, EndSize.X + 10.0f, EndSize.Y + 10.0f);

	const FLinearColor CardColor(0.035f, 0.055f, 0.065f, 0.94f * FadeAlpha);
	const FLinearColor BorderColor(0.25f, 1.0f, 0.45f, FadeAlpha);
	DrawRect(CardColor, CardTopLeft.X, CardTopLeft.Y, CardSize.X, CardSize.Y);
	DrawLine(CardTopLeft.X, CardTopLeft.Y, CardTopLeft.X + CardSize.X, CardTopLeft.Y, BorderColor, 2.0f);
	DrawLine(CardTopLeft.X + CardSize.X, CardTopLeft.Y,
		CardTopLeft.X + CardSize.X, CardTopLeft.Y + CardSize.Y, BorderColor, 2.0f);
	DrawLine(CardTopLeft.X + CardSize.X, CardTopLeft.Y + CardSize.Y,
		CardTopLeft.X, CardTopLeft.Y + CardSize.Y, BorderColor, 2.0f);
	DrawLine(CardTopLeft.X, CardTopLeft.Y + CardSize.Y, CardTopLeft.X, CardTopLeft.Y, BorderColor, 2.0f);

	if (MoveAlpha < 0.58f)
	{
		const float TextAlpha = (1.0f - MoveAlpha / 0.58f) * FadeAlpha;
		DrawText(TEXT("✓  EVIDENCE SAVED"), FLinearColor(0.25f, 1.0f, 0.45f, TextAlpha),
			CardTopLeft.X + 24.0f, CardTopLeft.Y + 22.0f, GEngine->GetMediumFont(), 1.0f, false);
		if (!EvidenceSavedAnimationName.IsEmpty())
		{
			DrawText(EvidenceSavedAnimationName, FLinearColor(1.0f, 1.0f, 1.0f, TextAlpha),
				CardTopLeft.X + 24.0f, CardTopLeft.Y + 58.0f, GEngine->GetSmallFont(), 1.0f, false);
		}
	}
}
