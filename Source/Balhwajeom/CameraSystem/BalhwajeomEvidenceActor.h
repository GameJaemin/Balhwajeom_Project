// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomCameraTargetInterface.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "BalhwajeomEvidenceActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UInspectionComponent;
class UTexture2D;
class UWidgetComponent;
class UBalhwajeomInvestigationSubsystem;
struct FEvidenceActorTestAccessor;

/** A simple Blueprint-placeable object that can be discovered with the camera trace. */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomEvidenceActor : public AActor, public IBalhwajeomCameraTargetInterface
{
	GENERATED_BODY()

	friend struct FEvidenceActorTestAccessor;

public:
	ABalhwajeomEvidenceActor();

	UFUNCTION(BlueprintPure, Category = "Evidence")
	FBalhwajeomEvidenceData GetEvidenceData() const { return EvidenceData; }

	UFUNCTION(BlueprintCallable, Category = "Evidence")
	void MarkAsCollected();

	UFUNCTION(BlueprintCallable, Category = "Evidence|Investigation")
	void ConfigureInvestigationObject(FName InObjectID);

	UFUNCTION(BlueprintPure, Category = "Evidence|Investigation")
	FName GetObjectID() const { return ObjectID; }

	/** Executes the current F-interaction. Single-choice keyword documents award their word immediately. */
	UFUNCTION(BlueprintCallable, Category = "Evidence|Investigation")
	bool RequestInvestigationInteraction(FText& OutDisplayText);

	/** Hides the normal distance label while the dedicated photo camera HUD is active. */
	void SetInspectionLabelSuppressed(bool bSuppressed);

	/** Builds the label text; capture status is presented by the adjacent icon. */
	static FText FormatInspectionLabel(
		EPlayerInspectionDistanceState DistanceState,
		const FText& LabelText);

	/** Whether the normal inspection widget should stay visible for this distance state. */
	static bool ShouldDisplayInspectionLabel(
		EPlayerInspectionDistanceState DistanceState,
		const FText& LabelText);

	/** Per-object distance thresholds and text used by the normal inspection system. */
	UFUNCTION(BlueprintPure, Category = "Inspection")
	UInspectionComponent* GetInspectionComponent() const { return InspectionComponent; }

	virtual bool RequestCameraTargetInfo_Implementation(FBalhwajeomCameraTargetInfo& OutInfo) const override;
	virtual FVector RequestCameraFocusLocation_Implementation() const override;
	virtual UPrimitiveComponent* RequestCameraFramingComponent_Implementation() const override;
	virtual void NotifyCameraCaptureSucceeded_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleEvidenceStateChanged(FGuid ChangedInstanceID, FName PreviousStateID, FName NewStateID);

	UFUNCTION()
	void HandlePhotoCaptured(const FCapturedPhotoRecord& PhotoRecord);

	UFUNCTION()
	void HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState);

	void SetInspectionLabel(const FText& LabelText, bool bVisible);
	void ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState);
	void RegisterWithInvestigationSystem();
	void ApplyInvestigationState(FName StateID);
	UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UStaticMeshComponent> EvidenceMesh;

	/**
	 * Query-only volume used by photo focus and interaction traces.
	 * This keeps target recognition working when a designer swaps the visible mesh
	 * for an asset with missing or disabled complex collision.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Target")
	TObjectPtr<UBoxComponent> CameraTargetBounds;

	/**
	 * Makes every evidence actor discoverable by UPlayerInteractionComponent.
	 * Child Blueprints can author different distances and text on this inherited component.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UInspectionComponent> InspectionComponent;

	/** Screen-space label that follows this object in the normal third-person view. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UWidgetComponent> ObjectLabelWidget;

	/** Status icon used until this evidence has been photographed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inspection|UI")
	TObjectPtr<UTexture2D> PhotoRequiredIcon;

	/** Status icon used after this evidence has been photographed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inspection|UI")
	TObjectPtr<UTexture2D> PhotoCapturedIcon;

	/** Optional local offset from the evidence mesh's actual bounds center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|UI")
	FVector ObjectLabelOffset = FVector::ZeroVector;

	EPlayerInspectionDistanceState LastInspectionDistanceState =
		EPlayerInspectionDistanceState::OutOfRange;
	bool bInspectionLabelSuppressed = false;

	/** Move this point in a derived Blueprint to choose the precise focus/guide location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Target")
	TObjectPtr<USceneComponent> CameraFocusPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FBalhwajeomEvidenceData EvidenceData;

	/** Row ID in DT_EvidenceDefinitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence|Investigation")
	FName ObjectID = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Investigation")
	FGuid EvidenceInstanceID;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Investigation")
	FName CurrentStateID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	TArray<FText> CameraInformationStages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target")
	bool bCanBeCaptured = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (Units = "cm"))
	float MinimumFocusDistanceOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Focus", meta = (Units = "cm"))
	float MaximumFocusDistanceOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Legacy", meta = (ClampMin = "1.0", DeprecatedProperty, DeprecationMessage = "Focus distance is now owned by the photo camera."))
	float PreferredFocusDistanceAt1x = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Legacy", meta = (ClampMin = "1.0", DeprecatedProperty, DeprecationMessage = "Focus distance is now owned by the photo camera."))
	float FocusDistanceToleranceAt1x = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Target|Legacy", meta = (DeprecatedProperty, DeprecationMessage = "Zoom no longer changes focus or capture distance."))
	bool bScaleFocusDistanceWithZoom = true;
};
