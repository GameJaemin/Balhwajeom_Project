// Fill out your copyright notice in the Description page of Project Settings.

#include "BalhwajeomPhotoCameraComponent.h"
#include "Interaction/ItemInspectionIntegration.h"

#include "Async/Async.h"
#include "CameraSystem/BalhwajeomCameraFocusModel.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/EvidenceDefinitions.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Investigation/PhotoDefinitions.h"
#include "Investigation/SentenceDefinitions.h"
#include "Investigation/WordDefinitions.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "BalhwajeomEvidenceActor.h"
#include "BalhwajeomEvidenceCameraHUD.h"
#include "BalhwajeomCameraTargetInterface.h"
#include "PhotoWorldStoryActor.h"

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

	AActor* ResolveCameraTargetFromHit(AActor* HitActor)
	{
		TSet<AActor*> Visited;
		for (AActor* Candidate = HitActor; IsValid(Candidate) && !Visited.Contains(Candidate);)
		{
			Visited.Add(Candidate);
			if (Candidate->GetClass()->ImplementsInterface(
				UBalhwajeomCameraTargetInterface::StaticClass()))
			{
				return Candidate;
			}

			AActor* Parent = Candidate->GetAttachParentActor();
			Candidate = Parent ? Parent : Candidate->GetOwner();
		}
		return nullptr;
	}

	UTexture2D* CreateCapturePreviewTexture(
		const int32 Width,
		const int32 Height,
		const TArray<FColor>& Colors)
	{
		if (Width <= 0 || Height <= 0 || Colors.Num() != Width * Height)
		{
			return nullptr;
		}

		UTexture2D* Texture = UTexture2D::CreateTransient(
			Width, Height, PF_B8G8R8A8, NAME_None);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty())
		{
			return nullptr;
		}

		Texture->SRGB = true;
		Texture->NeverStream = true;
		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		void* Destination = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Destination, Colors.GetData(), Colors.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}

	bool ProjectPrimitiveBoundsToScreen(
		const UPrimitiveComponent* FramingComponent,
		APlayerController* PlayerController,
		int32 ViewportWidth,
		int32 ViewportHeight,
		FVector2D& OutScreenMin,
		FVector2D& OutScreenMax)
	{
		if (!FramingComponent || !PlayerController)
		{
			return false;
		}

		// Camera targets may own screen-space widgets and helper components whose
		// bounds are unrelated to the photographed silhouette. Only the component
		// explicitly supplied by RequestCameraFramingComponent is considered.
		const FBox Bounds = FramingComponent->Bounds.GetBox();
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
				Params) && ResolveCameraTargetFromHit(Hit.GetActor()) == Actor;
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
	PhotoWorldStoryClass = APhotoWorldStoryActor::StaticClass();
	FocusPrefilterMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusPrefilter.M_PP_CameraFocusPrefilter")));
	FocusBlurMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusBlur.M_PP_CameraFocusBlur")));
	FocusNearHorizontalMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearHorizontal.M_PP_CameraFocusNearHorizontal")));
	FocusNearVerticalMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusNearVertical.M_PP_CameraFocusNearVertical")));
	FocusCompositeMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Camera/Materials/M_PP_CameraFocusComposite.M_PP_CameraFocusComposite")));
}

void UBalhwajeomPhotoCameraComponent::BeginDestroy()
{
	SetCameraUIHiddenForScreenshot(false);
	ClearScreenshotDelegates();
	if (PendingCapture.IsSet())
	{
		if (FScreenshotRequest::IsScreenshotRequested() &&
			FPaths::IsSamePath(
				FScreenshotRequest::GetFilename(),
				PendingCapture->AbsolutePath))
		{
			FScreenshotRequest::Reset();
		}
		IFileManager::Get().Delete(*PendingCapture->AbsolutePath, false, true);
	}
	PendingCapture.Reset();
	Super::BeginDestroy();
}

void UBalhwajeomPhotoCameraComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsInCameraMode && bEnableEvidenceFocusSystem)
	{
		UpdateEvidenceFocus(DeltaTime);
		RefreshDisplayedGuideSnapshot();
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
	if (BalhwajeomItemInspection::IsOpen(GetOwner())) return;
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

	// The shutter always works. Evidence collection, however, requires a visible target
	// inside the focus-distance band and the center guide.
	if (bEnableEvidenceFocusSystem)
	{
		UpdateEvidenceFocus(0.0f);
		// A successful evidence shot flashes only after the viewport pixels have been
		// copied. Otherwise the white shutter overlay becomes the saved photograph.
		if (!TryCaptureActiveFocusTarget())
		{
			TriggerPhotoFlash();
		}
		return;
	}

	TriggerPhotoFlash();

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

	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	if (PlayerController)
	{
		SavedExplorationControlRotation = PlayerController->GetControlRotation();
		bHasSavedExplorationControlRotation = true;

		FVector OutgoingViewLocation;
		FRotator OutgoingViewRotation;
		PlayerController->GetPlayerViewPoint(OutgoingViewLocation, OutgoingViewRotation);

		// Aim the first-person camera at the world point under the third-person screen center.
		// The cameras have different origins, so copying only their rotation causes parallax and
		// pushes the object sideways on entry.
		const FVector OutgoingViewDirection = OutgoingViewRotation.Vector();
		FVector CenterTarget = OutgoingViewLocation + OutgoingViewDirection * WORLD_MAX;
		bool bFoundCenterTarget = false;
		if (UWorld* World = GetWorld())
		{
			FCollisionQueryParams QueryParams(
				SCENE_QUERY_STAT(PhotoCameraModeCenterHandoff), true, GetOwner());
			QueryParams.bTraceComplex = true;
			FHitResult CenterHit;
			if (World->LineTraceSingleByChannel(
				CenterHit,
				OutgoingViewLocation,
				CenterTarget,
				ECC_Visibility,
				QueryParams))
			{
				CenterTarget = CenterHit.ImpactPoint;
				bFoundCenterTarget = true;
			}
		}

		const FVector PhotoViewDirection = CenterTarget - PhotoCamera->GetComponentLocation();
		PlayerController->SetControlRotation(
			!bFoundCenterTarget || PhotoViewDirection.IsNearlyZero()
				? OutgoingViewRotation
				: PhotoViewDirection.Rotation());
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

	if (PlayerController)
	{
		// Reclaim the view target from an active FixedCameraZone so camera mode is always visible,
		// even while standing inside a zone. The switch happens while the screen is faded to black.
		// Control rotation was aligned above so the outgoing center object remains in view.
		PlayerController->SetViewTargetWithBlend(GetOwner(), 0.0f);
	}

	if (bEnableEvidenceFocusSystem)
	{
		CurrentFocusRegion = FBalhwajeomCameraFocusModel::CalculateDefaultRegion(
			MinimumFocusDistance, MaximumFocusDistance);
		CurrentMaximumBlurStrength = FMath::Clamp(MaximumBlurStrength, 0.0f, 1.0f);
		FocusGraceState.OnStrictTargetFound();
		InitializeFocusBlurMaterials();
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

	// Restore the exploration rotation captured before center-target alignment. Without this,
	// entering and leaving camera mode repeatedly feeds the parallax correction back into the
	// third-person view and makes it drift on every RMB press.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
		PlayerController && bHasSavedExplorationControlRotation)
	{
		PlayerController->SetControlRotation(SavedExplorationControlRotation);
	}
	bHasSavedExplorationControlRotation = false;

	if (PhotoCamera)
	{
		PhotoCamera->SetRelativeTransform(SavedFirstPersonRelativeTransform);
		PhotoCamera->SetFieldOfView(SavedFirstPersonFieldOfView);
		if (bEnableEvidenceFocusSystem)
		{
			PhotoCamera->PostProcessSettings = SavedPhotoPostProcessSettings;
			PhotoCamera->PostProcessBlendWeight = SavedPostProcessBlendWeight;
			FocusPrefilterMaterialInstance = nullptr;
			FocusBlurMaterialInstance = nullptr;
			FocusNearHorizontalMaterialInstance = nullptr;
			FocusNearVerticalMaterialInstance = nullptr;
			FocusCompositeMaterialInstance = nullptr;
			bFocusBlurInitializationFailed = false;
		}
	}
	CameraPanWorldOffset = FVector::ZeroVector;
	FocusGuideTraceElapsed = 0.0f;
	EarlyGuideRescanElapsed = 0.0f;
	SetComponentTickEnabled(false);
	ResetEvidenceFocus();

	bIsInCameraMode = false;
	if (ActivePhotoWorldStory.IsValid())
	{
		ActivePhotoWorldStory->TransitionToThirdPersonScale();
	}
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
	OnCameraTransitionFinished.Broadcast();
}

void UBalhwajeomPhotoCameraComponent::PanCamera(const FVector& ScreenDirection, float Value)
{
	if (!bIsInCameraMode || bIsCameraTransitioning || !PhotoCamera || !GetWorld() || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FVector Delta = ScreenDirection.GetSafeNormal() * Value * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
	CameraPanWorldOffset += Delta;

	// Clamp each axis independently. A vector-length clamp creates a circular
	// boundary; independent horizontal/vertical limits create a square pan area.
	const float PanLimit = FMath::Max(CameraPanMaxDistance, 0.0f);
	const float HorizontalOffset = FMath::Clamp(
		FVector::DotProduct(CameraPanWorldOffset, CameraPanRightDirection),
		-PanLimit,
		PanLimit);
	const float VerticalOffset = FMath::Clamp(
		FVector::DotProduct(CameraPanWorldOffset, FVector::UpVector),
		-PanLimit,
		PanLimit);
	CameraPanWorldOffset =
		CameraPanRightDirection * HorizontalOffset + FVector::UpVector * VerticalOffset;
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
	if (!bEnableEvidenceFocusSystem || !DisplayedFocusTarget.IsValid())
	{
		OutOpacity = 0.0f;
		return false;
	}

	AActor* DisplayedTarget = DisplayedFocusTarget.Get();
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	if (!DisplayedTarget || !PlayerController ||
		!DisplayedTarget->GetClass()->ImplementsInterface(
			UBalhwajeomCameraTargetInterface::StaticClass()))
	{
		OutOpacity = 0.0f;
		return false;
	}

	const FVector FocusWorldPosition =
		IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(DisplayedTarget);
	if (!PlayerController->ProjectWorldLocationToScreen(
		FocusWorldPosition, OutScreenPosition, false))
	{
		OutOpacity = 0.0f;
		return false;
	}

	bOutIsCentered = bHasStrictFocusTarget && ActiveFocusTarget.Get() == DisplayedTarget;
	OutTargetInfo = DisplayedFocusTargetInfo;
	OutOpacity = 1.0f;
	return true;
}

void UBalhwajeomPhotoCameraComponent::RefreshDisplayedGuideSnapshot()
{
	DisplayedFocusTarget = ActiveFocusTarget;
	DisplayedFocusTargetInfo = ActiveFocusTargetInfo;
	bDisplayedFocusGuideLocationValid = ActiveFocusTarget.IsValid();
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
	FVector CameraLocation;
	FVector CameraForward;
	if (!GetEffectiveCameraView(CameraLocation, CameraForward))
	{
		return false;
	}
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

	return ResolveCameraTargetFromHit(Hit.GetActor()) == Target &&
		FVector::Distance(Hit.ImpactPoint, GuideWorldPosition) <= GuideVisibilityImpactTolerance;
}

void UBalhwajeomPhotoCameraComponent::TriggerPhotoFlash() const
{
	if (const APlayerController* PlayerController =
		Cast<APlayerController>(GetOwningController(this)))
	{
		if (ABalhwajeomEvidenceCameraHUD* CameraHUD =
			Cast<ABalhwajeomEvidenceCameraHUD>(PlayerController->GetHUD()))
		{
			CameraHUD->TriggerPhotoFlash();
		}
	}
}

bool UBalhwajeomPhotoCameraComponent::GetEffectiveCameraView(
	FVector& OutLocation,
	FVector& OutForward) const
{
	if (APlayerController* PlayerController =
		Cast<APlayerController>(GetOwningController(this)))
	{
		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(OutLocation, ViewRotation);
		OutForward = ViewRotation.Vector();
		return !OutForward.IsNearlyZero();
	}

	if (PhotoCamera)
	{
		OutLocation = PhotoCamera->GetComponentLocation();
		OutForward = PhotoCamera->GetForwardVector();
		return true;
	}
	return false;
}

bool UBalhwajeomPhotoCameraComponent::TraceViewportCenter(FHitResult& OutHit) const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
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
	return World->LineTraceSingleByChannel(
		OutHit,
		RayOrigin,
		RayOrigin + RayDirection * WORLD_MAX,
		ECC_Visibility,
		Params);
}

bool UBalhwajeomPhotoCameraComponent::IsViewportCenterOverTarget(const AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	FHitResult Hit;
	return TraceViewportCenter(Hit) && ResolveCameraTargetFromHit(Hit.GetActor()) == Target;
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
	FBalhwajeomStrictFocusTarget StrictTarget;
	bHasStrictFocusTarget = FindStrictFocusTarget(StrictTarget);
	if (bHasStrictFocusTarget)
	{
		ActiveFocusTarget = StrictTarget.Target;
		ActiveFocusTargetInfo = StrictTarget.TargetInfo;
		ActiveFocusScreenPosition = FVector2D::ZeroVector;
		bActiveFocusTargetCentered = true;
		FocusGraceState.OnStrictTargetFound();
	}
	else
	{
		bActiveFocusTargetCentered = false;
		const bool bRetainVisualFocus = ActiveFocusTarget.IsValid() &&
			FocusGraceState.ShouldRetainAfterMiss(DeltaTime, FocusTargetGracePeriod);
		if (!bRetainVisualFocus)
		{
			ActiveFocusTarget.Reset();
			ActiveFocusTargetInfo = FBalhwajeomCameraTargetInfo{};
		}
	}

	FBalhwajeomFocusRegion DesiredRegion =
		FBalhwajeomCameraFocusModel::CalculateDefaultRegion(
			MinimumFocusDistance,
			MaximumFocusDistance);
	if (AActor* VisualTarget = ActiveFocusTarget.Get())
	{
		FVector CameraLocation;
		FVector CameraForward;
		if (GetEffectiveCameraView(CameraLocation, CameraForward))
		{
			const FVector FocusLocation =
				IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(VisualTarget);
			DesiredRegion = FBalhwajeomCameraFocusModel::CalculateFocusedRegion(
				FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(
					CameraLocation,
					CameraForward,
					FocusLocation),
				BlurStartDistance);
		}
	}
	ApplyFocusBlur(DeltaTime, DesiredRegion);
}

bool UBalhwajeomPhotoCameraComponent::FindStrictFocusTarget(
	FBalhwajeomStrictFocusTarget& OutTarget) const
{
	OutTarget = FBalhwajeomStrictFocusTarget{};
	FHitResult CenterHit;
	if (!TraceViewportCenter(CenterHit))
	{
		return false;
	}

	AActor* Target = ResolveCameraTargetFromHit(CenterHit.GetActor());
	if (!IsValid(Target))
	{
		return false;
	}

	FBalhwajeomCameraTargetInfo TargetInfo;
	if (!IBalhwajeomCameraTargetInterface::Execute_RequestCameraTargetInfo(Target, TargetInfo))
	{
		return false;
	}

	float MinimumOffset = TargetInfo.MinimumFocusDistanceOffset;
	float MaximumOffset = TargetInfo.MaximumFocusDistanceOffset;
	FBalhwajeomResolvedPhotoTarget ResolvedTarget;
	if (ResolveInvestigationTarget(TargetInfo, ResolvedTarget))
	{
		TargetInfo.PhotoID = ResolvedTarget.PhotoID;
		TargetInfo.bCanCapture = ResolvedTarget.bCanCapture;
		TargetInfo.MinimumFocusDistanceOffset = ResolvedTarget.MinimumFocusDistanceOffset;
		TargetInfo.MaximumFocusDistanceOffset = ResolvedTarget.MaximumFocusDistanceOffset;
		MinimumOffset = ResolvedTarget.MinimumFocusDistanceOffset;
		MaximumOffset = ResolvedTarget.MaximumFocusDistanceOffset;
	}

	FBalhwajeomFocusRange EffectiveRange;
	if (!FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
		MinimumFocusDistance,
		MaximumFocusDistance,
		MinimumOffset,
		MaximumOffset,
		EffectiveRange))
	{
		return false;
	}

	FVector CameraLocation;
	FVector CameraForward;
	if (!GetEffectiveCameraView(CameraLocation, CameraForward))
	{
		return false;
	}

	const FVector FocusLocation =
		IBalhwajeomCameraTargetInterface::Execute_RequestCameraFocusLocation(Target);
	const float FocusDistance = FVector::Distance(CameraLocation, FocusLocation);
	if (!EffectiveRange.Contains(FocusDistance))
	{
		return false;
	}

	OutTarget.Target = Target;
	OutTarget.TargetInfo = MoveTemp(TargetInfo);
	OutTarget.FocusLocation = FocusLocation;
	OutTarget.FocusDistance = FocusDistance;
	return true;
}

