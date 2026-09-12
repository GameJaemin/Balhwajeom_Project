#include "ItemInspection/JMItemInspectionPreviewActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "ItemInspection/JMItemInspectionSurfaceWidgetBase.h"
#include "ItemInspection/JMItemInspectionTypes.h"
#include "Materials/MaterialInterface.h"

namespace
{
struct FJMResolvedPreviewLighting
{
	float KeyIntensity = 7.0f;
	FLinearColor KeyColor = FLinearColor(1.0f, 0.95f, 0.86f, 1.0f);
	float FillIntensity = 2.25f;
	FLinearColor FillColor = FLinearColor(0.72f, 0.84f, 1.0f, 1.0f);
	float RimIntensity = 2.8f;
	FLinearColor RimColor = FLinearColor(0.85f, 0.92f, 1.0f, 1.0f);
};

FJMResolvedPreviewLighting ResolvePreviewLighting(const FJMItemInspectionPreviewLightingSettings& LightingSettings)
{
	FJMResolvedPreviewLighting ResolvedLighting;

	switch (LightingSettings.LightingPreset)
	{
	case EJMItemInspectionLightingPreset::BrightProduct:
		ResolvedLighting.KeyIntensity = 8.5f;
		ResolvedLighting.KeyColor = FLinearColor(1.0f, 0.98f, 0.92f, 1.0f);
		ResolvedLighting.FillIntensity = 4.0f;
		ResolvedLighting.FillColor = FLinearColor(0.82f, 0.9f, 1.0f, 1.0f);
		ResolvedLighting.RimIntensity = 2.0f;
		ResolvedLighting.RimColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
		break;
	case EJMItemInspectionLightingPreset::WarmDramatic:
		ResolvedLighting.KeyIntensity = 7.5f;
		ResolvedLighting.KeyColor = FLinearColor(1.0f, 0.78f, 0.55f, 1.0f);
		ResolvedLighting.FillIntensity = 1.0f;
		ResolvedLighting.FillColor = FLinearColor(0.38f, 0.48f, 0.72f, 1.0f);
		ResolvedLighting.RimIntensity = 3.8f;
		ResolvedLighting.RimColor = FLinearColor(1.0f, 0.88f, 0.72f, 1.0f);
		break;
	case EJMItemInspectionLightingPreset::CoolRim:
		ResolvedLighting.KeyIntensity = 6.5f;
		ResolvedLighting.KeyColor = FLinearColor(0.92f, 0.96f, 1.0f, 1.0f);
		ResolvedLighting.FillIntensity = 1.8f;
		ResolvedLighting.FillColor = FLinearColor(0.45f, 0.58f, 1.0f, 1.0f);
		ResolvedLighting.RimIntensity = 5.0f;
		ResolvedLighting.RimColor = FLinearColor(0.35f, 0.68f, 1.0f, 1.0f);
		break;
	case EJMItemInspectionLightingPreset::Flat:
		ResolvedLighting.KeyIntensity = 4.0f;
		ResolvedLighting.KeyColor = FLinearColor::White;
		ResolvedLighting.FillIntensity = 4.0f;
		ResolvedLighting.FillColor = FLinearColor::White;
		ResolvedLighting.RimIntensity = 0.6f;
		ResolvedLighting.RimColor = FLinearColor::White;
		break;
	case EJMItemInspectionLightingPreset::SoftStudio:
	default:
		break;
	}

	if (LightingSettings.bOverridePreset)
	{
		ResolvedLighting.KeyIntensity = LightingSettings.KeyLightIntensity;
		ResolvedLighting.KeyColor = LightingSettings.KeyLightColor;
		ResolvedLighting.FillIntensity = LightingSettings.FillLightIntensity;
		ResolvedLighting.FillColor = LightingSettings.FillLightColor;
		ResolvedLighting.RimIntensity = LightingSettings.RimLightIntensity;
		ResolvedLighting.RimColor = LightingSettings.RimLightColor;
	}

	const float Multiplier = FMath::Max(LightingSettings.IntensityMultiplier, 0.0f);
	ResolvedLighting.KeyIntensity *= Multiplier;
	ResolvedLighting.FillIntensity *= Multiplier;
	ResolvedLighting.RimIntensity *= Multiplier;
	return ResolvedLighting;
}

void SetLightViewChannels(ULightComponent* LightComponent, bool bChannel0, bool bChannel1, bool bChannel2)
{
	if (!LightComponent)
	{
		return;
	}

	LightComponent->ViewLightingChannels.bViewChannel0 = bChannel0;
	LightComponent->ViewLightingChannels.bViewChannel1 = bChannel1;
	LightComponent->ViewLightingChannels.bViewChannel2 = bChannel2;
	LightComponent->ViewLightingChannels.bViewChannel3 = false;
	LightComponent->ViewLightingChannels.bViewChannel4 = false;
}
}

