// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomCameraFocusModel.h"
#include "Components/ActorComponent.h"
#include "Engine/Scene.h"
#include "BalhwajeomPhotoCameraComponent.generated.h"

class UCameraComponent;
class UBalhwajeomInvestigationSubsystem;
class APhotoWorldStoryActor;
class UMaterialInstanceDynamic;
class UMaterialInterface;

enum class EBalhwajeomPhotoCaptureResult : uint8
{
    NoFocusedTarget,
    NotFramedEnough,
    NotCentered,
    InvalidTargetSnapshot,
    CaptureDisabled,
    AlreadyCaptured,
    CapturePending,
    ImageSaveFailed,
    RegistrationRejected,
    Succeeded
};

struct FBalhwajeomResolvedPhotoTarget
{
    FGuid EvidenceInstanceID;
    FName ObjectID = NAME_None;
    FName StateID = NAME_None;
    FName PhotoID = NAME_None;
    bool bCanCapture = false;
    float MinimumFocusDistanceOffset = 0.0f;
    float MaximumFocusDistanceOffset = 0.0f;
};

struct FBalhwajeomStrictFocusTarget
{
    TWeakObjectPtr<AActor> Target;
    FBalhwajeomCameraTargetInfo TargetInfo;
    FVector FocusLocation = FVector::ZeroVector;
    float FocusDistance = 0.0f;
};

struct FBalhwajeomPendingPhotoCapture
{
    FGuid RequestID;
    FBalhwajeomResolvedPhotoTarget TargetSnapshot;
    FDateTime RequestedTime;
    FString RelativePath;
    FString AbsolutePath;
    FTransform StorySpawnTransform = FTransform::Identity;
    bool bHasStorySpawnTransform = false;
};

DECLARE_MULTICAST_DELEGATE(FOnCameraModeExited);
DECLARE_MULTICAST_DELEGATE(FOnCameraTransitionFinished);

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomPhotoCameraComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    UBalhwajeomPhotoCameraComponent();

    virtual void BeginDestroy() override;

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    /** Broadcast right when camera mode fully exits, so an active FixedCameraZone can reclaim the view target. */
    FOnCameraModeExited OnCameraModeExited;

    /** Broadcast after the fade-out/switch/fade-in sequence has fully completed. */
    FOnCameraTransitionFinished OnCameraTransitionFinished;

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void SetPhotoCamera(UCameraComponent* Camera);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void SetNormalCamera(UCameraComponent* Camera);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void ToggleCameraMode();

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void RequestExitCameraMode();

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void LookYaw(float Value);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void LookPitch(float Value);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void PanHorizontal(float Value);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void PanVertical(float Value);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void ZoomCamera(float Value);

    UFUNCTION(BlueprintCallable, Category = "Photo Camera")
    void TakePhoto();

    UFUNCTION(BlueprintPure, Category = "Photo Camera")
    bool IsInCameraMode() const { return bIsInCameraMode; }

    UFUNCTION(BlueprintPure, Category = "Photo Camera")
    bool IsCameraTransitioning() const { return bIsCameraTransitioning; }

    /** Returns the currently focused target's screen guide and object-authored response. */
    UFUNCTION(BlueprintPure, Category = "Photo Camera|Focus")
    bool GetActiveFocusGuide(
        FVector2D& OutScreenPosition,
        bool& bOutIsCentered,
        FBalhwajeomCameraTargetInfo& OutTargetInfo,
        float& OutOpacity) const;

    UFUNCTION(BlueprintPure, Category = "Photo Camera|Focus")
    AActor* GetActiveFocusTarget() const { return ActiveFocusTarget.Get(); }

    /** Target that owns the currently rendered (possibly frozen) HUD guide. */
    UFUNCTION(BlueprintPure, Category = "Photo Camera|Focus")
    AActor* GetDisplayedFocusTarget() const { return DisplayedFocusTarget.Get(); }

    UFUNCTION(BlueprintCallable, Category = "Evidence", meta = (DeprecatedFunction, DeprecationMessage = "Use BalhwajeomInvestigationSubsystem.RegisterCapturedPhoto."))
    bool AddEvidence(const FBalhwajeomEvidenceData& NewEvidence);

    UFUNCTION(BlueprintPure, Category = "Evidence", meta = (DeprecatedFunction, DeprecationMessage = "Use BalhwajeomInvestigationSubsystem.HasCapturedPhoto."))
    bool HasEvidence(FName EvidenceID) const;

    UFUNCTION(BlueprintPure, Category = "Evidence", meta = (DeprecatedFunction, DeprecationMessage = "A captured-photo list API will be supplied by BalhwajeomInvestigationSubsystem."))
    TArray<FBalhwajeomEvidenceData> GetCollectedEvidence() const
    {
        return CollectedEvidence;
    }