void UBalhwajeomPhotoCameraComponent::InitializeFocusBlurMaterials()
{
	const bool bAllReady = FocusPrefilterMaterialInstance && FocusBlurMaterialInstance &&
		FocusNearHorizontalMaterialInstance && FocusNearVerticalMaterialInstance &&
		FocusCompositeMaterialInstance;
	if (!bEnableEvidenceFocusBlur || !PhotoCamera || bAllReady || bFocusBlurInitializationFailed)
	{
		return;
	}

	UMaterialInterface* PrefilterBase = FocusPrefilterMaterial.LoadSynchronous();
	UMaterialInterface* BlurBase = FocusBlurMaterial.LoadSynchronous();
	UMaterialInterface* NearHorizontalBase = FocusNearHorizontalMaterial.LoadSynchronous();
	UMaterialInterface* NearVerticalBase = FocusNearVerticalMaterial.LoadSynchronous();
	UMaterialInterface* CompositeBase = FocusCompositeMaterial.LoadSynchronous();
	if (!PrefilterBase || !BlurBase || !NearHorizontalBase || !NearVerticalBase || !CompositeBase)
	{
		bFocusBlurInitializationFailed = true;
		UE_LOG(LogTemp, Warning,
			TEXT("Camera focus blur disabled because one or more material assets could not load: %s, %s, %s, %s, %s"),
			*FocusPrefilterMaterial.ToSoftObjectPath().ToString(),
			*FocusBlurMaterial.ToSoftObjectPath().ToString(),
			*FocusNearHorizontalMaterial.ToSoftObjectPath().ToString(),
			*FocusNearVerticalMaterial.ToSoftObjectPath().ToString(),
			*FocusCompositeMaterial.ToSoftObjectPath().ToString());
		return;
	}

	UMaterialInstanceDynamic* PrefilterInstance =
		UMaterialInstanceDynamic::Create(PrefilterBase, this);
	UMaterialInstanceDynamic* BlurInstance =
		UMaterialInstanceDynamic::Create(BlurBase, this);
	UMaterialInstanceDynamic* NearHorizontalInstance =
		UMaterialInstanceDynamic::Create(NearHorizontalBase, this);
	UMaterialInstanceDynamic* NearVerticalInstance =
		UMaterialInstanceDynamic::Create(NearVerticalBase, this);
	UMaterialInstanceDynamic* CompositeInstance =
		UMaterialInstanceDynamic::Create(CompositeBase, this);
	if (!PrefilterInstance || !BlurInstance || !NearHorizontalInstance ||
		!NearVerticalInstance || !CompositeInstance)
	{
		bFocusBlurInitializationFailed = true;
		UE_LOG(LogTemp, Warning,
			TEXT("Camera focus blur disabled because its complete five-pass material chain could not be initialized."));
		return;
	}

	FocusPrefilterMaterialInstance = PrefilterInstance;
	FocusBlurMaterialInstance = BlurInstance;
	FocusNearHorizontalMaterialInstance = NearHorizontalInstance;
	FocusNearVerticalMaterialInstance = NearVerticalInstance;
	FocusCompositeMaterialInstance = CompositeInstance;
	PhotoCamera->AddOrUpdateBlendable(FocusPrefilterMaterialInstance, 1.0f);
	PhotoCamera->AddOrUpdateBlendable(FocusBlurMaterialInstance, 1.0f);
	PhotoCamera->AddOrUpdateBlendable(FocusNearHorizontalMaterialInstance, 1.0f);
	PhotoCamera->AddOrUpdateBlendable(FocusNearVerticalMaterialInstance, 1.0f);
	PhotoCamera->AddOrUpdateBlendable(FocusCompositeMaterialInstance, 1.0f);
}

