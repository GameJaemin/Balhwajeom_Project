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
#include "Interaction/PlayerInteractionComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Animation/AnimationAsset.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"

ABalhwajeomCameraCharacter::ABalhwajeomCameraCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	Movement->JumpZVelocity = 500.0f;
	Movement->AirControl = 0.35f;
	Movement->MaxWalkSpeed = 500.0f;
	Movement->MinAnalogWalkSpeed = 20.0f;
	Movement->BrakingDecelerationWalking = 2000.0f;
	Movement->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = ThirdPersonArmLength;
	CameraBoom->TargetOffset = ThirdPersonTargetOffset;
	CameraBoom->SocketOffset = ThirdPersonSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;

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
	PlayerInteractionComponent = CreateDefaultSubobject<UPlayerInteractionComponent>(TEXT("PlayerInteractionComponent"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> InteractionMapping(
		TEXT("/Game/Balhwajeom/Input/IMC_Interaction.IMC_Interaction"));
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractionAction(
		TEXT("/Game/Balhwajeom/Input/IA_Interact.IA_Interact"));
	if (InteractionMapping.Succeeded()) PlayerInteractionComponent->InteractionMappingContext = InteractionMapping.Object;
	if (InteractionAction.Succeeded()) PlayerInteractionComponent->InteractAction = InteractionAction.Object;
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

void ABalhwajeomCameraCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// This path is opt-in so legacy children that use an Animation Blueprint are untouched.
	if (!IdleAnimation || !WalkAnimation || !GetMesh())
	{
		return;
	}

	const float HorizontalSpeed = GetVelocity().Size2D();
	UAnimationAsset* DesiredAnimation = HorizontalSpeed >= WalkAnimationThreshold
		? WalkAnimation.Get()
		: IdleAnimation.Get();

	if (DesiredAnimation != ActiveLocomotionAnimation)
	{
		GetMesh()->PlayAnimation(DesiredAnimation, true);
		ActiveLocomotionAnimation = DesiredAnimation;
	}
}

void ABalhwajeomCameraCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UStoryStateSubsystem* StoryState =
			GameInstance->GetSubsystem<UStoryStateSubsystem>())
		{
			StoryState->SetPlayerModeTag(
				BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration
			);
		}
	}

	// Blueprint child assets can retain the old quarter-view component values even after the
	// native constructor changes. Normalize the exploration camera at runtime so every child
	// starts with the same camera layout as BP_ThirdPersonCharacter.
	CameraBoom->SetUsingAbsoluteRotation(false);
	CameraBoom->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	CameraBoom->TargetArmLength = ThirdPersonArmLength;
	CameraBoom->TargetOffset = ThirdPersonTargetOffset;
	CameraBoom->SocketOffset = ThirdPersonSocketOffset;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;

	TopDownCamera->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	TopDownCamera->SetFieldOfView(ThirdPersonFieldOfView);
	TopDownCamera->bUsePawnControlRotation = false;

	if (Controller)
	{
		FRotator InitialControlRotation = Controller->GetControlRotation();
		InitialControlRotation.Pitch = ThirdPersonInitialPitch;
		InitialControlRotation.Roll = 0.0f;
		Controller->SetControlRotation(InitialControlRotation);
	}

	// The legacy Blueprint still contains an authored camera component. If more than one camera
	// is active, AActor can select that stale component instead of the native follow camera.
	TInlineComponentArray<UCameraComponent*> CameraComponents(this);
	for (UCameraComponent* CameraComponent : CameraComponents)
	{
		CameraComponent->SetActive(CameraComponent == TopDownCamera);
	}

	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetOnlyOwnerSee(false);
	GetMesh()->SetVisibility(true, true);

	if (PlayerInteractionComponent)
	{
		PlayerInteractionComponent->OnInspectionSucceeded.AddUniqueDynamic(
			this, &ABalhwajeomCameraCharacter::HandleInspectionSucceeded);
	}
}

void ABalhwajeomCameraCharacter::HandleInspectionSucceeded(FText InspectionText)
{
	if (GEngine && !InspectionText.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 4.0f, FColor(255, 220, 140), InspectionText.ToString(), true, FVector2D(1.25f));
	}
}

void ABalhwajeomCameraCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABalhwajeomCameraCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABalhwajeomCameraCharacter::MoveRight);

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
	if (FMath::IsNearlyZero(Value) || (Controller && Controller->IsMoveInputIgnored()))
	{
		return;
	}

	if (PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode())
	{
		PhotoCameraComponent->PanVertical(Value);
		return;
	}

	if (ActiveCameraZone)
	{
		AddMovementInput(ActiveCameraZone->GetPlanarForwardVector(), Value);
		return;
	}

	if (Controller)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, Value);
	}
}

void ABalhwajeomCameraCharacter::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value) || (Controller && Controller->IsMoveInputIgnored()))
	{
		return;
	}

	if (PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode())
	{
		PhotoCameraComponent->PanHorizontal(Value);
		return;
	}

	if (ActiveCameraZone)
	{
		AddMovementInput(ActiveCameraZone->GetPlanarRightVector(), Value);
		return;
	}

	if (Controller)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, Value);
	}
}

bool ABalhwajeomCameraCharacter::IsInCameraMode() const
{
	return PhotoCameraComponent && PhotoCameraComponent->IsInCameraMode();
}

void ABalhwajeomCameraCharacter::RestoreExplorationView(float BlendTime)
{
	if (ActiveCameraZone)
	{
		ActiveCameraZone->ActivateCamera(this);
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetViewTargetWithBlend(this, BlendTime, VTBlend_Cubic);
	}
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
	else
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
	else if (!ActiveCameraZone)
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