AJMItemInspectionPreviewActor::AJMItemInspectionPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PreviewPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewPivot"));
	PreviewPivot->SetupAttachment(SceneRoot);

	PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMeshComponent->SetupAttachment(PreviewPivot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetGenerateOverlapEvents(false);
	PreviewMeshComponent->SetLightingChannels(false, false, true);

	SurfaceWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SurfaceWidget"));
	SurfaceWidgetComponent->SetupAttachment(PreviewPivot);
	SurfaceWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SurfaceWidgetComponent->SetGenerateOverlapEvents(false);
	SurfaceWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	SurfaceWidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
	SurfaceWidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	SurfaceWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	SurfaceWidgetComponent->SetTickWhenOffscreen(true);
	SurfaceWidgetComponent->SetManuallyRedraw(false);
	SurfaceWidgetComponent->SetTranslucentSortPriority(10);
	SurfaceWidgetComponent->SetVisibility(false);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(SceneRoot);
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->bAlwaysPersistRenderingState = true;
	SceneCapture->CaptureSource = SCS_SceneColorHDR;
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetCloud(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetSkyLighting(false);
	SceneCapture->ShowFlags.SetVolumetricFog(false);
	SceneCapture->ViewLightingChannels.bViewChannel0 = false;
	SceneCapture->ViewLightingChannels.bViewChannel1 = false;
	SceneCapture->ViewLightingChannels.bViewChannel2 = true;
	SceneCapture->FOVAngle = 45.0f;

	KeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(SceneRoot);
	KeyLight->SetUseInverseSquaredFalloff(false);
	KeyLight->SetLightFalloffExponent(4.0f);
	KeyLight->SetInnerConeAngle(32.0f);
	KeyLight->SetOuterConeAngle(58.0f);
	KeyLight->SetLightingChannels(false, false, true);
	SetLightViewChannels(KeyLight, false, false, true);

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(SceneRoot);
	FillLight->SetUseInverseSquaredFalloff(false);
	FillLight->SetLightFalloffExponent(3.5f);
	FillLight->SetLightingChannels(false, false, true);
	SetLightViewChannels(FillLight, false, false, true);

	RimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(SceneRoot);
	RimLight->SetUseInverseSquaredFalloff(false);
	RimLight->SetLightFalloffExponent(5.0f);
	RimLight->SetLightingChannels(false, false, true);
	SetLightViewChannels(RimLight, false, false, true);
}

bool AJMItemInspectionPreviewActor::ConfigurePreview(UJMItemInspectionData* InspectionData, UStaticMesh* PreviewMesh, UTextureRenderTarget2D* RenderTarget, float PreviewFOV, float ZoomSpeed)
{
	if (!IsValid(InspectionData) || !IsValid(PreviewMesh) || !IsValid(RenderTarget) || !SceneCapture || !PreviewMeshComponent)
	{
		return false;
	}

	CurrentInspectionData = InspectionData;
	CurrentZoomSpeed = FMath::Max(ZoomSpeed, 0.01f);
	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->FOVAngle = FMath::Clamp(PreviewFOV, 5.0f, 170.0f);
	SceneCapture->ShowOnlyActors.Reset();
	SceneCapture->ShowOnlyActors.Add(this);

	PreviewMeshComponent->SetStaticMesh(PreviewMesh);
	const int32 MaterialSlotCount = PreviewMeshComponent->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < InspectionData->MaterialOverrides.Num() && MaterialIndex < MaterialSlotCount; ++MaterialIndex)
	{
		if (!InspectionData->MaterialOverrides[MaterialIndex].IsNull())
		{
			if (UMaterialInterface* Material = InspectionData->MaterialOverrides[MaterialIndex].LoadSynchronous())
			{
				PreviewMeshComponent->SetMaterial(MaterialIndex, Material);
			}
		}
	}

	SceneCapture->bCaptureEveryFrame = false;
	SurfaceWidgetComponent->SetVisibility(false);
	SurfaceWidgetComponent->SetWidgetClass(nullptr);
	if (InspectionData->SurfaceWidget.bEnabled)
	{
		TSubclassOf<UJMItemInspectionSurfaceWidgetBase> SurfaceWidgetClass = UJMItemInspectionSurfaceWidgetBase::StaticClass();
		if (!InspectionData->SurfaceWidget.WidgetClass.IsNull())
		{
			if (TSubclassOf<UJMItemInspectionSurfaceWidgetBase> LoadedClass = InspectionData->SurfaceWidget.WidgetClass.LoadSynchronous())
			{
				SurfaceWidgetClass = LoadedClass;
			}
		}

		const FIntPoint SafeDrawSize = InspectionData->SurfaceWidget.GetSafeDrawSize();
		SurfaceWidgetComponent->SetDrawSize(FVector2D(SafeDrawSize.X, SafeDrawSize.Y));
		SurfaceWidgetComponent->SetRelativeTransform(InspectionData->SurfaceWidget.SurfaceTransform);
		SurfaceWidgetComponent->SetTwoSided(InspectionData->SurfaceWidget.bTwoSided);
		SurfaceWidgetComponent->SetWidgetClass(SurfaceWidgetClass);
		SurfaceWidgetComponent->SetVisibility(true);
		SurfaceWidgetComponent->InitWidget();
		if (UJMItemInspectionSurfaceWidgetBase* SurfaceWidget = Cast<UJMItemInspectionSurfaceWidgetBase>(SurfaceWidgetComponent->GetWidget()))
		{
			const FText& SurfaceContent = InspectionData->SurfaceWidget.bUseDescriptionAsContent
				? InspectionData->Description
				: InspectionData->SurfaceWidget.Content;
			SurfaceWidget->SetSurfaceContent(SurfaceContent);
		}
		SurfaceWidgetComponent->RequestRedraw();
		// World-space widgets render to their own target during the frame. Keeping
		// capture active for this short-lived preview session guarantees the first
		// rendered text and any Blueprint content updates are visible.
		SceneCapture->bCaptureEveryFrame = true;
	}

	ApplyViewSettings();
	CapturePreview();
	return true;
}

