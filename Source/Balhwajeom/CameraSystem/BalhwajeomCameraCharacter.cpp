// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "BalhwajeomFixedCameraZone.h"
#include "BalhwajeomPhotoCameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Tablet/BalhwajeomTabletComponent.h"

ABalhwajeomCameraCharacter::ABalhwajeomCameraCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->BrakingDecelerationWalking = 2000.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 1200.0f;
	// Fixed diagonal quarter view. Absolute rotation keeps the camera from
	// spinning when the character turns toward its movement direction.
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, -45.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(15.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetAutoActivate(false);

	PhotoCameraComponent = CreateDefaultSubobject<UBalhwajeomPhotoCameraComponent>(TEXT("PhotoCameraComponent"));
	PhotoCameraComponent->SetNormalCamera(TopDownCamera);
	PhotoCameraComponent->SetPhotoCamera(FirstPersonCamera);

	TabletComponent = CreateDefaultSubobject<UBalhwajeomTabletComponent>(TEXT("TabletComponent"));
	PhotoCameraComponent->OnCameraModeExited.AddLambda([this]()
	{
		// Camera mode grabbed the view target away from the active zone; hand it back now that we're done.
		if (ActiveCameraZone)
		{
			ActiveCameraZone->ActivateCamera(this);
		}
	});

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (CharacterMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(CharacterMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimationBlueprint(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (AnimationBlueprint.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimationBlueprint.Class);
	}
}

void ABalhwajeomCameraCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Apply the Blueprint default so designers can tune WalkSpeed without
	// changing or recompiling this C++ class.
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (bAllowCameraOrbit)
	{
		// Let the boom read the controller's rotation instead of holding a fixed world-space angle.
		CameraBoom->SetUsingAbsoluteRotation(false);
		CameraBoom->bUsePawnControlRotation = true;
		CameraBoom->bInheritPitch = true;
		CameraBoom->bInheritYaw = true;
		CameraBoom->bInheritRoll = false;

		// Orbit mode expects a strafe-style controller: the character always faces the camera's
		// yaw, and WASD translates relative to that facing instead of spinning to face movement.
		// (bOrientRotationToMovement would otherwise snap the character to face the input
		// direction, which reads as "AD just turns the character" and makes it spin to face the
		// camera when walking backward.)
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

void ABalhwajeomCameraCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABalhwajeomCameraCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABalhwajeomCameraCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ABalhwajeomCameraCharacter::StartSprinting);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ABalhwajeomCameraCharacter::StopSprinting);

	if (UBalhwajeomPhotoCameraComponent* PhotoCamera = PhotoCameraComponent.Get())
	{
		PlayerInputComponent->BindAction(TEXT("CameraMode"), IE_Pressed, PhotoCamera, &UBalhwajeomPhotoCameraComponent::ToggleCameraMode);
		PlayerInputComponent->BindAction(TEXT("ExitCameraMode"), IE_Pressed, PhotoCamera, &UBalhwajeomPhotoCameraComponent::RequestExitCameraMode);
		PlayerInputComponent->BindAction(TEXT("TakePhoto"), IE_Pressed, PhotoCamera, &UBalhwajeomPhotoCameraComponent::TakePhoto);
		PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABalhwajeomCameraCharacter::HandleLookUp);
		PlayerInputComponent->BindAxis(TEXT("CameraZoom"), PhotoCamera, &UBalhwajeomPhotoCameraComponent::ZoomCamera);
	}
}

void ABalhwajeomCameraCharacter::MoveForward(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	if (PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode())
	{
		PhotoCameraComponent->PanVertical(Value);
		return;
	}

	FVector CameraForward = ActiveCameraZone
		? ActiveCameraZone->GetPlanarForwardVector()
		: CameraBoom->GetForwardVector();
	CameraForward.Z = 0.0f;
	AddMovementInput(CameraForward.GetSafeNormal(), Value);
}

void ABalhwajeomCameraCharacter::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	if (PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode())
	{
		PhotoCameraComponent->PanHorizontal(Value);
		return;
	}

	FVector CameraRight = ActiveCameraZone
		? ActiveCameraZone->GetPlanarRightVector()
		: CameraBoom->GetRightVector();
	CameraRight.Z = 0.0f;
	AddMovementInput(CameraRight.GetSafeNormal(), Value);
}

void ABalhwajeomCameraCharacter::StartSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ABalhwajeomCameraCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

bool ABalhwajeomCameraCharacter::IsInCameraMode() const
{
	return PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode();
}

TArray<FBalhwajeomEvidenceData> ABalhwajeomCameraCharacter::GetCollectedEvidence() const
{
	return PhotoCameraComponent ? PhotoCameraComponent->GetCollectedEvidence() : TArray<FBalhwajeomEvidenceData>();
}

void ABalhwajeomCameraCharacter::ApplyMouseYawInput(float Value)
{
	if (ActiveCameraZone)
	{
		ActiveCameraZone->AddYawInput(Value);
	}
	else if (PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode())
	{
		PhotoCameraComponent->LookYaw(Value);
	}
	else if (bAllowCameraOrbit)
	{
		AddControllerYawInput(Value);
	}
}

void ABalhwajeomCameraCharacter::HandleLookUp(float Value)
{
	if (PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode())
	{
		PhotoCameraComponent->LookPitch(Value);
	}
	else if (!ActiveCameraZone && bAllowCameraOrbit)
	{
		AddControllerPitchInput(Value);
	}
}

void ABalhwajeomCameraCharacter::RegisterCameraZone(ABalhwajeomFixedCameraZone* Zone)
{
	if (!IsValid(Zone))
	{
		return;
	}

	OverlappingCameraZones.AddUnique(Zone);
	RefreshActiveCameraZone();
}

void ABalhwajeomCameraCharacter::UnregisterCameraZone(ABalhwajeomFixedCameraZone* Zone)
{
	OverlappingCameraZones.Remove(Zone);
	RefreshActiveCameraZone();
}

void ABalhwajeomCameraCharacter::RefreshActiveCameraZone()
{
	OverlappingCameraZones.RemoveAll([](const TObjectPtr<ABalhwajeomFixedCameraZone>& Zone)
	{
		return !IsValid(Zone);
	});

	ABalhwajeomFixedCameraZone* BestZone = nullptr;
	for (ABalhwajeomFixedCameraZone* Zone : OverlappingCameraZones)
	{
		if (!BestZone || Zone->GetZonePriority() > BestZone->GetZonePriority())
		{
			BestZone = Zone;
		}
	}

	if (BestZone == ActiveCameraZone)
	{
		return;
	}

	ABalhwajeomFixedCameraZone* PreviousZone = ActiveCameraZone;
	ActiveCameraZone = BestZone;

	if (PhotoCameraComponent)
	{
		PhotoCameraComponent->RequestExitCameraMode();
	}

	if (PreviousZone && ActiveCameraZone)
	{
		PreviousZone->DeactivateCamera(this, false);
	}

	if (ActiveCameraZone)
	{
		ActiveCameraZone->ActivateCamera(this);
	}
	else if (PreviousZone)
	{
		PreviousZone->DeactivateCamera(this);
	}
}
