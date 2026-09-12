#pragma once

#include "CoreMinimal.h"
#include "JMItemInspectionTypes.generated.h"

class UJMItemInspectionData;
class UJMItemInspectionSurfaceWidgetBase;

UENUM(BlueprintType)
enum class EJMItemInspectionState : uint8
{
	Closed,
	Loading,
	Opening,
	TransitioningIn,
	Inspecting,
	TransitioningOut,
	Closing,
	Failed
};

UENUM(BlueprintType)
enum class EJMItemInspectionTransitionEasing : uint8
{
	Linear,
	EaseOutCubic,
	EaseOutBack
};

UENUM(BlueprintType)
enum class EJMItemInspectionCloseReason : uint8
{
	User,
	CloseButton,
	ExternalRequest,
	SourceInvalid,
	WorldTearDown,
	Replaced,
	Failed
};

UENUM(BlueprintType)
enum class EJMItemInspectionLightingPreset : uint8
{
	SoftStudio,
	BrightProduct,
	WarmDramatic,
	CoolRim,
	Flat
};

/**
 * Optional world-space UMG content attached to the inspected mesh.
 * The widget lives only for the inspection session and follows preview rotation/zoom.
 */
USTRUCT(BlueprintType)
struct ITEMINSPECTORRUNTIME_API FJMItemInspectionSurfaceWidgetSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget")
	bool bEnabled = false;

	/** A subclass of UJMItemInspectionSurfaceWidgetBase. Null safely uses the native fallback widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget")
	TSoftClassPtr<UJMItemInspectionSurfaceWidgetBase> WidgetClass;

	/** Per-item text rendered by the surface widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget")
	bool bUseDescriptionAsContent = true;

	/** Used only when bUseDescriptionAsContent is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget", meta = (MultiLine = true, EditCondition = "!bUseDescriptionAsContent"))
	FText Content;

	/** Widget render resolution. World size is controlled by SurfaceTransform.Scale3D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget", meta = (ClampMin = "64"))
	FIntPoint DrawSize = FIntPoint(1024, 1024);

	/** Local transform relative to the same pivot that rotates the preview mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget")
	FTransform SurfaceTransform = FTransform(
		FRotator(-90.0, 0.0, 0.0),
		FVector(0.0, 0.0, 0.2),
		FVector(0.1, 0.1, 0.1));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Surface Widget")
	bool bTwoSided = false;

	FIntPoint GetSafeDrawSize() const
	{
		return FIntPoint(FMath::Max(DrawSize.X, 64), FMath::Max(DrawSize.Y, 64));
	}
};

USTRUCT(BlueprintType)
struct ITEMINSPECTORRUNTIME_API FJMItemInspectionPreviewLightingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting")
	EJMItemInspectionLightingPreset LightingPreset = EJMItemInspectionLightingPreset::SoftStudio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting", meta = (ClampMin = "0.0"))
	float IntensityMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting")
	bool bIsolateFromWorldLighting = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting")
	bool bCastPreviewShadows = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting")
	bool bAffectTranslucentLighting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom")
	bool bOverridePreset = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom", meta = (EditCondition = "bOverridePreset", ClampMin = "0.0"))
	float KeyLightIntensity = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom", meta = (EditCondition = "bOverridePreset"))
	FLinearColor KeyLightColor = FLinearColor(1.0f, 0.95f, 0.86f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom", meta = (EditCondition = "bOverridePreset", ClampMin = "0.0"))
	float FillLightIntensity = 2.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom", meta = (EditCondition = "bOverridePreset"))
	FLinearColor FillLightColor = FLinearColor(0.72f, 0.84f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom", meta = (EditCondition = "bOverridePreset", ClampMin = "0.0"))
	float RimLightIntensity = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Lighting|Custom", meta = (EditCondition = "bOverridePreset"))
	FLinearColor RimLightColor = FLinearColor(0.85f, 0.92f, 1.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct ITEMINSPECTORRUNTIME_API FJMItemInspectionViewSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	FVector PreviewOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	FRotator InitialRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection", meta = (ClampMin = "0.01"))
	float PreviewScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection", meta = (ClampMin = "0.01"))
	float RotationSensitivity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bAllowPitchRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bAllowYawRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bAllowZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection", meta = (ClampMin = "0.01"))
	float InitialZoom = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection", meta = (ClampMin = "0.01"))
	float MinZoom = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection", meta = (ClampMin = "0.01"))
	float MaxZoom = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionPreviewLightingSettings LightingSettings;
};

/** Controls the optional world-item-to-inspector entrance transition. */
USTRUCT(BlueprintType)
struct ITEMINSPECTORRUNTIME_API FJMItemInspectionTransitionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition")
	bool bEnableEnterTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition")
	bool bEnableExitTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (ClampMin = "0.01", Units = "s", EditCondition = "bEnableEnterTransition"))
	float EnterDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (ClampMin = "0.01", Units = "s", EditCondition = "bEnableExitTransition"))
	float ExitDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bEnableEnterTransition"))
	float SourceHideDelay = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableEnterTransition"))
	float InspectorFadeStartAlpha = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (EditCondition = "bEnableEnterTransition"))
	EJMItemInspectionTransitionEasing Easing = EJMItemInspectionTransitionEasing::EaseOutCubic;

	/** Keeps the level actor hidden after the transition until inspection closes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (EditCondition = "bEnableEnterTransition"))
	bool bHideSourceActorDuringInspection = true;
};

/** Runtime-only information sampled from the level actor before the transition starts. */
struct ITEMINSPECTORRUNTIME_API FJMItemInspectionTransitionSource
{
	FBoxSphereBounds WorldBounds;
	FVector2D ScreenCenter = FVector2D::ZeroVector;
	FVector2D ScreenSize = FVector2D::ZeroVector;
	FQuat PreviewRelativeRotation = FQuat::Identity;
	bool bIsOnScreen = false;
	bool bIsValid = false;
};

USTRUCT(BlueprintType)
struct ITEMINSPECTORRUNTIME_API FJMItemInspectionRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UJMItemInspectionData> InspectionData = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UObject> RequestSource = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bPauseGame = false;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bBlockPlayerInput = true;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bHideSourceActor = false;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bAllowClose = true;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition")
	bool bOverrideTransitionSettings = false;

	/** Integration paths such as inspect-before-pickup can suppress returning to a world source. */
	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition")
	bool bDisableExitTransition = false;

	/** Uses a simple full-widget fade when opening from another UI instead of a world actor. */
	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition")
	bool bUseSimpleUITransition = false;

	UPROPERTY(BlueprintReadWrite, Category = "JM Gameplay|Item Inspection|Transition", meta = (EditCondition = "bOverrideTransitionSettings"))
	FJMItemInspectionTransitionSettings TransitionSettings;
};
