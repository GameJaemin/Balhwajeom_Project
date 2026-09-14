// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomCameraTargetInterface.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "BalhwajeomEvidenceActor.generated.h"

class APhotoWorldStoryActor;
class UArrowComponent;
class UNiagaraComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UInspectionComponent;
class UJMInspectableComponent;
class UJMItemInspectionData;
class UTexture2D;
class UWidgetComponent;
class UBalhwajeomInvestigationSubsystem;
class UStoryStateSubsystem;
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

	/** Runtime identity of this placed copy, which is what the investigation subsystem keys on. */
	UFUNCTION(BlueprintPure, Category = "Evidence|Investigation")
	FGuid GetEvidenceInstanceID() const { return EvidenceInstanceID; }

	UFUNCTION(BlueprintPure, Category = "Evidence|Investigation")
	FName GetCurrentStateID() const { return CurrentStateID; }

	/** Executes the current F-interaction. Single-choice keyword documents award their word immediately. */
	UFUNCTION(BlueprintCallable, Category = "Evidence|Investigation")
	bool RequestInvestigationInteraction(FText& OutDisplayText);

	/** Read-only availability check used by the player's interaction prompt. */
	UFUNCTION(BlueprintPure, Category = "Evidence|Investigation")
	bool CanRequestInvestigationInteraction() const;

	/** False while the evidence is waiting for its data-authored story progression tag. */
	UFUNCTION(BlueprintPure, Category = "Evidence|Progression")
	bool IsProgressionAvailable() const { return bProgressionAvailable; }

	/** Completes a progression removal. A future Blueprint animation override calls this at its end. */
	UFUNCTION(BlueprintCallable, Category = "Evidence|Progression")
	void FinalizeProgressionRemoval();

	/**
	 * Plays the current state's photo story as world-locked 3D text at StoryAnchor.
	 * Interaction triggers this automatically for states whose presentation is WorldStory;
	 * it is exposed so a Blueprint or a debug key can replay the same presentation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Evidence|Story")
	bool PlayWorldStory();

	/** Stops this object's story early, for example when the player walks away. */
	UFUNCTION(BlueprintCallable, Category = "Evidence|Story")
	void StopWorldStory();

	/** Final spawn transform for the story text. Unit scale, with the optional yaw override applied. */
	UFUNCTION(BlueprintPure, Category = "Evidence|Story")
	FTransform GetWorldStoryTransform() const;

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

	UFUNCTION(BlueprintPure, Category = "Inspection|3D")
	UJMInspectableComponent* GetItemInspectionComponent() const { return ItemInspectionComponent; }

	/**
	 * Fires whenever a state is applied, which is the place to swap the visible object, start a
	 * Niagara system, or play a sound for that state.
	 * bInitialApply is true when BeginPlay restores an already-advanced state, so one-shot effects
	 * should be skipped in that case or they replay every time the level loads.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Evidence|Investigation")
	void OnEvidenceStateApplied(FName PreviousStateID, FName NewStateID, bool bInitialApply);

	virtual bool RequestCameraTargetInfo_Implementation(FBalhwajeomCameraTargetInfo& OutInfo) const override;
	virtual FVector RequestCameraFocusLocation_Implementation() const override;
	virtual UPrimitiveComponent* RequestCameraFramingComponent_Implementation() const override;
	virtual void NotifyCameraCaptureSucceeded_Implementation() override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Resizes CameraTargetBounds to match EvidenceMesh's current static mesh bounds. Called from
	 * OnConstruction (so it's correct in the editor right after placing/converting an actor, e.g.
	 * via "Replace Selected Actors With") and again from BeginPlay as a safety net. */
	void FitCameraTargetBoundsToMesh();

	/** Keeps the floating status icon centred after a state swaps the mesh for a different size. */
	void UpdateObjectLabelPlacement();

	/** Applies the state's mesh and plays its one-shot effect. Only the mesh is applied on a load. */
	void ApplyStateVisuals(const struct FEvidenceStateDefinition& State, bool bInitialApply);

	UFUNCTION()
	void HandleEvidenceStateChanged(FGuid ChangedInstanceID, FName PreviousStateID, FName NewStateID);

	UFUNCTION()
	void HandlePhotoCaptured(const FCapturedPhotoRecord& PhotoRecord);

	UFUNCTION()
	void HandlePhotoGalleryReset();

	UFUNCTION()
	void HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState);

	UFUNCTION()
	void HandleStoryStateTagChanged(FGameplayTag StateTag);

	/** Defaults to immediate removal; a Blueprint can override it to play an animation first. */
	UFUNCTION(BlueprintNativeEvent, Category = "Evidence|Progression")
	void BeginProgressionRemoval();
	virtual void BeginProgressionRemoval_Implementation();

	bool PlayWorldStoryForState(FName StateID);
	/** Warns only when the current state actually asks for a WorldStory, so ordinary states stay quiet. */
	void LogBlockedWorldStory(const TCHAR* Reason) const;
	void SetInspectionLabel(const FText& LabelText, bool bVisible);
	void ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState);
	bool CanClearForProgression() const;
	void RefreshProgressionAvailability();
	void RefreshProgressionClearedState();
	void RegisterWithInvestigationSystem();
	void ApplyInvestigationState(FName StateID, bool bInitialApply = false);
	void ConfigureItemInspection();
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

	/** Enables the rotating SceneCapture inspector for this Evidence Actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection|3D")
	bool bEnable3DInspection = false;

	/** Optional authored settings. Missing mesh/text fields are filled from this Evidence Actor at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection|3D", meta = (EditCondition = "bEnable3DInspection"))
	TObjectPtr<UJMItemInspectionData> ItemInspectionData;

	/** Runtime bridge consumed by the existing F-key interaction trace. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection|3D")
	TObjectPtr<UJMInspectableComponent> ItemInspectionComponent;

	/**
	 * Whether the level or Blueprint ticked ItemInspectionComponent.bInspectionEnabled by hand.
	 * That flag is derived state every ConfigureItemInspection() rewrites, so it is captured
	 * once at BeginPlay and treated as the same opt-in bEnable3DInspection gives.
	 */
	bool bAuthoredItemInspectionEnabled = false;

	UPROPERTY(Transient)
	TObjectPtr<UJMItemInspectionData> RuntimeItemInspectionData;

	/** Screen-space label that follows this object in the normal third-person view. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inspection")
	TObjectPtr<UWidgetComponent> ObjectLabelWidget;

	/** Status icon used until this evidence has been photographed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inspection|UI")
	TObjectPtr<UTexture2D> PhotoRequiredIcon;

	/** Status icon used after this evidence has been photographed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inspection|UI")
	TObjectPtr<UTexture2D> PhotoCapturedIcon;

	/** Status icon used when the current evidence state cannot be photographed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inspection|UI")
	TObjectPtr<UTexture2D> PhotoUnavailableIcon;

	/** Optional local offset from the evidence mesh's actual bounds center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|UI")
	FVector ObjectLabelOffset = FVector::ZeroVector;

	EPlayerInspectionDistanceState LastInspectionDistanceState =
		EPlayerInspectionDistanceState::OutOfRange;
	bool bInspectionLabelSuppressed = false;

	/** Loaded from DT_EvidenceDefinitions; empty means this object is always available. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Progression")
	FGameplayTag RequiredActivationTag;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Progression")
	bool bProgressionAvailable = true;

	/** Completion condition that changes this object's normal F interaction into removal. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Progression")
	FGameplayTag ClearRequiredTag;

	/** Added after removal, normally unlocking the next phase's evidence actors. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Progression")
	FGameplayTag GrantedTagOnClear;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Evidence|Progression")
	bool bProgressionCleared = false;

	bool bProgressionRemovalPending = false;
	bool bActorBaselineHidden = false;
	bool bActorBaselineCollisionEnabled = true;

	ECollisionEnabled::Type CameraTargetBoundsBaselineCollisionEnabled =
		ECollisionEnabled::QueryOnly;
	ECollisionResponse CameraTargetBoundsBaselineVisibilityResponse = ECR_Block;
	bool bCameraTargetBoundsBaselineCaptured = false;

	/** Move this point in a derived Blueprint to choose the precise focus/guide location. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Target")
	TObjectPtr<USceneComponent> CameraFocusPoint;

	/**
	 * Where the world-locked 3D story text appears and which way it faces. Move and rotate this
	 * in a derived Blueprint or directly on the placed instance; its +X axis is the reading
	 * direction, so point the arrow at where the player will be standing.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence|Story")
	TObjectPtr<USceneComponent> StoryAnchor;

#if WITH_EDITORONLY_DATA
	/** Editor-only reading-direction indicator for StoryAnchor. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> StoryAnchorArrow;
#endif

	/** Presentation actor spawned at StoryAnchor. The native class is used when this is empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evidence|Story")
	TSubclassOf<APhotoWorldStoryActor> StoryActorClass;

	/** Replaces StoryAnchor's authored yaw with one facing the player. Pitch and roll stay authored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence|Story")
	bool bStoryFacesPlayer = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<APhotoWorldStoryActor> ActiveWorldStory;

	/** Weak because a finished one-shot system destroys its own component. */
	TWeakObjectPtr<UNiagaraComponent> ActiveStateEffect;

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
