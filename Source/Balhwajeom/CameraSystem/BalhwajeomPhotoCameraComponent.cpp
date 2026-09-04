// Fill out your copyright notice in the Description page of Project Settings.

#include "BalhwajeomPhotoCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "BalhwajeomEvidenceActor.h"
#include "BalhwajeomEvidenceCameraHUD.h"
#include "BalhwajeomCameraTargetInterface.h"

namespace
{
	APawn* GetOwningPawn(const UActorComponent* Component)
	{
		return Component ? Cast<APawn>(Component->GetOwner()) : nullptr;
	}

	AController* GetOwningController(const UActorComponent* Component)
	{
		APawn* OwnerPawn = GetOwningPawn(Component);
		return OwnerPawn ? OwnerPawn->GetController() : nullptr;
	}

	bool ProjectActorBoundsToScreen(
		const AActor* Actor,
		APlayerController* PlayerController,
		int32 ViewportWidth,
		int32 ViewportHeight,
		FVector2D& OutScreenMin,
		FVector2D& OutScreenMax)
	{
		if (!Actor || !PlayerController)
		{
			return false;
		}

		const FBox Bounds = Actor->GetComponentsBoundingBox(true);
		if (!Bounds.IsValid)
		{
			return false;
		}

		const FVector Min = Bounds.Min;
		const FVector Max = Bounds.Max;
		const FVector Corners[8] =
		{
			FVector(Min.X, Min.Y, Min.Z), FVector(Min.X, Min.Y, Max.Z),
			FVector(Min.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Max.Z),
			FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Max.Z),
			FVector(Max.X, Max.Y, Min.Z), FVector(Max.X, Max.Y, Max.Z)
		};