void AJMItemInspectionPreviewActor::RotatePreview(float ScreenDeltaX, float ScreenDeltaY)
{
	if (!CurrentInspectionData || !PreviewPivot)
	{
		return;
	}

	const FJMItemInspectionViewSettings& ViewSettings = CurrentInspectionData->ViewSettings;
	FQuat DeltaRotation = FQuat::Identity;
	if (ViewSettings.bAllowYawRotation)
	{
		const float YawRadians = FMath::DegreesToRadians(ScreenDeltaX * ViewSettings.RotationSensitivity);
		DeltaRotation = FQuat(FVector::UpVector, YawRadians) * DeltaRotation;
	}

	if (ViewSettings.bAllowPitchRotation)
	{
		const float PitchRadians = FMath::DegreesToRadians(ScreenDeltaY * ViewSettings.RotationSensitivity);
		DeltaRotation = FQuat(FVector::RightVector, PitchRadians) * DeltaRotation;
	}

	CurrentRotationQuat = (DeltaRotation * CurrentRotationQuat).GetNormalized();
	PreviewPivot->SetRelativeRotation(CurrentRotationQuat);
	CapturePreview();
}

void AJMItemInspectionPreviewActor::ZoomPreview(float WheelDelta)
{
	if (!CurrentInspectionData)
	{
		return;
	}

	const FJMItemInspectionViewSettings& ViewSettings = CurrentInspectionData->ViewSettings;
	if (!ViewSettings.bAllowZoom)
	{
		return;
	}

	const float MinZoom = FMath::Max(0.01f, FMath::Min(ViewSettings.MinZoom, ViewSettings.MaxZoom));
	const float MaxZoom = FMath::Max(MinZoom, FMath::Max(ViewSettings.MinZoom, ViewSettings.MaxZoom));
	CurrentZoom = FMath::Clamp(CurrentZoom + (WheelDelta * CurrentZoomSpeed), MinZoom, MaxZoom);

	ApplyCameraDistance();
	CapturePreview();
}