void UBalhwajeomPhotoCameraComponent::ApplyFocusBlur(
	const float DeltaTime,
	const FBalhwajeomFocusRegion& DesiredRegion)
{
	if (!bEnableEvidenceFocusBlur || !PhotoCamera)
	{
		return;
	}

	InitializeFocusBlurMaterials();
	const float Speed = FMath::Max(0.0f, FocusApplicationSpeed);
	CurrentFocusRegion = FBalhwajeomCameraFocusModel::InterpolateRegion(
		CurrentFocusRegion, DesiredRegion, DeltaTime, Speed);
	if (DeltaTime > 0.0f && Speed > 0.0f)
	{
		CurrentMaximumBlurStrength = FMath::FInterpTo(
			CurrentMaximumBlurStrength,
			FMath::Clamp(MaximumBlurStrength, 0.0f, 1.0f),
			DeltaTime,
			Speed);
	}
	else
	{
		CurrentMaximumBlurStrength = FMath::Clamp(MaximumBlurStrength, 0.0f, 1.0f);
	}

	if (!FocusPrefilterMaterialInstance || !FocusBlurMaterialInstance ||
		!FocusNearHorizontalMaterialInstance || !FocusNearVerticalMaterialInstance ||
		!FocusCompositeMaterialInstance)
	{
		return;
	}

	auto SetFocusParameters = [this](UMaterialInstanceDynamic* MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(
			TEXT("SharpNearDistance"), CurrentFocusRegion.SharpNear);
		MaterialInstance->SetScalarParameterValue(
			TEXT("SharpFarDistance"), CurrentFocusRegion.SharpFar);
		MaterialInstance->SetScalarParameterValue(
			TEXT("BlurTransitionDistance"), FMath::Max(0.0f, BlurTransitionDistance));
		MaterialInstance->SetScalarParameterValue(
			TEXT("MaximumBlurStrength"), CurrentMaximumBlurStrength);
		MaterialInstance->SetScalarParameterValue(
			TEXT("MaximumBlurRadiusPixels"), FMath::Max(0.0f, MaximumBlurRadiusPixels));
		MaterialInstance->SetScalarParameterValue(
			TEXT("NearBlurRadiusScale"), FMath::Max(0.0f, NearBlurRadiusScale));
		MaterialInstance->SetScalarParameterValue(
			TEXT("FarBlurRadiusScale"), FMath::Max(0.0f, FarBlurRadiusScale));
	};
	SetFocusParameters(FocusPrefilterMaterialInstance);
	SetFocusParameters(FocusBlurMaterialInstance);
	SetFocusParameters(FocusNearHorizontalMaterialInstance);
	SetFocusParameters(FocusNearVerticalMaterialInstance);
	SetFocusParameters(FocusCompositeMaterialInstance);

	FPostProcessSettings& Settings = PhotoCamera->PostProcessSettings;
	Settings.bOverride_DepthOfFieldFocalDistance = true;
	Settings.DepthOfFieldFocalDistance = 0.0f;
	PhotoCamera->PostProcessBlendWeight = 1.0f;
}