		FVector2D ScreenMin(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
		FVector2D ScreenMax(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
		bool bProjectedAnyCorner = false;
		for (const FVector& Corner : Corners)
		{
			FVector2D ScreenCorner;
			if (PlayerController->ProjectWorldLocationToScreen(Corner, ScreenCorner, false))
			{
				ScreenMin.X = FMath::Min(ScreenMin.X, ScreenCorner.X);
				ScreenMin.Y = FMath::Min(ScreenMin.Y, ScreenCorner.Y);
				ScreenMax.X = FMath::Max(ScreenMax.X, ScreenCorner.X);
				ScreenMax.Y = FMath::Max(ScreenMax.Y, ScreenCorner.Y);
				bProjectedAnyCorner = true;
			}
		}

		if (!bProjectedAnyCorner || ScreenMax.X < 0.0f || ScreenMax.Y < 0.0f ||
			ScreenMin.X > ViewportWidth || ScreenMin.Y > ViewportHeight)
		{
			return false;
		}

		OutScreenMin.X = FMath::Clamp(ScreenMin.X, 0.0f, static_cast<float>(ViewportWidth));
		OutScreenMin.Y = FMath::Clamp(ScreenMin.Y, 0.0f, static_cast<float>(ViewportHeight));
		OutScreenMax.X = FMath::Clamp(ScreenMax.X, 0.0f, static_cast<float>(ViewportWidth));
		OutScreenMax.Y = FMath::Clamp(ScreenMax.Y, 0.0f, static_cast<float>(ViewportHeight));
		return true;
	}

	bool FindClosestVisibleSilhouettePoint(
		AActor* Actor,
		APlayerController* PlayerController,
		UWorld* World,
		AActor* TraceOwner,
		const FVector2D& ViewportCenter,
		const FVector2D& ScreenBoundsMin,
		const FVector2D& ScreenBoundsMax,
		float TraceDistance,
		float PixelStep,
		int32 MaxSamples,
		FVector2D& OutScreenPosition,
		FVector& OutWorldPosition,
		FVector& OutWorldNormal)
	{
		if (!Actor || !PlayerController || !World || MaxSamples <= 0)
		{
			return false;
		}

		const float Step = FMath::Max(PixelStep, 1.0f);
		const FVector2D SearchOrigin(
			FMath::Clamp(ViewportCenter.X, ScreenBoundsMin.X, ScreenBoundsMax.X),
			FMath::Clamp(ViewportCenter.Y, ScreenBoundsMin.Y, ScreenBoundsMax.Y));
		int32 SamplesUsed = 0;

		auto TraceScreenPoint = [&] (
			const FVector2D& ScreenPoint,
			FVector& OutHitWorldPosition,
			FVector& OutHitWorldNormal) -> bool
		{
			if (ScreenPoint.X < ScreenBoundsMin.X ||
				ScreenPoint.X > ScreenBoundsMax.X || ScreenPoint.Y < ScreenBoundsMin.Y ||
				ScreenPoint.Y > ScreenBoundsMax.Y)
			{
				return false;
			}
			if (++SamplesUsed > MaxSamples)
			{
				return false;
			}

			FVector RayOrigin;
			FVector RayDirection;
			if (!PlayerController->DeprojectScreenPositionToWorld(
				ScreenPoint.X, ScreenPoint.Y, RayOrigin, RayDirection))
			{
				return false;
			}

			FCollisionQueryParams Params(SCENE_QUERY_STAT(EvidenceSilhouetteTrace), true, TraceOwner);
			Params.bTraceComplex = true;
			FHitResult Hit;
			const bool bHitTarget = World->LineTraceSingleByChannel(
				Hit,
				RayOrigin,
				RayOrigin + RayDirection * TraceDistance,
				ECC_Visibility,
				Params) && Hit.GetActor() == Actor;
			if (bHitTarget)
			{
				OutHitWorldPosition = Hit.ImpactPoint;
				OutHitWorldNormal = Hit.ImpactNormal.GetSafeNormal();
			}
			return bHitTarget;
		};

		FVector SearchOriginHit = FVector::ZeroVector;
		FVector SearchOriginNormal = FVector::ZeroVector;
		if (TraceScreenPoint(SearchOrigin, SearchOriginHit, SearchOriginNormal))
		{
			OutScreenPosition = SearchOrigin;
			OutWorldPosition = SearchOriginHit;
			OutWorldNormal = SearchOriginNormal;
			return true;
		}

		const float MaxRadius = FVector2D::Distance(ScreenBoundsMin, ScreenBoundsMax);
		const int32 MaxRings = FMath::CeilToInt(MaxRadius / Step);
		for (int32 Ring = 1; Ring <= MaxRings && SamplesUsed < MaxSamples; ++Ring)
		{
			const float Radius = Ring * Step;
			FVector2D BestPointThisRing = FVector2D::ZeroVector;
			FVector BestWorldPointThisRing = FVector::ZeroVector;
			FVector BestWorldNormalThisRing = FVector::ZeroVector;
			float BestDistanceThisRing = TNumericLimits<float>::Max();

			auto TryPoint = [&](const FVector2D& Point)
			{
				FVector HitWorldPosition = FVector::ZeroVector;
				FVector HitWorldNormal = FVector::ZeroVector;
				if (SamplesUsed < MaxSamples &&
					TraceScreenPoint(Point, HitWorldPosition, HitWorldNormal))
				{
					const float Distance = FVector2D::Distance(Point, ViewportCenter);
					if (Distance < BestDistanceThisRing)
					{
						BestDistanceThisRing = Distance;
						BestPointThisRing = Point;
						BestWorldPointThisRing = HitWorldPosition;
						BestWorldNormalThisRing = HitWorldNormal;
					}
				}
			};

			for (float Offset = -Radius; Offset <= Radius && SamplesUsed < MaxSamples; Offset += Step)
			{
				TryPoint(SearchOrigin + FVector2D(Offset, -Radius));
				TryPoint(SearchOrigin + FVector2D(Offset, Radius));
			}
			for (float Offset = -Radius + Step; Offset < Radius && SamplesUsed < MaxSamples; Offset += Step)
			{
				TryPoint(SearchOrigin + FVector2D(-Radius, Offset));
				TryPoint(SearchOrigin + FVector2D(Radius, Offset));
			}

			if (BestDistanceThisRing < TNumericLimits<float>::Max())
			{
				// Refine around the first coarse hit so the dot hugs thin or angled edges.
				FVector2D RefinedPoint = BestPointThisRing;
				FVector RefinedWorldPoint = BestWorldPointThisRing;
				FVector RefinedWorldNormal = BestWorldNormalThisRing;
				for (float RefineStep = Step * 0.5f; RefineStep >= 1.0f && SamplesUsed < MaxSamples;
					RefineStep *= 0.5f)
				{
					FVector2D BestRefinedPoint = RefinedPoint;
					FVector BestRefinedWorldPoint = RefinedWorldPoint;
					FVector BestRefinedWorldNormal = RefinedWorldNormal;
					float BestRefinedDistance = FVector2D::Distance(RefinedPoint, ViewportCenter);
					for (int32 Y = -1; Y <= 1 && SamplesUsed < MaxSamples; ++Y)
					{
						for (int32 X = -1; X <= 1 && SamplesUsed < MaxSamples; ++X)
						{
							const FVector2D Point = RefinedPoint + FVector2D(X * RefineStep, Y * RefineStep);
							FVector HitWorldPosition = FVector::ZeroVector;
							FVector HitWorldNormal = FVector::ZeroVector;
							if (TraceScreenPoint(Point, HitWorldPosition, HitWorldNormal))
							{
								const float Distance = FVector2D::Distance(Point, ViewportCenter);
								if (Distance < BestRefinedDistance)
								{
									BestRefinedDistance = Distance;
									BestRefinedPoint = Point;
									BestRefinedWorldPoint = HitWorldPosition;
									BestRefinedWorldNormal = HitWorldNormal;
								}
							}
						}
					}
					RefinedPoint = BestRefinedPoint;
					RefinedWorldPoint = BestRefinedWorldPoint;
					RefinedWorldNormal = BestRefinedWorldNormal;
				}

				OutScreenPosition = RefinedPoint;
				OutWorldPosition = RefinedWorldPoint;
				OutWorldNormal = RefinedWorldNormal;
				return true;
			}
		}

		return false;
	}
}

UBalhwajeomPhotoCameraComponent::UBalhwajeomPhotoCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBalhwajeomPhotoCameraComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsInCameraMode && bEnableEvidenceFocusSystem)
	{
		// Silhouette discovery remains interval-based, but center entry must react
		// immediately to the exact pixel under the camera reticle.
		bActiveFocusTargetCentered = IsViewportCenterOverTarget(ActiveFocusTarget.Get());
		bActiveFocusTargetFramedEnough = CalculateTargetFrameCoverage(
			ActiveFocusTarget.Get(), ActiveFocusCoverageRatio) &&
			ActiveFocusCoverageRatio >= FMath::Clamp(MinimumCaptureCoverageRatio, 0.0f, 1.0f);

		EarlyGuideRescanElapsed += DeltaTime;
		if (DisplayedFocusTarget.IsValid() && bDisplayedFocusGuideLocationValid &&
			!IsDisplayedGuideSurfaceVisible())
		{
			bDisplayedFocusGuideVisibilityValid = false;
			if (EarlyGuideRescanElapsed >= GuideEarlyRescanCooldown)
			{
				UpdateEvidenceFocus(DeltaTime);
				RefreshDisplayedGuideSnapshot();
				FocusGuideTraceElapsed = 0.0f;
				EarlyGuideRescanElapsed = 0.0f;
				bDisplayedFocusGuideVisibilityValid = IsDisplayedGuideSurfaceVisible();
			}
			return;
		}
		bDisplayedFocusGuideVisibilityValid = true;

		FocusGuideTraceElapsed += DeltaTime;
		if (FocusGuideTraceInterval <= 0.0f || FocusGuideTraceElapsed >= FocusGuideTraceInterval)
		{
			UpdateEvidenceFocus(FocusGuideTraceElapsed);
			RefreshDisplayedGuideSnapshot();
			FocusGuideTraceElapsed = 0.0f;
		}
	}
}

