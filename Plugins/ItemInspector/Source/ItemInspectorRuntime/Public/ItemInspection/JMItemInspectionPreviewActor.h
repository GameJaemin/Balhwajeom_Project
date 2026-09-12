#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JMItemInspectionPreviewActor.generated.h"

class UJMItemInspectionData;
class UMaterialInterface;
class UPointLightComponent;
class USpotLightComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UWidgetComponent;
struct FJMItemInspectionTransitionSource;

UCLASS(NotPlaceable, Transient)
class ITEMINSPECTORRUNTIME_API AJMItemInspectionPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AJMItemInspectionPreviewActor();

	bool ConfigurePreview(UJMItemInspectionData* InspectionData, UStaticMesh* PreviewMesh, UTextureRenderTarget2D* RenderTarget, float PreviewFOV, float ZoomSpeed);

	void RotatePreview(float ScreenDeltaX, float ScreenDeltaY);
	void ZoomPreview(float WheelDelta);
	void ResetPreviewRotation();
	void CapturePreview();
	void BeginEnterTransition(const FJMItemInspectionTransitionSource& Source);
	void UpdateEnterTransition(float Alpha);
	void CompleteEnterTransition();
	void CancelTransition();
	void BeginExitTransition(const FJMItemInspectionTransitionSource& Source);
	void UpdateExitTransition(float Alpha);
	void CompleteExitTransition();
	void CancelExitTransition();

protected:
	void ApplyViewSettings();
	void ApplyCameraDistance();
	void ApplyLightingSettings(float BoundsRadius);
	float CalculateCameraDistance(float BoundsRadius, float PreviewFOV) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<USceneComponent> PreviewPivot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UStaticMeshComponent> PreviewMeshComponent = nullptr;

	/** Optional world-space UMG surface attached to PreviewPivot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UWidgetComponent> SurfaceWidgetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<USpotLightComponent> KeyLight = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UPointLightComponent> FillLight = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UPointLightComponent> RimLight = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UJMItemInspectionData> CurrentInspectionData = nullptr;

	FQuat CurrentRotationQuat = FQuat::Identity;
	float CurrentZoom = 1.0f;
	float BaseCameraDistance = 250.0f;
	float CurrentZoomSpeed = 0.1f;
	FQuat TransitionStartRotation = FQuat::Identity;
	FQuat TransitionTargetRotation = FQuat::Identity;
	FVector TransitionStartOffset = FVector::ZeroVector;
	FVector TransitionTargetOffset = FVector::ZeroVector;
	float TransitionStartScale = 1.0f;
	float TransitionTargetScale = 1.0f;
	float TransitionStartZoom = 1.0f;
	float TransitionTargetZoom = 1.0f;
	bool bEnterTransitionActive = false;
	bool bExitTransitionActive = false;
};
