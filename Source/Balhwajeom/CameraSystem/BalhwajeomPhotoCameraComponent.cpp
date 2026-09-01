// Fill out your copyright notice in the Description page of Project Settings.

#include "BalhwajeomPhotoCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "BalhwajeomEvidenceActor.h"
#include "BalhwajeomEvidenceCameraHUD.h"

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
}

UBalhwajeomPhotoCameraComponent::UBalhwajeomPhotoCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
		PanCamera(PhotoCamera->GetRightVector(), Value);
	}
}

void UBalhwajeomPhotoCameraComponent::PanVertical(float Value)
{
	if (PhotoCamera)
	{
		PanCamera(PhotoCamera->GetUpVector(), Value);
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
	SavedFirstPersonRelativeTransform = PhotoCamera->GetRelativeTransform();
	SavedFirstPersonFieldOfView = PhotoCamera->FieldOfView;
	CameraModeEntryWorldLocation = PhotoCamera->GetComponentLocation();
	CameraPanWorldOffset = FVector::ZeroVector;
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
		PlayerController->SetViewTargetWithBlend(GetOwner(), 0.0f);

		if (const AActor* Owner = GetOwner())
		{
			PlayerController->SetControlRotation(FRotator(0.0f, Owner->GetActorRotation().Yaw, 0.0f));
		}
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
	}
	CameraPanWorldOffset = FVector::ZeroVector;

	bIsInCameraMode = false;
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