void UBalhwajeomPhotoCameraComponent::SetPhotoCamera(UCameraComponent* Camera)
{
	PhotoCamera = Camera;
}

void UBalhwajeomPhotoCameraComponent::SetNormalCamera(UCameraComponent* Camera)
{
	NormalCamera = Camera;
}

void UBalhwajeomPhotoCameraComponent::ToggleCameraMode()
{
	// Ignore rapid presses until the current fade-out/switch/fade-in sequence ends.
	if (bIsCameraTransitioning || !PhotoCamera || !NormalCamera || !GetWorld())
	{
		return;
	}

	bIsCameraTransitioning = true;
	const float HalfDuration = CameraTransitionDuration * 0.5f;

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
	{
		if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			CameraManager->StartCameraFade(0.0f, 1.0f, HalfDuration, FLinearColor::Black, false, true);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(
		CameraSwitchTimerHandle,
		this,
		&UBalhwajeomPhotoCameraComponent::SwitchCameraAtFadeOut,
		HalfDuration,
		false);
}

void UBalhwajeomPhotoCameraComponent::RequestExitCameraMode()
{
	if (bIsInCameraMode && !bIsCameraTransitioning)
	{
		ToggleCameraMode();
	}
}

void UBalhwajeomPhotoCameraComponent::LookYaw(float Value)
{
	if (!bIsInCameraMode || bIsCameraTransitioning)
	{
		return;
	}

	if (APawn* OwnerPawn = GetOwningPawn(this))
	{
		OwnerPawn->AddControllerYawInput(Value);
	}
}

void UBalhwajeomPhotoCameraComponent::LookPitch(float Value)
{
	if (!bIsInCameraMode || bIsCameraTransitioning)
	{
		return;
	}

	if (APawn* OwnerPawn = GetOwningPawn(this))
	{
		OwnerPawn->AddControllerPitchInput(Value);
	}
}

void UBalhwajeomPhotoCameraComponent::PanHorizontal(float Value)
{
	if (PhotoCamera)
	{
		PanCamera(CameraPanRightDirection, Value);
	}
}

void UBalhwajeomPhotoCameraComponent::PanVertical(float Value)
{
	if (PhotoCamera)
	{
		const float AbsolutePitch = FMath::Abs(
			FRotator::NormalizeAxis(PhotoCamera->GetComponentRotation().Pitch));
		const float SlowdownStart = FMath::Min(
			VerticalPanSlowdownStartPitch, VerticalPanDisablePitch);
		const float DisablePitch = FMath::Max(
			VerticalPanSlowdownStartPitch, VerticalPanDisablePitch);

		float SpeedScale = 1.0f;
		if (FMath::IsNearlyEqual(SlowdownStart, DisablePitch))
		{
			SpeedScale = AbsolutePitch < SlowdownStart ? 1.0f : 0.0f;
		}
		else
		{
			const float SlowdownAlpha = FMath::Clamp(
				(AbsolutePitch - SlowdownStart) / (DisablePitch - SlowdownStart),
				0.0f,
				1.0f);
			SpeedScale = 1.0f - FMath::SmoothStep(0.0f, 1.0f, SlowdownAlpha);
		}

		PanCamera(FVector::UpVector, Value * SpeedScale);
	}
}

void UBalhwajeomPhotoCameraComponent::ZoomCamera(float Value)
{
	if (!bIsInCameraMode || bIsCameraTransitioning || !PhotoCamera || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const float MinFOV = FMath::Min(MinCameraFieldOfView, MaxCameraFieldOfView);
	const float MaxFOV = FMath::Max(MinCameraFieldOfView, MaxCameraFieldOfView);
	PhotoCamera->SetFieldOfView(
		FMath::Clamp(PhotoCamera->FieldOfView - Value * CameraZoomStep, MinFOV, MaxFOV));
}

void UBalhwajeomPhotoCameraComponent::TakePhoto()
{
	if (!bIsInCameraMode || bIsCameraTransitioning || !PhotoCamera || !GetWorld())
	{
		return;
	}

	AActor* Owner = GetOwner();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
	{
		if (ABalhwajeomEvidenceCameraHUD* CameraHUD = Cast<ABalhwajeomEvidenceCameraHUD>(PlayerController->GetHUD()))
		{
			CameraHUD->TriggerPhotoFlash();
		}
	}

	// The shutter always works. Evidence collection, however, requires a visible target
	// inside the focus-distance band and the center guide.
	if (bEnableEvidenceFocusSystem)
	{
		UpdateEvidenceFocus(0.0f);
		TryCaptureActiveFocusTarget();
		return;
	}

	const FVector TraceStart = PhotoCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + PhotoCamera->GetForwardVector() * PhotoTraceDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvidencePhotoTrace), true, Owner);
	FHitResult Hit;

	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		ShowPhotoFeedback(TEXT("특별한 것은 발견되지 않았다."), FColor::Silver);
		return;
	}

	ABalhwajeomEvidenceActor* Evidence = Cast<ABalhwajeomEvidenceActor>(Hit.GetActor());
	if (!Evidence)
	{
		ShowPhotoFeedback(TEXT("특별한 것은 발견되지 않았다."), FColor::Silver);
		return;
	}

	const FBalhwajeomEvidenceData EvidenceData = Evidence->GetEvidenceData();
	if (AddEvidence(EvidenceData))
	{
		Evidence->MarkAsCollected();
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
		{
			if (ABalhwajeomEvidenceCameraHUD* CameraHUD = Cast<ABalhwajeomEvidenceCameraHUD>(PlayerController->GetHUD()))
			{
				CameraHUD->TriggerEvidenceSavedAnimation(EvidenceData.EvidenceName);
			}
		}
		ShowPhotoFeedback(FString::Printf(TEXT("증거 획득: %s"), *EvidenceData.EvidenceName.ToString()), FColor::Green);
	}
	else
	{
		ShowPhotoFeedback(FString::Printf(TEXT("이미 기록한 증거: %s"), *EvidenceData.EvidenceName.ToString()), FColor::Yellow);
	}
}