void AJMItemInspectionPreviewActor::ResetPreviewRotation()
{
	if (!CurrentInspectionData || !PreviewPivot)
	{
		return;
	}

	CurrentRotationQuat = CurrentInspectionData->ViewSettings.InitialRotation.Quaternion();
	PreviewPivot->SetRelativeRotation(CurrentRotationQuat);
	CapturePreview();
}

void AJMItemInspectionPreviewActor::CapturePreview()
{
	if (SceneCapture && !SceneCapture->bCaptureEveryFrame)
	{
		SceneCapture->CaptureScene();
	}
}

void AJMItemInspectionPreviewActor::BeginEnterTransition(const FJMItemInspectionTransitionSource& Source)
{
	if (!CurrentInspectionData || !PreviewPivot || !PreviewMeshComponent)
	{
		return;
	}

	const FJMItemInspectionViewSettings& ViewSettings = CurrentInspectionData->ViewSettings;
	TransitionTargetRotation = ViewSettings.InitialRotation.Quaternion();
	TransitionTargetOffset = ViewSettings.PreviewOffset;
	TransitionTargetScale = FMath::Max(ViewSettings.PreviewScale, 0.01f);
	TransitionTargetZoom = FMath::Clamp(
		ViewSettings.InitialZoom,
		FMath::Max(0.01f, FMath::Min(ViewSettings.MinZoom, ViewSettings.MaxZoom)),
		FMath::Max(ViewSettings.MinZoom, ViewSettings.MaxZoom));

	TransitionStartRotation = Source.PreviewRelativeRotation.GetNormalized();
	TransitionStartOffset = FVector::ZeroVector;
	TransitionStartScale = TransitionTargetScale * 0.85f;
	TransitionStartZoom = FMath::Max(TransitionTargetZoom * 0.85f, 0.01f);
	bEnterTransitionActive = true;
	UpdateEnterTransition(0.0f);
}

void AJMItemInspectionPreviewActor::UpdateEnterTransition(float Alpha)
{
	if (!bEnterTransitionActive || !CurrentInspectionData || !PreviewPivot || !PreviewMeshComponent)
	{
		return;
	}

	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	CurrentRotationQuat = FQuat::Slerp(TransitionStartRotation, TransitionTargetRotation, SafeAlpha).GetNormalized();
	PreviewPivot->SetRelativeRotation(CurrentRotationQuat);

	const UStaticMesh* StaticMesh = PreviewMeshComponent->GetStaticMesh();
	const FVector MeshOrigin = StaticMesh ? StaticMesh->GetBounds().Origin : FVector::ZeroVector;
	const float CurrentScale = FMath::Lerp(TransitionStartScale, TransitionTargetScale, SafeAlpha);
	const FVector CurrentOffset = FMath::Lerp(TransitionStartOffset, TransitionTargetOffset, SafeAlpha);
	PreviewMeshComponent->SetRelativeScale3D(FVector(CurrentScale));
	PreviewMeshComponent->SetRelativeLocation(CurrentOffset - (MeshOrigin * CurrentScale));

	CurrentZoom = FMath::Lerp(TransitionStartZoom, TransitionTargetZoom, SafeAlpha);
	ApplyCameraDistance();
	CapturePreview();
}