void UBalhwajeomPhotoCameraComponent::ResetEvidenceFocus()
{
	bHasStrictFocusTarget = false;
	FocusGraceState.OnStrictTargetFound();
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
	// Shutter validation is always fresh and strict. The visual 0.1-second grace
	// may keep the guide/blur stable, but can never authorize a capture.
	FBalhwajeomStrictFocusTarget StrictTarget;
	if (!FindStrictFocusTarget(StrictTarget))
	{
		ShowPhotoFeedback(TEXT("초점이 맞지 않았다."), FColor::Silver);
		return false;
	}
	AActor* Target = StrictTarget.Target.Get();

	const FBalhwajeomCameraTargetInfo& TargetInfo = StrictTarget.TargetInfo;

	FBalhwajeomResolvedPhotoTarget ResolvedTarget;
	if (ResolveInvestigationTarget(TargetInfo, ResolvedTarget))
	{
		if (!ResolvedTarget.bCanCapture || ResolvedTarget.PhotoID.IsNone())
		{
			ShowPhotoFeedback(TEXT("현재 상태에서는 촬영할 수 없다."), FColor::Silver);
			return false;
		}

		if (PendingCapture.IsSet())
		{
			ShowPhotoFeedback(TEXT("이전 사진을 저장하고 있다."), FColor::Yellow);
			return false;
		}

		UBalhwajeomInvestigationSubsystem* InvestigationSubsystem =
			GetInvestigationSubsystem();
		if (!InvestigationSubsystem)
		{
			ShowPhotoFeedback(TEXT("사진 시스템을 사용할 수 없다."), FColor::Red);
			return false;
		}

		if (InvestigationSubsystem->HasCapturedPhoto(ResolvedTarget.PhotoID))
		{
			ShowPhotoFeedback(TEXT("이미 기록한 사진이다."), FColor::Yellow);
			return false;
		}

		return BeginInvestigationImageCapture(ResolvedTarget);
	}

	// Compatibility path for evidence Blueprints that have not yet migrated to the
	// Investigation IDs. This path is removed after the owner of EvidenceActor
	// supplies EvidenceInstanceID/ObjectID/StateID.
	if (!TargetInfo.bCanBeCaptured || TargetInfo.EvidenceData.EvidenceID.IsNone())
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

bool UBalhwajeomPhotoCameraComponent::ResolveInvestigationTarget(
	const FBalhwajeomCameraTargetInfo& TargetInfo,
	FBalhwajeomResolvedPhotoTarget& OutTarget) const
{
	OutTarget = FBalhwajeomResolvedPhotoTarget{};
	if (!TargetInfo.EvidenceInstanceID.IsValid() ||
		TargetInfo.ObjectID.IsNone() ||
		TargetInfo.StateID.IsNone())
	{
		return false;
	}

	UBalhwajeomInvestigationSubsystem* InvestigationSubsystem =
		GetInvestigationSubsystem();
	if (!InvestigationSubsystem)
	{
		return false;
	}

	FEvidenceStateDefinition StateDefinition;
	if (!InvestigationSubsystem->GetEvidenceStateDefinition(
		TargetInfo.StateID,
		StateDefinition) ||
		StateDefinition.ObjectID != TargetInfo.ObjectID)
	{
		return false;
	}

	OutTarget.EvidenceInstanceID = TargetInfo.EvidenceInstanceID;
	OutTarget.ObjectID = TargetInfo.ObjectID;
	OutTarget.StateID = TargetInfo.StateID;
	OutTarget.PhotoID = StateDefinition.PhotoID;
	OutTarget.bCanCapture = StateDefinition.bCanCapture;
	OutTarget.MinimumFocusDistanceOffset = StateDefinition.MinimumFocusDistanceOffset;
	OutTarget.MaximumFocusDistanceOffset = StateDefinition.MaximumFocusDistanceOffset;
	return true;
}

UBalhwajeomInvestigationSubsystem*
UBalhwajeomPhotoCameraComponent::GetInvestigationSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance
		? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
		: nullptr;
}