bool UBalhwajeomPhotoCameraComponent::AddEvidence(const FBalhwajeomEvidenceData& NewEvidence)
{
	if (NewEvidence.EvidenceID.IsNone() || HasEvidence(NewEvidence.EvidenceID))
	{
		return false;
	}

	FBalhwajeomEvidenceData CollectedData = NewEvidence;
	CollectedData.bAlreadyCollected = true;
	CollectedEvidence.Add(MoveTemp(CollectedData));
	return true;
}

bool UBalhwajeomPhotoCameraComponent::HasEvidence(FName EvidenceID) const
{
	return CollectedEvidence.ContainsByPredicate(
		[EvidenceID](const FBalhwajeomEvidenceData& Evidence)
		{
			return Evidence.EvidenceID == EvidenceID;
		});
}

void UBalhwajeomPhotoCameraComponent::EnterCameraMode()
{
	if (bIsInCameraMode || !PhotoCamera || !NormalCamera)
	{
		return;
	}

	bIsInCameraMode = true;
	SetWorldInspectionLabelsSuppressed(true);
	SavedFirstPersonRelativeTransform = PhotoCamera->GetRelativeTransform();
	SavedFirstPersonFieldOfView = PhotoCamera->FieldOfView;
	SavedPhotoPostProcessSettings = PhotoCamera->PostProcessSettings;
	SavedPostProcessBlendWeight = PhotoCamera->PostProcessBlendWeight;
	CameraModeEntryWorldLocation = PhotoCamera->GetComponentLocation();
	CameraPanWorldOffset = FVector::ZeroVector;
	CameraPanRightDirection = PhotoCamera->GetRightVector();
	CameraPanRightDirection.Z = 0.0f;
	if (!CameraPanRightDirection.Normalize())
	{
		CameraPanRightDirection = FVector::RightVector;
	}
	NormalCamera->SetActive(false);
	PhotoCamera->SetActive(true);

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh())
		{
			OwnerMesh->SetOwnerNoSee(true);
		}
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
	{
		// Reclaim the view target from an active FixedCameraZone so camera mode is always visible,
		// even while standing inside a zone. The switch happens while the screen is faded to black.
		// Control rotation is intentionally left untouched (pitch included) so whatever direction
		// the player was already looking (e.g. from orbit mode) carries straight into camera mode.
		PlayerController->SetViewTargetWithBlend(GetOwner(), 0.0f);
	}

	if (bEnableEvidenceFocusSystem)
	{
		CurrentFocalDistance = UnfocusedFocalDistance;
		FocusGuideTraceElapsed = 0.0f;
		EarlyGuideRescanElapsed = GuideEarlyRescanCooldown;
		SetComponentTickEnabled(true);
		UpdateEvidenceFocus(0.0f);
		RefreshDisplayedGuideSnapshot();
	}
}

void UBalhwajeomPhotoCameraComponent::ExitCameraMode()
{
	if (!bIsInCameraMode)
	{
		return;
	}

	// Control rotation is intentionally left as-is: whatever yaw/pitch the player looked at while
	// in camera mode carries over into the normal view (e.g. the orbit boom). Only the camera's own
	// pan/zoom state is reset below, not the character's rotation.

	if (PhotoCamera)
	{
		PhotoCamera->SetRelativeTransform(SavedFirstPersonRelativeTransform);
		PhotoCamera->SetFieldOfView(SavedFirstPersonFieldOfView);
		if (bEnableEvidenceFocusSystem)
		{
			PhotoCamera->PostProcessSettings = SavedPhotoPostProcessSettings;
			PhotoCamera->PostProcessBlendWeight = SavedPostProcessBlendWeight;
		}
	}
	CameraPanWorldOffset = FVector::ZeroVector;
	FocusGuideTraceElapsed = 0.0f;
	EarlyGuideRescanElapsed = 0.0f;
	SetComponentTickEnabled(false);
	ResetEvidenceFocus();

	bIsInCameraMode = false;
	SetWorldInspectionLabelsSuppressed(false);
	if (PhotoCamera)
	{
		PhotoCamera->SetActive(false);
	}
	if (NormalCamera)
	{
		NormalCamera->SetActive(true);
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh())
		{
			OwnerMesh->SetOwnerNoSee(false);
		}
	}

	// Let an active FixedCameraZone (if any) reclaim the view target now that the screen is faded to black.
	OnCameraModeExited.Broadcast();
}