protected:
    void EnterCameraMode();
    void ExitCameraMode();
    void SwitchCameraAtFadeOut();
    void FinishCameraTransition();
    void PanCamera(const FVector& ScreenDirection, float Value);
    void ShowPhotoFeedback(const FString& Message, const FColor& Color) const;
    void TriggerPhotoFlash() const;
    void UpdateEvidenceFocus(float DeltaTime);
    void RefreshDisplayedGuideSnapshot();
    bool IsDisplayedGuideSurfaceVisible() const;
    bool GetEffectiveCameraView(FVector& OutLocation, FVector& OutForward) const;
    bool TraceViewportCenter(FHitResult& OutHit) const;
    bool IsViewportCenterOverTarget(const AActor* Target) const;
    bool CalculateTargetFrameCoverage(const AActor* Target, float& OutCoverageRatio) const;
    bool FindStrictFocusTarget(FBalhwajeomStrictFocusTarget& OutTarget) const;
    void ApplyFocusBlur(float DeltaTime, const FBalhwajeomFocusRegion& DesiredRegion);
    void InitializeFocusBlurMaterials();
    void ResetEvidenceFocus();
    bool TryCaptureActiveFocusTarget();
    bool ResolveInvestigationTarget(
        const FBalhwajeomCameraTargetInfo& TargetInfo,
        FBalhwajeomResolvedPhotoTarget& OutTarget) const;
    UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;
    bool BeginInvestigationImageCapture(const FBalhwajeomResolvedPhotoTarget& Target);
    void HandleScreenshotCaptured(int32 Width, int32 Height, const TArray<FColor>& Colors);
    void HandleScreenshotProcessed();
    void CompleteImageSave(FGuid RequestID, bool bSucceeded, const FString& AbsolutePath);
    void ClearScreenshotDelegates();
    void SetWorldInspectionLabelsSuppressed(bool bSuppressed) const;
    bool CalculateStorySpawnTransform(FTransform& OutTransform) const;
    void StartPhotoWorldStory(
        const struct FPhotoDefinition& PhotoDefinition,
        const FTransform& SpawnTransform);

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> PhotoCamera;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> NormalCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bIsInCameraMode = false;

    /** Prevents repeated RMB input from restarting or reversing an active fade. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bIsCameraTransitioning = false;

    /** Enables the evidence focus and PhotoID capture flow. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus")
    bool bEnableEvidenceFocusSystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (ClampMin = "100.0", DeprecatedProperty, DeprecationMessage = "The exact center ray now uses world range; MinimumFocusDistance and MaximumFocusDistance own eligibility."))
    float FocusTargetScanDistance = 5000.0f;

    /** Global inclusive minimum camera-to-CameraFocusPoint distance for focus and capture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Distance", meta = (ClampMin = "0.0", Units = "cm"))
    float MinimumFocusDistance = 400.0f;

    /** Global inclusive maximum camera-to-CameraFocusPoint distance for focus and capture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Distance", meta = (ClampMin = "0.0", Units = "cm"))
    float MaximumFocusDistance = 1000.0f;

    /** Half-width of the sharp region on either side of a focused target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0", Units = "cm"))
    float BlurStartDistance = 100.0f;

    /** Distance over which blur eases quadratically from zero to MaximumBlurStrength. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0", Units = "cm"))
    float BlurTransitionDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaximumBlurStrength = 0.6f;

    /** Interpolation speed shared by focal distance, sharp range, and blur strength. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0"))
    float FocusApplicationSpeed = 8.0f;

    /** Visual-only target retention after the exact center ray loses its target. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus", meta = (ClampMin = "0.0", Units = "s"))
    float FocusTargetGracePeriod = 0.1f;

    /** Moves the edge guide slightly from the traced silhouette edge toward the authored focus point. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float YellowGuideInsetRatio = 0.12f;

    /** Coarse screen-space step used to find the visible complex-collision silhouette. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "1.0", ClampMax = "64.0"))
    float SilhouetteTracePixelStep = 12.0f;

    /** Per-target trace budget for one guide update. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "16", ClampMax = "2048"))
    int32 SilhouetteTraceMaxSamples = 384;

    /** The guide takes one tracking snapshot per cycle, then fades in and out. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FocusGuideTraceInterval = 1.0f;

    /** Minimum delay between event-driven rescans when the cached surface turns away or becomes occluded. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float GuideEarlyRescanCooldown = 0.15f;

    /** Allowed difference between the cached anchor and the first complex trace impact. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "0.1", ClampMax = "50.0"))
    float GuideVisibilityImpactTolerance = 5.0f;

    /** Surface normal must face the camera by at least this dot-product value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Silhouette", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float GuideFacingDotThreshold = 0.0f;

    /** Fraction of the target's projected framing bounds that must be inside the viewport. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (ClampMin = "0.0", ClampMax = "1.0", DeprecatedProperty, DeprecationMessage = "Capture no longer requires projected frame coverage."))
    float MinimumCaptureCoverageRatio = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (DeprecatedProperty, DeprecationMessage = "Use bEnableEvidenceFocusBlur."))
    bool bEnableEvidenceDepthOfField = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur")
    bool bEnableEvidenceFocusBlur = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
    TSoftObjectPtr<UMaterialInterface> FocusPrefilterMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
    TSoftObjectPtr<UMaterialInterface> FocusBlurMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
    TSoftObjectPtr<UMaterialInterface> FocusNearHorizontalMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
    TSoftObjectPtr<UMaterialInterface> FocusNearVerticalMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Evidence Focus|Blur")
    TSoftObjectPtr<UMaterialInterface> FocusCompositeMaterial;

    /** Full-resolution blur radius represented by strength 1.0 at 1080p. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0"))
    float MaximumBlurRadiusPixels = 12.0f;

    /** Foreground silhouette radius relative to MaximumBlurRadiusPixels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0"))
    float NearBlurRadiusScale = 1.25f;

    /** Background blur radius relative to MaximumBlurRadiusPixels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Blur", meta = (ClampMin = "0.0"))
    float FarBlurRadiusScale = 1.0f;

    /** Retained only so existing Blueprint instances keep their serialized value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy",
        meta = (ClampMin = "0.1", Units = "cm", DeprecatedProperty,
            DeprecationMessage = "The layered focus blur no longer preserves depth edges with bilateral rejection."))
    float DepthRejectionDistance = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "The unfocused sharp region now uses MinimumFocusDistance and MaximumFocusDistance."))
    float UnfocusedFocalDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (ClampMin = "0.1", ClampMax = "32.0", DeprecatedProperty, DeprecationMessage = "Focus blur now uses MaximumBlurStrength."))
    float EvidenceFocusFStop = 4.0f;

    /** Higher F-stop keeps the unfocused view readable instead of heavily blurred. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (ClampMin = "0.1", ClampMax = "32.0", DeprecatedProperty, DeprecationMessage = "Focus blur now uses MaximumBlurStrength."))
    float UnfocusedFStop = 5.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Legacy", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Use FocusApplicationSpeed."))
    float FocusInterpolationSpeed = 8.0f;

    /** Total fade-out + fade-in time for a camera mode change. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1"))
    float CameraTransitionDuration = 0.5f;

    /** Camera pan speed along the fixed entry Right axis and world Up axis, in cm/s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0"))
    float CameraPanSpeed = 80.0f;

    /** Half-size of the square pan area on both the horizontal and vertical axes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0"))
    float CameraPanMaxDistance = 150.0f;

    /** W/S begins slowing down when the absolute camera pitch reaches this angle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0", ClampMax = "89.0"))
    float VerticalPanSlowdownStartPitch = 45.0f;

    /** W/S is fully disabled at and beyond this absolute camera pitch. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0", ClampMax = "89.0"))
    float VerticalPanDisablePitch = 70.0f;

    /** Mouse-wheel zoom step in degrees of field of view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.1"))
    float CameraZoomStep = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "5.0", ClampMax = "170.0"))
    float MinCameraFieldOfView = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "5.0", ClampMax = "170.0"))
    float MaxCameraFieldOfView = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "100.0"))
    float PhotoTraceDistance = 3000.0f;

    /** World-space presentation spawned after a registered photo capture. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Photo Story")
    TSubclassOf<APhotoWorldStoryActor> PhotoWorldStoryClass;

    /** Depth along the deprojected lower-center screen ray, in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Photo Story",
        meta = (ClampMin = "10.0", UIMin = "10.0", Units = "cm"))
    float PhotoStoryDisplayDistance = 200.0f;

    /** Normalized viewport width at which the story initially appears. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Photo Story",
        meta = (ClampMin = "0.05", ClampMax = "0.95", UIMin = "0.05", UIMax = "0.95"))
    float PhotoStoryScreenXRatio = 0.5f;

    /** Normalized viewport height at which the story initially appears. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Photo Story",
        meta = (ClampMin = "0.05", ClampMax = "0.95", UIMin = "0.05", UIMax = "0.95"))
    float PhotoStoryScreenYRatio = 0.72f;

    /** Hook for a future SceneCapture/thumbnail record without changing the collection API. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
    TArray<FBalhwajeomEvidenceData> CollectedEvidence;

    FTimerHandle CameraSwitchTimerHandle;
    FTimerHandle CameraTransitionTimerHandle;
    FTransform SavedFirstPersonRelativeTransform;
    FVector CameraModeEntryWorldLocation = FVector::ZeroVector;
    FVector CameraPanWorldOffset = FVector::ZeroVector;
    FVector CameraPanRightDirection = FVector::RightVector;
    float SavedFirstPersonFieldOfView = 90.0f;

    FPostProcessSettings SavedPhotoPostProcessSettings;
    float SavedPostProcessBlendWeight = 1.0f;
    FBalhwajeomFocusRegion CurrentFocusRegion;
    float CurrentMaximumBlurStrength = 0.0f;
    FBalhwajeomFocusGraceState FocusGraceState;
    bool bHasStrictFocusTarget = false;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FocusPrefilterMaterialInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FocusBlurMaterialInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FocusNearHorizontalMaterialInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FocusNearVerticalMaterialInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FocusCompositeMaterialInstance;

    bool bFocusBlurInitializationFailed = false;
    float FocusGuideTraceElapsed = 0.0f;
    float EarlyGuideRescanElapsed = 0.0f;

    TWeakObjectPtr<AActor> ActiveFocusTarget;
    FBalhwajeomCameraTargetInfo ActiveFocusTargetInfo;
    FVector2D ActiveFocusScreenPosition = FVector2D::ZeroVector;
    bool bActiveFocusTargetCentered = false;
    bool bActiveFocusTargetFramedEnough = false;
    float ActiveFocusCoverageRatio = 0.0f;
    FVector ActiveFocusGuideLocalPosition = FVector::ZeroVector;
    FVector ActiveFocusGuideLocalNormal = FVector::ZeroVector;
    bool bActiveFocusGuideLocationValid = false;

    /** Frozen HUD snapshot. It is replaced only while the previous pulse is fully invisible. */
    TWeakObjectPtr<AActor> DisplayedFocusTarget;
    FBalhwajeomCameraTargetInfo DisplayedFocusTargetInfo;
    FVector DisplayedFocusGuideLocalPosition = FVector::ZeroVector;
    FVector DisplayedFocusGuideLocalNormal = FVector::ZeroVector;
    bool bDisplayedFocusGuideLocationValid = false;
    bool bDisplayedFocusGuideVisibilityValid = false;

    /** Captured state is per placed Actor, so copies sharing one EvidenceID remain independent. */
    TSet<TWeakObjectPtr<AActor>> CapturedFocusTargets;

    /** Temporary compatibility storage. New investigation captures never write to this array. */
    TOptional<FBalhwajeomPendingPhotoCapture> PendingCapture;
    FDelegateHandle ScreenshotCapturedHandle;
    FDelegateHandle ScreenshotProcessedHandle;
    bool bReceivedScreenshotPixels = false;
    TWeakObjectPtr<APhotoWorldStoryActor> ActivePhotoWorldStory;

private:

};
