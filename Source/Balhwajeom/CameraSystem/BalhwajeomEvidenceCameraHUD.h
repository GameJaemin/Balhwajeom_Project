// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BalhwajeomEvidenceCameraHUD.generated.h"

/** Minimal functional camera overlay for the MVP. */
UCLASS()
class BALHWAJEOM_API ABalhwajeomEvidenceCameraHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void TriggerPhotoFlash();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.01"))
	float PhotoFlashDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Guide", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float GuideCenterTransitionDuration = 0.25f;

private:
	float PhotoFlashEndTime = -1.0f;
	FVector2D DisplayedGuidePosition = FVector2D::ZeroVector;
	FVector2D GuideTransitionStartPosition = FVector2D::ZeroVector;
	float GuideTransitionElapsed = 0.0f;
	bool bHasDisplayedGuide = false;
	bool bPreviousGuideCentered = false;
};