void UBalhwajeomPhotoCameraComponent::SetWorldInspectionLabelsSuppressed(
	bool bSuppressed) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ABalhwajeomEvidenceActor> It(World); It; ++It)
	{
		It->SetInspectionLabelSuppressed(bSuppressed);
	}
}

void UBalhwajeomPhotoCameraComponent::SwitchCameraAtFadeOut()
{
	if (bIsInCameraMode)
	{
		ExitCameraMode();
	}
	else
	{
		EnterCameraMode();
	}

	const float HalfDuration = CameraTransitionDuration * 0.5f;
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
	{
		if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
		{
			CameraManager->StartCameraFade(1.0f, 0.0f, HalfDuration, FLinearColor::Black, false, false);
		}
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			CameraTransitionTimerHandle,
			this,
			&UBalhwajeomPhotoCameraComponent::FinishCameraTransition,
			HalfDuration,
			false);
	}
}

void UBalhwajeomPhotoCameraComponent::FinishCameraTransition()
{
	bIsCameraTransitioning = false;
}

void UBalhwajeomPhotoCameraComponent::PanCamera(const FVector& ScreenDirection, float Value)
{
	if (!bIsInCameraMode || bIsCameraTransitioning || !PhotoCamera || !GetWorld() || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FVector Delta = ScreenDirection.GetSafeNormal() * Value * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
	CameraPanWorldOffset += Delta;
	CameraPanWorldOffset = CameraPanWorldOffset.GetClampedToMaxSize(CameraPanMaxDistance);
	PhotoCamera->SetWorldLocation(CameraModeEntryWorldLocation + CameraPanWorldOffset);
}

void UBalhwajeomPhotoCameraComponent::ShowPhotoFeedback(const FString& Message, const FColor& Color) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, Color, Message);
	}
}

bool UBalhwajeomPhotoCameraComponent::GetActiveFocusGuide(
	FVector2D& OutScreenPosition,
	bool& bOutIsCentered,
	FBalhwajeomCameraTargetInfo& OutTargetInfo,
	float& OutOpacity) const
{
	if (!bEnableEvidenceFocusSystem || !DisplayedFocusTarget.IsValid() ||
		!bDisplayedFocusGuideVisibilityValid)
	{
		OutOpacity = 0.0f;
		return false;
	}

	AActor* DisplayedTarget = DisplayedFocusTarget.Get();
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	if (!DisplayedTarget || !PlayerController || !bDisplayedFocusGuideLocationValid)
	{
		OutOpacity = 0.0f;
		return false;
	}

	const FVector SilhouetteGuideWorldPosition = DisplayedTarget->GetActorTransform().TransformPosition(
		DisplayedFocusGuideLocalPosition);
	FVector2D SilhouetteGuideScreenPosition;
	if (!PlayerController->ProjectWorldLocationToScreen(
		SilhouetteGuideWorldPosition, SilhouetteGuideScreenPosition, false))
	{
		OutOpacity = 0.0f;
		return false;
	}

	float DisplayedCoverageRatio = 0.0f;
	const bool bDisplayedTargetFramedEnough = CalculateTargetFrameCoverage(
		DisplayedTarget, DisplayedCoverageRatio) &&
		DisplayedCoverageRatio >= FMath::Clamp(MinimumCaptureCoverageRatio, 0.0f, 1.0f);
	bOutIsCentered = IsViewportCenterOverTarget(DisplayedTarget) && bDisplayedTargetFramedEnough;

	// Pull the edge guide slightly inside the silhouette toward the authored center.
	// Once the reticle ray actually hits the target, use that center directly.
	OutScreenPosition = SilhouetteGuideScreenPosition;
	if (DisplayedTarget->GetClass()->ImplementsInterface(UBalhwajeomCameraTargetInterface::StaticClass()))
	{
		const FVector CenterWorldPosition =
			IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(DisplayedTarget);
		FVector2D CenterScreenPosition;
		if (PlayerController->ProjectWorldLocationToScreen(
			CenterWorldPosition, CenterScreenPosition, false))
		{
			OutScreenPosition = bOutIsCentered
				? CenterScreenPosition
				: FMath::Lerp(
					SilhouetteGuideScreenPosition,
					CenterScreenPosition,
					FMath::Clamp(YellowGuideInsetRatio, 0.0f, 0.5f));
		}
	}
	OutTargetInfo = DisplayedFocusTargetInfo;
	if (bOutIsCentered || FocusGuideTraceInterval <= KINDA_SMALL_NUMBER)
	{
		// Centered confirmation stays fully visible instead of using the scan pulse.
		OutOpacity = 1.0f;
	}
	else
	{
		const float Phase = FMath::Clamp(FocusGuideTraceElapsed / FocusGuideTraceInterval, 0.0f, 1.0f);
		OutOpacity = FMath::Sin(Phase * PI);
	}
	return true;
}

void UBalhwajeomPhotoCameraComponent::RefreshDisplayedGuideSnapshot()
{
	DisplayedFocusTarget = ActiveFocusTarget;
	DisplayedFocusTargetInfo = ActiveFocusTargetInfo;
	DisplayedFocusGuideLocalPosition = ActiveFocusGuideLocalPosition;
	DisplayedFocusGuideLocalNormal = ActiveFocusGuideLocalNormal;
	bDisplayedFocusGuideLocationValid = bActiveFocusGuideLocationValid;
	bDisplayedFocusGuideVisibilityValid = bDisplayedFocusGuideLocationValid;
}

