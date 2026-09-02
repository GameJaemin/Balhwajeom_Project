// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.generated.h"

/** Designer-authored data stored when an evidence photo succeeds. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FBalhwajeomEvidenceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FName EvidenceID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FText EvidenceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
	bool bAlreadyCollected = false;
};

/** Information returned by any object that can be inspected through photo mode. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FBalhwajeomCameraTargetInfo
{
	GENERATED_BODY()

	/** Evidence payload stored when a correctly focused and centered photo succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	FBalhwajeomEvidenceData EvidenceData;

	/** Object-authored lines shown only while the target is focused and centered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	TArray<FText> InformationStages;

	/** Allows informational targets that can be focused but should not be collected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	bool bCanBeCaptured = true;

	/** Preferred camera-to-target distance at 1x zoom, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (ClampMin = "1.0"))
	float PreferredFocusDistanceAt1x = 700.0f;

	/** Accepted distance on either side of the preferred distance at 1x zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (ClampMin = "1.0"))
	float FocusDistanceToleranceAt1x = 300.0f;

	/** When true, zooming in moves the accepted focus band farther away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus")
	bool bScaleFocusDistanceWithZoom = true;
};
