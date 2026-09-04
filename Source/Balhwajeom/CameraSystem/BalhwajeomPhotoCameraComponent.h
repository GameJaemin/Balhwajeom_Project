// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.h"
#include "Components/ActorComponent.h"
#include "Engine/Scene.h"
#include "BalhwajeomPhotoCameraComponent.generated.h"

class UCameraComponent;

DECLARE_MULTICAST_DELEGATE(FOnCameraModeExited);
DECLARE_MULTICAST_DELEGATE(FOnCameraTransitionFinished);

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomPhotoCameraComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    UBalhwajeomPhotoCameraComponent();

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

    UFUNCTION(BlueprintCallable, Category = "Evidence")
    bool AddEvidence(const FBalhwajeomEvidenceData& NewEvidence);

    UFUNCTION(BlueprintPure, Category = "Evidence")
    bool HasEvidence(FName EvidenceID) const;

    UFUNCTION(BlueprintPure, Category = "Evidence")
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
    void UpdateEvidenceFocus(float DeltaTime);
    void RefreshDisplayedGuideSnapshot();
    bool IsDisplayedGuideSurfaceVisible() const;
    bool IsViewportCenterOverTarget(const AActor* Target) const;
    bool CalculateTargetFrameCoverage(const AActor* Target, float& OutCoverageRatio) const;
    void ApplyDepthOfField(float DeltaTime, float DesiredFocalDistance, bool bHasFocusedTarget);
    void ResetEvidenceFocus();
    bool TryCaptureActiveFocusTarget();
    void SetWorldInspectionLabelsSuppressed(bool bSuppressed) const;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> PhotoCamera;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> NormalCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bIsInCameraMode = false;

    /** Prevents repeated RMB input from restarting or reversing an active fade. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bIsCameraTransitioning = false;

    /** Off by default so existing camera Blueprints keep their previous behavior. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus")
    bool bEnableEvidenceFocusSystem = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus", meta = (ClampMin = "100.0"))
    float FocusTargetScanDistance = 5000.0f;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Framing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinimumCaptureCoverageRatio = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Depth Of Field")
    bool bEnableEvidenceDepthOfField = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Depth Of Field", meta = (ClampMin = "0.0"))
    float UnfocusedFocalDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Depth Of Field", meta = (ClampMin = "0.1", ClampMax = "32.0"))
    float EvidenceFocusFStop = 4.0f;

    /** Higher F-stop keeps the unfocused view readable instead of heavily blurred. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Depth Of Field", meta = (ClampMin = "0.1", ClampMax = "32.0"))
    float UnfocusedFStop = 5.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Evidence Focus|Depth Of Field", meta = (ClampMin = "0.0"))
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
    float CurrentFocalDistance = 300.0f;
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

private:

};