bool UBalhwajeomPhotoCameraComponent::IsDisplayedGuideSurfaceVisible() const
{
	AActor* Target = DisplayedFocusTarget.Get();
	if (!Target || !PhotoCamera || !GetWorld() || !bDisplayedFocusGuideLocationValid)
	{
		return false;
	}

	const FTransform TargetTransform = Target->GetActorTransform();
	const FVector GuideWorldPosition = TargetTransform.TransformPosition(DisplayedFocusGuideLocalPosition);
	const FVector GuideWorldNormal = TargetTransform.TransformVectorNoScale(
		DisplayedFocusGuideLocalNormal).GetSafeNormal();
	const FVector CameraLocation = PhotoCamera->GetComponentLocation();
	const FVector ToGuide = GuideWorldPosition - CameraLocation;
	const float GuideDistance = ToGuide.Size();
	if (GuideDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector ViewDirectionFromSurface = (CameraLocation - GuideWorldPosition).GetSafeNormal();
	if (FVector::DotProduct(GuideWorldNormal, ViewDirectionFromSurface) < GuideFacingDotThreshold)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EvidenceGuideOcclusion), true, GetOwner());
	Params.bTraceComplex = true;
	FHitResult Hit;
	const FVector TraceEnd = CameraLocation + ToGuide.GetSafeNormal() *
		(GuideDistance + GuideVisibilityImpactTolerance * 2.0f);
	if (!GetWorld()->LineTraceSingleByChannel(
		Hit, CameraLocation, TraceEnd, ECC_Visibility, Params))
	{
		return false;
	}

	return Hit.GetActor() == Target &&
		FVector::Distance(Hit.ImpactPoint, GuideWorldPosition) <= GuideVisibilityImpactTolerance;
}

bool UBalhwajeomPhotoCameraComponent::IsViewportCenterOverTarget(const AActor* Target) const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	UWorld* World = GetWorld();
	if (!Target || !PlayerController || !World)
	{
		return false;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return false;
	}

	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(
		ViewportWidth * 0.5f,
		ViewportHeight * 0.5f,
		RayOrigin,
		RayDirection))
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EvidenceCenterTrace), true, GetOwner());
	Params.bTraceComplex = true;
	FHitResult Hit;
	return World->LineTraceSingleByChannel(
		Hit,
		RayOrigin,
		RayOrigin + RayDirection * FocusTargetScanDistance,
		ECC_Visibility,
		Params) && Hit.GetActor() == Target;
}

