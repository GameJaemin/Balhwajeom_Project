// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomCameraTargetInterface.h"
#include "BalhwajeomEvidenceActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/** A simple Blueprint-placeable object that can be discovered with the camera trace. */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomEvidenceActor : public AActor, public IBalhwajeomCameraTargetInterface
{
	GENERATED_BODY()

public:
	ABalhwajeomEvidenceActor();

	UFUNCTION(BlueprintPure, Category = "Evidence")
	FBalhwajeomEvidenceData GetEvidenceData() const { return EvidenceData; }

	UFUNCTION(BlueprintCallable, Category = "Evidence")
	void MarkAsCollected();

	virtual bool RequestCameraTargetInfo_Implementation(FBalhwajeomCameraTargetInfo& OutInfo) const override;
	virtual FVector RequestCameraFocusLocation_Implementation() const override;
	virtual void NotifyCameraCaptureSucceeded_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UStaticMeshComponent> EvidenceMesh;

	/** Move this point in a derived Blueprint to choose the precise focus/guide location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Target")
	TObjectPtr<USceneComponent> CameraFocusPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FBalhwajeomEvidenceData EvidenceData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	TArray<FText> CameraInformationStages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	bool bCanBeCaptured = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (ClampMin = "1.0"))
	float PreferredFocusDistanceAt1x = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (ClampMin = "1.0"))
	float FocusDistanceToleranceAt1x = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus")
	bool bScaleFocusDistanceWithZoom = true;
};
