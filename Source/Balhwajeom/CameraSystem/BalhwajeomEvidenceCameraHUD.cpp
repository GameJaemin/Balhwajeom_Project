// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceCameraHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "BalhwajeomPhotoCameraComponent.h"
#include "GameFramework/Pawn.h"

void ABalhwajeomEvidenceCameraHUD::DrawHUD()
{
	Super::DrawHUD();

	const APawn* OwningPawn = PlayerOwner ? PlayerOwner->GetPawn() : nullptr;
	const UBalhwajeomPhotoCameraComponent* PhotoCamera =
		OwningPawn ? OwningPawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>() : nullptr;
	if (!Canvas || !PhotoCamera || !PhotoCamera->IsInCameraMode())
	{
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

		const FLinearColor YellowGuideColor(1.0f, 0.85f, 0.2f, 1.0f);
		const FLinearColor GreenGuideColor(0.25f, 1.0f, 0.35f, 1.0f);
		const FLinearColor BaseGuideColor = bGuideCentered
			? FMath::Lerp(YellowGuideColor, GreenGuideColor, TransitionAlpha)
			: FMath::Lerp(GreenGuideColor, YellowGuideColor, TransitionAlpha);
		FLinearColor GuideColor = BaseGuideColor;
		GuideColor.A = GuideOpacity;
		const float GuideSize = bGuideCentered ? 14.0f : 9.0f;
		DrawRect(
			GuideColor,
			DisplayedGuidePosition.X - GuideSize * 0.5f,
			DisplayedGuidePosition.Y - GuideSize * 0.5f,
			GuideSize,
			GuideSize);

		if (bGuideCentered)
		{
			float TextY = 95.0f;
			const float TextX = Canvas->ClipX - 420.0f;
			DrawText(
				TargetInfo.EvidenceData.EvidenceName.ToString(),
				BaseGuideColor,
				TextX,
				TextY,
				GEngine->GetMediumFont(),
				1.0f,
				false);
			TextY += 30.0f;

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
	else
	{
		bHasDisplayedGuide = false;
		GuideTransitionElapsed = 0.0f;
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

void ABalhwajeomEvidenceCameraHUD::TriggerPhotoFlash()
{
	if (GetWorld())
	{
		PhotoFlashEndTime = GetWorld()->GetTimeSeconds() + PhotoFlashDuration;
	}
}