bool UBalhwajeomPhotoCameraComponent::CalculateTargetFrameCoverage(
	const AActor* Target,
	float& OutCoverageRatio) const
{
	OutCoverageRatio = 0.0f;
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	if (!Target || !PlayerController ||
		!Target->GetClass()->ImplementsInterface(UBalhwajeomCameraTargetInterface::StaticClass()))
	{
		return false;
	}

	UPrimitiveComponent* FramingComponent =
		IBalhwajeomCameraTargetInterface::Execute_RequestCameraFramingComponent(
			const_cast<AActor*>(Target));
	if (!IsValid(FramingComponent))
	{
		return false;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return false;
	}

	// Calculate local bounds and project all eight corners. Using the component
	// supplied by the target excludes label widgets, focus points and helper objects.
	const FBox LocalBounds = FramingComponent->CalcBounds(FTransform::Identity).GetBox();
	if (!LocalBounds.IsValid)
	{
		return false;
	}

	const FVector Min = LocalBounds.Min;
	const FVector Max = LocalBounds.Max;
	const FVector LocalCorners[8] =
	{
		FVector(Min.X, Min.Y, Min.Z), FVector(Min.X, Min.Y, Max.Z),
		FVector(Min.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Max.Z),
		FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Max.Z),
		FVector(Max.X, Max.Y, Min.Z), FVector(Max.X, Max.Y, Max.Z)
	};

	FVector2D ScreenMin(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
	FVector2D ScreenMax(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());
	for (const FVector& LocalCorner : LocalCorners)
	{
		const FVector WorldCorner = FramingComponent->GetComponentTransform().TransformPosition(LocalCorner);
		FVector2D ScreenCorner;
		if (!PlayerController->ProjectWorldLocationToScreen(WorldCorner, ScreenCorner, false))
		{
			return false;
		}

		ScreenMin.X = FMath::Min(ScreenMin.X, ScreenCorner.X);
		ScreenMin.Y = FMath::Min(ScreenMin.Y, ScreenCorner.Y);
		ScreenMax.X = FMath::Max(ScreenMax.X, ScreenCorner.X);
		ScreenMax.Y = FMath::Max(ScreenMax.Y, ScreenCorner.Y);
	}

	const float FullWidth = ScreenMax.X - ScreenMin.X;
	const float FullHeight = ScreenMax.Y - ScreenMin.Y;
	const float FullArea = FullWidth * FullHeight;
	if (FullWidth <= KINDA_SMALL_NUMBER || FullHeight <= KINDA_SMALL_NUMBER ||
		FullArea <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float VisibleMinX = FMath::Clamp(ScreenMin.X, 0.0f, static_cast<float>(ViewportWidth));
	const float VisibleMinY = FMath::Clamp(ScreenMin.Y, 0.0f, static_cast<float>(ViewportHeight));
	const float VisibleMaxX = FMath::Clamp(ScreenMax.X, 0.0f, static_cast<float>(ViewportWidth));
	const float VisibleMaxY = FMath::Clamp(ScreenMax.Y, 0.0f, static_cast<float>(ViewportHeight));
	const float VisibleWidth = FMath::Max(VisibleMaxX - VisibleMinX, 0.0f);
	const float VisibleHeight = FMath::Max(VisibleMaxY - VisibleMinY, 0.0f);
	OutCoverageRatio = FMath::Clamp(
		(VisibleWidth * VisibleHeight) / FullArea,
		0.0f,
		1.0f);
	return true;
}

void UBalhwajeomPhotoCameraComponent::UpdateEvidenceFocus(float DeltaTime)
{
	if (!PhotoCamera || !GetWorld())
	{
		ResetEvidenceFocus();
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	if (!PlayerController)
	{
		ResetEvidenceFocus();
		return;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		ResetEvidenceFocus();
		return;
	}

	const FVector CameraLocation = PhotoCamera->GetComponentLocation();
	const FVector CameraForward = PhotoCamera->GetForwardVector();
	const FVector2D ViewportCenter(ViewportWidth * 0.5f, ViewportHeight * 0.5f);
	const float ZoomRatio = SavedFirstPersonFieldOfView /
		FMath::Max(PhotoCamera->FieldOfView, 1.0f);

	AActor* BestTarget = nullptr;
	FBalhwajeomCameraTargetInfo BestInfo;
	FVector2D BestScreenPosition = FVector2D::ZeroVector;
	FVector BestFocusLocation = FVector::ZeroVector;
	FVector BestGuideWorldPosition = FVector::ZeroVector;
	FVector BestGuideWorldNormal = FVector::ZeroVector;
	float BestScreenDistance = TNumericLimits<float>::Max();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate) || Candidate == GetOwner() ||
			!Candidate->GetClass()->ImplementsInterface(UBalhwajeomCameraTargetInterface::StaticClass()))
		{
			continue;
		}

		FBalhwajeomCameraTargetInfo CandidateInfo;
		if (!IBalhwajeomCameraTargetInterface::Execute_RequestCameraTargetInfo(Candidate, CandidateInfo))
		{
			continue;
		}

		const FVector FocusLocation =
			IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(Candidate);
		const FVector ToTarget = FocusLocation - CameraLocation;
		const float TargetDistance = ToTarget.Size();
		if (TargetDistance > FocusTargetScanDistance ||
			FVector::DotProduct(CameraForward, ToTarget.GetSafeNormal()) <= 0.0f)
		{
			continue;
		}

		const float DistanceScale = CandidateInfo.bScaleFocusDistanceWithZoom ? ZoomRatio : 1.0f;
		const float PreferredDistance = CandidateInfo.PreferredFocusDistanceAt1x * DistanceScale;
		const float DistanceTolerance = CandidateInfo.FocusDistanceToleranceAt1x * DistanceScale;
		if (FMath::Abs(TargetDistance - PreferredDistance) > DistanceTolerance)
		{
			continue;
		}

		FVector2D ScreenBoundsMin;
		FVector2D ScreenBoundsMax;
		if (!ProjectActorBoundsToScreen(
			Candidate,
			PlayerController,
			ViewportWidth,
			ViewportHeight,
			ScreenBoundsMin,
			ScreenBoundsMax))
		{
			continue;
		}

		// Use the projected bounds only as a search region. Complex visibility traces
		// then reject holes and empty space inside concave silhouettes.
		FVector2D ScreenPosition;
		FVector GuideWorldPosition;
		FVector GuideWorldNormal;
		if (!FindClosestVisibleSilhouettePoint(
			Candidate,
			PlayerController,
			GetWorld(),
			GetOwner(),
			ViewportCenter,
			ScreenBoundsMin,
			ScreenBoundsMax,
			FocusTargetScanDistance,
			SilhouetteTracePixelStep,
			SilhouetteTraceMaxSamples,
			ScreenPosition,
			GuideWorldPosition,
			GuideWorldNormal))
		{
			continue;
		}

		const float ScreenDistance = FVector2D::Distance(ScreenPosition, ViewportCenter);
		if (ScreenDistance < BestScreenDistance)
		{
			BestScreenDistance = ScreenDistance;
			BestTarget = Candidate;
			BestInfo = MoveTemp(CandidateInfo);
			BestScreenPosition = ScreenPosition;
			BestFocusLocation = GuideWorldPosition;
			BestGuideWorldPosition = GuideWorldPosition;
			BestGuideWorldNormal = GuideWorldNormal;
		}
	}

	ActiveFocusTarget = BestTarget;
	ActiveFocusTargetInfo = BestInfo;
	ActiveFocusScreenPosition = BestScreenPosition;
	bActiveFocusTargetCentered = IsViewportCenterOverTarget(BestTarget);
	bActiveFocusTargetFramedEnough = CalculateTargetFrameCoverage(
		BestTarget, ActiveFocusCoverageRatio) &&
		ActiveFocusCoverageRatio >= FMath::Clamp(MinimumCaptureCoverageRatio, 0.0f, 1.0f);
	bActiveFocusGuideLocationValid = BestTarget != nullptr;
	ActiveFocusGuideLocalPosition = BestTarget
		? BestTarget->GetActorTransform().InverseTransformPosition(BestGuideWorldPosition)
		: FVector::ZeroVector;
	ActiveFocusGuideLocalNormal = BestTarget
		? BestTarget->GetActorTransform().InverseTransformVectorNoScale(BestGuideWorldNormal).GetSafeNormal()
		: FVector::ZeroVector;

	const float DesiredFocalDistance = BestTarget
		? FMath::Max(FVector::DotProduct(BestFocusLocation - CameraLocation, CameraForward), 1.0f)
		: UnfocusedFocalDistance;
	ApplyDepthOfField(DeltaTime, DesiredFocalDistance, BestTarget != nullptr);
}

