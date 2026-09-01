// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.h"
#include "Components/ActorComponent.h"
#include "BalhwajeomPhotoCameraComponent.generated.h"

class UCameraComponent;

DECLARE_MULTICAST_DELEGATE(FOnCameraModeExited);

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomPhotoCameraComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    UBalhwajeomPhotoCameraComponent();

    /** Broadcast right when camera mode fully exits, so an active FixedCameraZone can reclaim the view target. */
    FOnCameraModeExited OnCameraModeExited;

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

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> PhotoCamera;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> NormalCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bIsInCameraMode = false;

    /** Prevents repeated RMB input from restarting or reversing an active fade. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bIsCameraTransitioning = false;

    /** Total fade-out + fade-in time for a camera mode change. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.1"))
    float CameraTransitionDuration = 0.5f;

    /** Camera pan speed along the current screen Right/Up axes, in cm/s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0"))
    float CameraPanSpeed = 80.0f;

    /** Maximum distance the camera may be panned from its entry position. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pan", meta = (ClampMin = "0.0"))
    float CameraPanMaxDistance = 150.0f;

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
    float SavedFirstPersonFieldOfView = 90.0f;

private:

};