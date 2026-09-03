// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomCameraTargetInterface.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "BalhwajeomEvidenceActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UInspectionComponent;
class UWidgetComponent;

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

	/** Per-object distance thresholds and text used by the normal inspection system. */
	UFUNCTION(BlueprintPure, Category = "Inspection")
	UInspectionComponent* GetInspectionComponent() const { return InspectionComponent; }

	virtual bool RequestCameraTargetInfo_Implementation(FBalhwajeomCameraTargetInfo& OutInfo) const override;
	virtual FVector RequestCameraFocusLocation_Implementation() const override;
	virtual void NotifyCameraCaptureSucceeded_Implementation() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState);

	void SetInspectionLabel(const FText& LabelText, bool bVisible);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UStaticMeshComponent> EvidenceMesh;

	/**
	 * Makes every evidence actor discoverable by UPlayerInteractionComponent.
	 * Child Blueprints can author different distances and text on this inherited component.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UInspectionComponent> InspectionComponent;

	/** Screen-space label that follows this object in the normal third-person view. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UWidgetComponent> ObjectLabelWidget;

	/** Optional local offset from the evidence mesh's actual bounds center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|UI")
	FVector ObjectLabelOffset = FVector::ZeroVector;

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
