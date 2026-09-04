// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomCameraTargetInterface.generated.h"

class UPrimitiveComponent;

UINTERFACE(BlueprintType)
class BALHWAJEOM_API UBalhwajeomCameraTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/** Request/response contract used by photo mode without depending on a concrete evidence Actor. */
class BALHWAJEOM_API IBalhwajeomCameraTargetInterface
{
	GENERATED_BODY()

public:
	/** Called while photo mode evaluates this object as a possible focus target. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera Target")
	bool RequestCameraTargetInfo(FBalhwajeomCameraTargetInfo& OutInfo) const;

	/** A designer-positioned point used for distance, visibility and screen-center tests. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera Target")
	FVector RequestCameraFocusLocation() const;

	/** Visible component whose projected bounds determine how much of the target is inside the photo frame. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera Target")
	UPrimitiveComponent* RequestCameraFramingComponent() const;

	/** Notification sent only after a focused, centered, valid photo succeeds. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera Target")
	void NotifyCameraCaptureSucceeded();
};