bool UBalhwajeomPhotoCameraComponent::BeginInvestigationImageCapture(
	const FBalhwajeomResolvedPhotoTarget& Target)
{
	if (PendingCapture.IsSet() || FScreenshotRequest::IsScreenshotRequested())
	{
		ShowPhotoFeedback(TEXT("다른 사진 저장 요청이 진행 중이다."), FColor::Yellow);
		return false;
	}

	const FGuid RequestID = FGuid::NewGuid();
	const FString SafePhotoID = FPaths::MakeValidFileName(Target.PhotoID.ToString(), TEXT('_'));
	const FString FileName = FString::Printf(TEXT("%s.png"), *SafePhotoID);
	FString RelativeDirectory = FPaths::Combine(TEXT("Investigation"), TEXT("Photos"));
#if WITH_DEV_AUTOMATION_TESTS
	if (GIsAutomationTesting)
	{
		RelativeDirectory = FPaths::Combine(RelativeDirectory, TEXT("Automation"));
	}
#endif
	const FString RelativePath = FPaths::Combine(RelativeDirectory, FileName);
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectSavedDir(), RelativePath));

	if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsolutePath), true))
	{
		ShowPhotoFeedback(TEXT("사진 저장 폴더를 만들지 못했다."), FColor::Red);
		return false;
	}

	FBalhwajeomPendingPhotoCapture NewCapture;
	NewCapture.RequestID = RequestID;
	NewCapture.TargetSnapshot = Target;
	NewCapture.RequestedTime = FDateTime::UtcNow();
	NewCapture.RelativePath = RelativePath;
	NewCapture.AbsolutePath = AbsolutePath;
	NewCapture.bHasStorySpawnTransform = CalculateStorySpawnTransform(NewCapture.StorySpawnTransform);
	PendingCapture = MoveTemp(NewCapture);
	PendingCapturePreviewTexture = nullptr;
	bReceivedScreenshotPixels = false;

	ScreenshotCapturedHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(
		this,
		&UBalhwajeomPhotoCameraComponent::HandleScreenshotCaptured);
	ScreenshotProcessedHandle = FScreenshotRequest::OnScreenshotRequestProcessed().AddUObject(
		this,
		&UBalhwajeomPhotoCameraComponent::HandleScreenshotProcessed);

	// Draw the requested frame without the first-person camera overlay.  The HUD
	// is restored as soon as the clean back-buffer pixels arrive.
	SetCameraUIHiddenForScreenshot(true);
	FScreenshotRequest::RequestScreenshot(
		AbsolutePath,
		false,
		false);

	if (!FScreenshotRequest::IsScreenshotRequested())
	{
		SetCameraUIHiddenForScreenshot(false);
		ClearScreenshotDelegates();
		PendingCapture.Reset();
		ShowPhotoFeedback(TEXT("사진 캡처를 요청하지 못했다."), FColor::Red);
		return false;
	}

	return true;
}