void AJMItemInspectionPreviewActor::CompleteEnterTransition()
{
	if (!bEnterTransitionActive)
	{
		return;
	}

	bEnterTransitionActive = false;
	ApplyViewSettings();
	CapturePreview();
}

void AJMItemInspectionPreviewActor::CancelTransition()
{
	if (!bEnterTransitionActive)
	{
		return;
	}

	bEnterTransitionActive = false;
	ApplyViewSettings();
	CapturePreview();
}

void AJMItemInspectionPreviewActor::BeginExitTransition(const FJMItemInspectionTransitionSource& Source)
{
	if (!CurrentInspectionData || !PreviewPivot || !PreviewMeshComponent)
	{
		return;
	}

	const UStaticMesh* StaticMesh = PreviewMeshComponent->GetStaticMesh();
	const FVector MeshOrigin = StaticMesh ? StaticMesh->GetBounds().Origin : FVector::ZeroVector;
	TransitionStartRotation = CurrentRotationQuat;
	TransitionTargetRotation = Source.PreviewRelativeRotation.GetNormalized();
	TransitionStartScale = PreviewMeshComponent->GetRelativeScale3D().X;
	TransitionStartOffset = PreviewMeshComponent->GetRelativeLocation() + (MeshOrigin * TransitionStartScale);
	TransitionTargetScale = FMath::Max(CurrentInspectionData->ViewSettings.PreviewScale * 0.85f, 0.01f);
	TransitionTargetOffset = FVector::ZeroVector;
	TransitionStartZoom = CurrentZoom;
	TransitionTargetZoom = FMath::Max(CurrentInspectionData->ViewSettings.InitialZoom * 0.85f, 0.01f);
	bExitTransitionActive = true;
	UpdateExitTransition(0.0f);
}

void AJMItemInspectionPreviewActor::UpdateExitTransition(float Alpha)
{
	if (!bExitTransitionActive || !PreviewPivot || !PreviewMeshComponent)
	{
		return;
	}

	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	CurrentRotationQuat = FQuat::Slerp(TransitionStartRotation, TransitionTargetRotation, SafeAlpha).GetNormalized();
	PreviewPivot->SetRelativeRotation(CurrentRotationQuat);

	const UStaticMesh* StaticMesh = PreviewMeshComponent->GetStaticMesh();
	const FVector MeshOrigin = StaticMesh ? StaticMesh->GetBounds().Origin : FVector::ZeroVector;
	const float CurrentScale = FMath::Lerp(TransitionStartScale, TransitionTargetScale, SafeAlpha);
	const FVector CurrentOffset = FMath::Lerp(TransitionStartOffset, TransitionTargetOffset, SafeAlpha);
	PreviewMeshComponent->SetRelativeScale3D(FVector(CurrentScale));
	PreviewMeshComponent->SetRelativeLocation(CurrentOffset - (MeshOrigin * CurrentScale));
	CurrentZoom = FMath::Lerp(TransitionStartZoom, TransitionTargetZoom, SafeAlpha);
	ApplyCameraDistance();
	CapturePreview();
}

void AJMItemInspectionPreviewActor::CompleteExitTransition()
{
	if (!bExitTransitionActive)
	{
		return;
	}

	UpdateExitTransition(1.0f);
	bExitTransitionActive = false;
}