void UBalhwajeomPhotoCameraComponent::ApplyDepthOfField(
	float DeltaTime,
	float DesiredFocalDistance,
	bool bHasFocusedTarget)
{
	if (!bEnableEvidenceDepthOfField || !PhotoCamera)
	{
		return;
	}

	CurrentFocalDistance = DeltaTime > 0.0f
		? FMath::FInterpTo(CurrentFocalDistance, DesiredFocalDistance, DeltaTime, FocusInterpolationSpeed)
		: DesiredFocalDistance;

	FPostProcessSettings& Settings = PhotoCamera->PostProcessSettings;
	Settings.bOverride_DepthOfFieldFocalDistance = true;
	Settings.bOverride_DepthOfFieldFstop = true;
	Settings.bOverride_DepthOfFieldMinFstop = true;
	Settings.bOverride_DepthOfFieldSensorWidth = true;
	Settings.DepthOfFieldFocalDistance = CurrentFocalDistance;
	Settings.DepthOfFieldFstop = bHasFocusedTarget ? EvidenceFocusFStop : UnfocusedFStop;
	Settings.DepthOfFieldMinFstop = 0.1f;
	Settings.DepthOfFieldSensorWidth = 36.0f;
	PhotoCamera->PostProcessBlendWeight = 1.0f;
}

void UBalhwajeomPhotoCameraComponent::ResetEvidenceFocus()
{
	ActiveFocusTarget.Reset();
	ActiveFocusTargetInfo = FBalhwajeomCameraTargetInfo();
	ActiveFocusScreenPosition = FVector2D::ZeroVector;
	bActiveFocusTargetCentered = false;
	bActiveFocusTargetFramedEnough = false;
	ActiveFocusCoverageRatio = 0.0f;
	ActiveFocusGuideLocalPosition = FVector::ZeroVector;
	ActiveFocusGuideLocalNormal = FVector::ZeroVector;
	bActiveFocusGuideLocationValid = false;
	DisplayedFocusTarget.Reset();
	DisplayedFocusTargetInfo = FBalhwajeomCameraTargetInfo();
	DisplayedFocusGuideLocalPosition = FVector::ZeroVector;
	DisplayedFocusGuideLocalNormal = FVector::ZeroVector;
	bDisplayedFocusGuideLocationValid = false;
	bDisplayedFocusGuideVisibilityValid = false;
}

bool UBalhwajeomPhotoCameraComponent::TryCaptureActiveFocusTarget()
{
	AActor* Target = ActiveFocusTarget.Get();
	if (!Target)
	{
		ShowPhotoFeedback(TEXT("초점이 맞지 않았다."), FColor::Silver);
		return false;
	}

	if (!bActiveFocusTargetFramedEnough)
	{
		const int32 RequiredPercent = FMath::RoundToInt(
			FMath::Clamp(MinimumCaptureCoverageRatio, 0.0f, 1.0f) * 100.0f);
		ShowPhotoFeedback(
			FString::Printf(TEXT("대상을 화면 안에 %d%% 이상 담아야 한다."), RequiredPercent),
			FColor::Yellow);
		return false;
	}

	if (!bActiveFocusTargetCentered)
	{
		ShowPhotoFeedback(TEXT("대상을 화면 중앙에 맞춰야 한다."), FColor::Yellow);
		return false;
	}

	FBalhwajeomCameraTargetInfo TargetInfo;
	if (!IBalhwajeomCameraTargetInterface::Execute_RequestCameraTargetInfo(Target, TargetInfo) ||
		!TargetInfo.bCanBeCaptured || TargetInfo.EvidenceData.EvidenceID.IsNone())
	{
		ShowPhotoFeedback(TEXT("촬영할 수 없는 대상이다."), FColor::Silver);
		return false;
	}

	if (CapturedFocusTargets.Contains(Target) || TargetInfo.EvidenceData.bAlreadyCollected)
	{
		ShowPhotoFeedback(TEXT("이미 기록한 대상이다."), FColor::Yellow);
		return false;
	}

	FBalhwajeomEvidenceData CollectedData = TargetInfo.EvidenceData;
	CollectedData.bAlreadyCollected = true;
	CollectedEvidence.Add(MoveTemp(CollectedData));
	CapturedFocusTargets.Add(Target);

	IBalhwajeomCameraTargetInterface::Execute_NotifyCameraCaptureSucceeded(Target);
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
	{
		if (ABalhwajeomEvidenceCameraHUD* CameraHUD = Cast<ABalhwajeomEvidenceCameraHUD>(PlayerController->GetHUD()))
		{
			CameraHUD->TriggerEvidenceSavedAnimation(TargetInfo.EvidenceData.EvidenceName);
		}
	}
	ShowPhotoFeedback(
		FString::Printf(TEXT("증거 획득: %s"), *TargetInfo.EvidenceData.EvidenceName.ToString()),
		FColor::Green);

	// Keep the guide on the photographed object. The HUD reads the Actor's updated
	// collected state and changes the centered question mark to a check immediately.
	ActiveFocusTargetInfo.EvidenceData.bAlreadyCollected = true;
	DisplayedFocusTargetInfo.EvidenceData.bAlreadyCollected = true;
	return true;
}
