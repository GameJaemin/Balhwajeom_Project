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

	/** Stable identity of the placed evidence Actor registered with the investigation subsystem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation")
	FGuid EvidenceInstanceID;

	/** Evidence definition ID for the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation")
	FName ObjectID = NAME_None;

	/** Current evidence state ID at the time this snapshot was requested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation")
	FName StateID = NAME_None;

	/** Photo awarded by the current state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation")
	FName PhotoID = NAME_None;

	/** Whether the current state permits a photo capture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation")
	bool bCanCapture = false;

	/** Added to the photo camera's global minimum focus/capture distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation", meta = (Units = "cm"))
	float MinimumFocusDistanceOffset = 0.0f;

	/** Added to the photo camera's global maximum focus/capture distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Investigation", meta = (Units = "cm"))
	float MaximumFocusDistanceOffset = 0.0f;

	/** Preferred camera-to-target distance supplied by the current state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Legacy", meta = (ClampMin = "1.0", DeprecatedProperty, DeprecationMessage = "Focus distance is now owned by the photo camera."))
	float PreferredFocusDistance = 700.0f;

	/** Accepted distance on either side of PreferredFocusDistance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Legacy", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Focus distance is now owned by the photo camera."))
	float FocusDistanceTolerance = 300.0f;

	/** Evidence payload stored when a correctly focused and centered photo succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target", meta = (DeprecatedProperty, DeprecationMessage = "Use the investigation ID and state fields instead."))
	FBalhwajeomEvidenceData EvidenceData;

	/** Object-authored lines shown only while the target is focused and centered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target", meta = (DeprecatedProperty, DeprecationMessage = "Use EvidenceStateDefinition.NearLabel instead."))
	TArray<FText> InformationStages;

	/** Allows informational targets that can be focused but should not be collected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target", meta = (DeprecatedProperty, DeprecationMessage = "Use bCanCapture instead."))
	bool bCanBeCaptured = true;

	/** Preferred camera-to-target distance at 1x zoom, in centimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (ClampMin = "1.0", DeprecatedProperty, DeprecationMessage = "Use PreferredFocusDistance instead."))
	float PreferredFocusDistanceAt1x = 700.0f;

	/** Accepted distance on either side of the preferred distance at 1x zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (ClampMin = "1.0", DeprecatedProperty, DeprecationMessage = "Use FocusDistanceTolerance instead."))
	float FocusDistanceToleranceAt1x = 300.0f;

	/** When true, zooming in moves the accepted focus band farther away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Legacy", meta = (DeprecatedProperty, DeprecationMessage = "Zoom no longer changes focus or capture distance."))
	bool bScaleFocusDistanceWithZoom = true;
};