void AJMItemInspectionPreviewActor::CancelExitTransition()
{
	if (!bExitTransitionActive)
	{
		return;
	}

	bExitTransitionActive = false;
	ApplyViewSettings();
	CapturePreview();
}

void AJMItemInspectionPreviewActor::ApplyViewSettings()
{
	if (!CurrentInspectionData || !PreviewMeshComponent || !SceneCapture)
	{
		return;
	}

	const FJMItemInspectionViewSettings& ViewSettings = CurrentInspectionData->ViewSettings;
	const float SafeScale = FMath::Max(ViewSettings.PreviewScale, 0.01f);
	const FBoxSphereBounds MeshBounds = PreviewMeshComponent->GetStaticMesh()
		? PreviewMeshComponent->GetStaticMesh()->GetBounds()
		: FBoxSphereBounds(FSphere(FVector::ZeroVector, 100.0f));

	PreviewMeshComponent->SetRelativeScale3D(FVector(SafeScale));
	PreviewMeshComponent->SetRelativeLocation(ViewSettings.PreviewOffset - (MeshBounds.Origin * SafeScale));

	CurrentRotationQuat = ViewSettings.InitialRotation.Quaternion();
	PreviewPivot->SetRelativeRotation(CurrentRotationQuat);

	CurrentZoom = FMath::Clamp(ViewSettings.InitialZoom, FMath::Max(0.01f, FMath::Min(ViewSettings.MinZoom, ViewSettings.MaxZoom)), FMath::Max(ViewSettings.MinZoom, ViewSettings.MaxZoom));
	BaseCameraDistance = CalculateCameraDistance(MeshBounds.SphereRadius * SafeScale, SceneCapture->FOVAngle);
	ApplyLightingSettings(MeshBounds.SphereRadius * SafeScale);
	ApplyCameraDistance();
}