void UBalhwajeomPhotoCameraComponent::HandleScreenshotCaptured(
	int32 Width,
	int32 Height,
	const TArray<FColor>& Colors)
{
	if (!PendingCapture.IsSet() || bReceivedScreenshotPixels)
	{
		return;
	}

	bReceivedScreenshotPixels = true;
	SetCameraUIHiddenForScreenshot(false);
	PendingCapturePreviewTexture = CreateCapturePreviewTexture(
		Width, Height, Colors);
	const FGuid RequestID = PendingCapture->RequestID;
	const FString AbsolutePath = PendingCapture->AbsolutePath;
	ClearScreenshotDelegates();

	// The pixels above represent the unflashed viewport. Start the visual shutter
	// response now so it remains visible to the player but cannot contaminate the PNG.
	TriggerPhotoFlash();
	ShowPhotoFeedback(TEXT("사진을 저장하고 있다."), FColor::Silver);

	if (Width <= 0 || Height <= 0 || Colors.Num() != Width * Height)
	{
		CompleteImageSave(RequestID, false, AbsolutePath);
		return;
	}

	TArray<FColor> OwnedPixels = Colors;
	TWeakObjectPtr<UBalhwajeomPhotoCameraComponent> WeakThis(this);
	Async(EAsyncExecution::ThreadPool,
		[WeakThis, RequestID, AbsolutePath, Width, Height, Pixels = MoveTemp(OwnedPixels)]() mutable
		{
			const FImageView Image(Pixels.GetData(), Width, Height);
			const bool bSaved = FImageUtils::SaveImageByExtension(*AbsolutePath, Image);
			AsyncTask(ENamedThreads::GameThread,
				[WeakThis, RequestID, AbsolutePath, bSaved]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->CompleteImageSave(RequestID, bSaved, AbsolutePath);
					}
					else if (bSaved)
					{
						IFileManager::Get().Delete(*AbsolutePath, false, true);
					}
				});
		});
}

void UBalhwajeomPhotoCameraComponent::HandleScreenshotProcessed()
{
	if (!PendingCapture.IsSet() || bReceivedScreenshotPixels)
	{
		return;
	}

	const FGuid RequestID = PendingCapture->RequestID;
	const FString AbsolutePath = PendingCapture->AbsolutePath;
	SetCameraUIHiddenForScreenshot(false);
	ClearScreenshotDelegates();
	CompleteImageSave(RequestID, false, AbsolutePath);
}

