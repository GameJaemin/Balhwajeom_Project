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