void AJMItemInspectionPreviewActor::ApplyLightingSettings(float BoundsRadius)
{
	if (!CurrentInspectionData || !PreviewMeshComponent || !SceneCapture || !KeyLight || !FillLight || !RimLight)
	{
		return;
	}

	const FJMItemInspectionPreviewLightingSettings& LightingSettings = CurrentInspectionData->ViewSettings.LightingSettings;
	const FJMResolvedPreviewLighting ResolvedLighting = ResolvePreviewLighting(LightingSettings);
	const bool bUseIsolatedChannel = LightingSettings.bIsolateFromWorldLighting;

	PreviewMeshComponent->SetLightingChannels(!bUseIsolatedChannel, false, bUseIsolatedChannel);
	SceneCapture->ViewLightingChannels.bViewChannel0 = !bUseIsolatedChannel;
	SceneCapture->ViewLightingChannels.bViewChannel1 = false;
	SceneCapture->ViewLightingChannels.bViewChannel2 = bUseIsolatedChannel;

	KeyLight->SetLightingChannels(!bUseIsolatedChannel, false, bUseIsolatedChannel);
	FillLight->SetLightingChannels(!bUseIsolatedChannel, false, bUseIsolatedChannel);
	RimLight->SetLightingChannels(!bUseIsolatedChannel, false, bUseIsolatedChannel);
	SetLightViewChannels(KeyLight, !bUseIsolatedChannel, false, bUseIsolatedChannel);
	SetLightViewChannels(FillLight, !bUseIsolatedChannel, false, bUseIsolatedChannel);
	SetLightViewChannels(RimLight, !bUseIsolatedChannel, false, bUseIsolatedChannel);

	const bool bCastShadows = LightingSettings.bCastPreviewShadows;
	PreviewMeshComponent->SetCastShadow(bCastShadows);
	KeyLight->SetCastShadows(bCastShadows);
	FillLight->SetCastShadows(false);
	RimLight->SetCastShadows(false);

	KeyLight->SetAffectTranslucentLighting(LightingSettings.bAffectTranslucentLighting);
	FillLight->SetAffectTranslucentLighting(LightingSettings.bAffectTranslucentLighting);
	RimLight->SetAffectTranslucentLighting(LightingSettings.bAffectTranslucentLighting);

	const float LightDistance = FMath::Max3(BaseCameraDistance * 0.9f, BoundsRadius * 4.0f, 120.0f);
	const float AttenuationRadius = FMath::Max(LightDistance * 2.4f, BoundsRadius * 6.0f);

	const FVector KeyLocation = FVector(-0.75f, -0.55f, 0.72f).GetSafeNormal() * LightDistance;
	const FVector FillLocation = FVector(-0.45f, 0.8f, 0.35f).GetSafeNormal() * (LightDistance * 0.9f);
	const FVector RimLocation = FVector(0.9f, 0.28f, 0.58f).GetSafeNormal() * (LightDistance * 0.95f);

	KeyLight->SetRelativeLocation(KeyLocation);
	KeyLight->SetRelativeRotation((-KeyLocation).Rotation());
	KeyLight->SetIntensity(ResolvedLighting.KeyIntensity);
	KeyLight->SetLightColor(ResolvedLighting.KeyColor);
	KeyLight->SetAttenuationRadius(AttenuationRadius);
	KeyLight->SetSourceRadius(FMath::Clamp(BoundsRadius * 0.35f, 4.0f, 80.0f));
	KeyLight->SetSoftSourceRadius(FMath::Clamp(BoundsRadius * 0.55f, 8.0f, 120.0f));
	KeyLight->SetVisibility(ResolvedLighting.KeyIntensity > KINDA_SMALL_NUMBER, true);

	FillLight->SetRelativeLocation(FillLocation);
	FillLight->SetIntensity(ResolvedLighting.FillIntensity);
	FillLight->SetLightColor(ResolvedLighting.FillColor);
	FillLight->SetAttenuationRadius(AttenuationRadius);
	FillLight->SetSourceRadius(FMath::Clamp(BoundsRadius * 0.5f, 8.0f, 120.0f));
	FillLight->SetSoftSourceRadius(FMath::Clamp(BoundsRadius * 0.75f, 12.0f, 160.0f));
	FillLight->SetVisibility(ResolvedLighting.FillIntensity > KINDA_SMALL_NUMBER, true);

	RimLight->SetRelativeLocation(RimLocation);
	RimLight->SetIntensity(ResolvedLighting.RimIntensity);
	RimLight->SetLightColor(ResolvedLighting.RimColor);
	RimLight->SetAttenuationRadius(AttenuationRadius);
	RimLight->SetSourceRadius(FMath::Clamp(BoundsRadius * 0.35f, 6.0f, 100.0f));
	RimLight->SetSoftSourceRadius(FMath::Clamp(BoundsRadius * 0.6f, 10.0f, 140.0f));
	RimLight->SetVisibility(ResolvedLighting.RimIntensity > KINDA_SMALL_NUMBER, true);
}

void AJMItemInspectionPreviewActor::ApplyCameraDistance()
{
	if (!SceneCapture)
	{
		return;
	}

	const float SafeZoom = FMath::Max(CurrentZoom, 0.01f);
	SceneCapture->SetRelativeLocation(FVector(-BaseCameraDistance / SafeZoom, 0.0f, 0.0f));
	SceneCapture->SetRelativeRotation(FRotator::ZeroRotator);
}

float AJMItemInspectionPreviewActor::CalculateCameraDistance(float BoundsRadius, float PreviewFOV) const
{
	const float SafeRadius = FMath::Max(BoundsRadius, 1.0f);
	const float SafeFOV = FMath::Clamp(PreviewFOV, 5.0f, 170.0f);
	const float HalfFOVRadians = FMath::DegreesToRadians(SafeFOV * 0.5f);
	const float Distance = SafeRadius / FMath::Max(FMath::Tan(HalfFOVRadians), 0.01f);
	return FMath::Max(Distance * 1.15f, 20.0f);
}