void UBalhwajeomPhotoCameraComponent::CompleteImageSave(
	FGuid RequestID,
	bool bSucceeded,
	const FString& AbsolutePath)
{
	SetCameraUIHiddenForScreenshot(false);
	if (!PendingCapture.IsSet() || PendingCapture->RequestID != RequestID)
	{
		if (bSucceeded)
		{
			IFileManager::Get().Delete(*AbsolutePath, false, true);
		}
		return;
	}

	const FBalhwajeomPendingPhotoCapture CompletedCapture = PendingCapture.GetValue();
	PendingCapture.Reset();
	bReceivedScreenshotPixels = false;

	if (!bSucceeded ||
		IFileManager::Get().FileSize(*CompletedCapture.AbsolutePath) <= 0)
	{
		PendingCapturePreviewTexture = nullptr;
		IFileManager::Get().Delete(*CompletedCapture.AbsolutePath, false, true);
		ShowPhotoFeedback(TEXT("사진 이미지 저장에 실패했다."), FColor::Red);
		return;
	}

	UBalhwajeomInvestigationSubsystem* InvestigationSubsystem =
		GetInvestigationSubsystem();
	if (!InvestigationSubsystem)
	{
		PendingCapturePreviewTexture = nullptr;
		IFileManager::Get().Delete(*CompletedCapture.AbsolutePath, false, true);
		ShowPhotoFeedback(TEXT("사진 시스템을 사용할 수 없다."), FColor::Red);
		return;
	}

	FCapturedPhotoRecord Record;
	Record.PhotoID = CompletedCapture.TargetSnapshot.PhotoID;
	Record.ObjectID = CompletedCapture.TargetSnapshot.ObjectID;
	Record.EvidenceInstanceID = CompletedCapture.TargetSnapshot.EvidenceInstanceID;
	Record.CapturedStateID = CompletedCapture.TargetSnapshot.StateID;
	Record.ImageRelativePath = CompletedCapture.RelativePath;
	Record.CapturedTime = CompletedCapture.RequestedTime;
	Record.bViewedInTablet = false;

	TArray<FName> NewlyGrantedWordIDs;
	FPhotoDefinition PhotoDefinition;
	if (InvestigationSubsystem->GetPhotoDefinition(Record.PhotoID, PhotoDefinition))
	{
		for (const FName WordID : PhotoDefinition.GrantedWordIDs)
		{
			if (!InvestigationSubsystem->HasAcquiredWord(WordID))
			{
				NewlyGrantedWordIDs.Add(WordID);
			}
		}
	}

	if (!InvestigationSubsystem->RegisterCapturedPhoto(Record))
	{
		PendingCapturePreviewTexture = nullptr;
		IFileManager::Get().Delete(*CompletedCapture.AbsolutePath, false, true);
		ShowPhotoFeedback(TEXT("현재 상태가 바뀌어 사진을 등록하지 못했다."), FColor::Yellow);
		return;
	}

	if (!PhotoDefinition.PhotoID.IsNone())
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this)))
		{
			if (ABalhwajeomEvidenceCameraHUD* CameraHUD = Cast<ABalhwajeomEvidenceCameraHUD>(PlayerController->GetHUD()))
			{
				FText CaptureSentence = PhotoDefinition.CustomDescription;
				if (!PhotoDefinition.PhotoSentenceID.IsNone())
				{
					FSentenceDefinition SentenceDefinition;
					if (InvestigationSubsystem->GetSentenceDefinition(
						PhotoDefinition.PhotoSentenceID, SentenceDefinition))
					{
						CaptureSentence = SentenceDefinition.SentenceTemplate;
					}
				}
				if (CaptureSentence.IsEmpty())
				{
					CaptureSentence = PhotoDefinition.PhotoName;
				}

				TArray<FText> GrantedKeywordTexts;
				for (const FName WordID : NewlyGrantedWordIDs)
				{
					FWordDefinition WordDefinition;
					if (InvestigationSubsystem->GetWordDefinition(WordID, WordDefinition))
					{
						GrantedKeywordTexts.Add(WordDefinition.DisplayWord);
					}
				}

				CameraHUD->TriggerCapturePhotoPresentation(
					PendingCapturePreviewTexture, CaptureSentence, GrantedKeywordTexts);
			}
		}

		if (CompletedCapture.bHasStorySpawnTransform)
		{
			StartPhotoWorldStory(PhotoDefinition, CompletedCapture.StorySpawnTransform);
		}
	}
	PendingCapturePreviewTexture = nullptr;

	ShowPhotoFeedback(TEXT("사진을 기록했다."), FColor::Green);
}

bool UBalhwajeomPhotoCameraComponent::CalculateStorySpawnTransform(FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningController(this));
	if (!PlayerController)
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
		ViewportWidth * PhotoStoryScreenXRatio,
		ViewportHeight * PhotoStoryScreenYRatio,
		RayOrigin,
		RayDirection))
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector StoryLocation = RayOrigin + RayDirection * PhotoStoryDisplayDistance;
	const FRotator StoryRotation = (CameraLocation - StoryLocation).Rotation();
	OutTransform = FTransform(StoryRotation, StoryLocation);
	return true;
}

void UBalhwajeomPhotoCameraComponent::StartPhotoWorldStory(
	const FPhotoDefinition& PhotoDefinition,
	const FTransform& SpawnTransform)
{
	if (!GetWorld() || !PhotoWorldStoryClass ||
		(PhotoDefinition.WorldStoryCues.IsEmpty() && PhotoDefinition.WorldStoryLines.IsEmpty()))
	{
		return;
	}

	if (ActivePhotoWorldStory.IsValid())
	{
		ActivePhotoWorldStory->StopStory();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APhotoWorldStoryActor* StoryActor = GetWorld()->SpawnActor<APhotoWorldStoryActor>(
		PhotoWorldStoryClass,
		SpawnTransform,
		SpawnParameters);
	if (!StoryActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn photo world story for '%s'."),
			*PhotoDefinition.PhotoID.ToString());
		return;
	}

	ActivePhotoWorldStory = StoryActor;
	StoryActor->StartStory(
		PhotoDefinition.WorldStoryCues,
		PhotoDefinition.WorldStoryLines,
		PhotoDefinition.StoryVoice);

	// Screenshot processing may finish after the player has already left photo mode.
	if (!bIsInCameraMode && IsValid(StoryActor))
	{
		StoryActor->TransitionToThirdPersonScale();
	}
}

void UBalhwajeomPhotoCameraComponent::ClearScreenshotDelegates()
{
	if (ScreenshotCapturedHandle.IsValid())
	{
		UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotCapturedHandle);
		ScreenshotCapturedHandle.Reset();
	}

	if (ScreenshotProcessedHandle.IsValid())
	{
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotProcessedHandle);
		ScreenshotProcessedHandle.Reset();
	}
}

void UBalhwajeomPhotoCameraComponent::SetCameraUIHiddenForScreenshot(const bool bHidden) const
{
	const APlayerController* PlayerController =
		Cast<APlayerController>(GetOwningController(this));
	if (ABalhwajeomEvidenceCameraHUD* CameraHUD = PlayerController
		? Cast<ABalhwajeomEvidenceCameraHUD>(PlayerController->GetHUD())
		: nullptr)
	{
		CameraHUD->SetCaptureUIHiddenForScreenshot(bHidden);
	}
}
