// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomFixedCameraZone.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "BalhwajeomCameraCharacter.h"

ABalhwajeomFixedCameraZone::ABalhwajeomFixedCameraZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ZoneVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneVolume"));
	ZoneVolume->SetupAttachment(SceneRoot);
	ZoneVolume->SetBoxExtent(FVector(500.0f, 500.0f, 250.0f));
	ZoneVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneVolume->SetGenerateOverlapEvents(true);

	OrbitPivot = CreateDefaultSubobject<USceneComponent>(TEXT("OrbitPivot"));
	OrbitPivot->SetupAttachment(SceneRoot);

	FixedCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FixedCamera"));
	FixedCamera->SetupAttachment(OrbitPivot);
	FixedCamera->SetRelativeLocation(FVector(-700.0f, 0.0f, 500.0f));
	FixedCamera->bUsePawnControlRotation = false;

	ZoneVolume->OnComponentBeginOverlap.AddDynamic(this, &ABalhwajeomFixedCameraZone::HandleZoneBeginOverlap);
	ZoneVolume->OnComponentEndOverlap.AddDynamic(this, &ABalhwajeomFixedCameraZone::HandleZoneEndOverlap);
}

void ABalhwajeomFixedCameraZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MinYawOffset = FMath::Min(MinYawOffset, 0.0f);
	MaxYawOffset = FMath::Max(MaxYawOffset, 0.0f);

	if (bKeepCameraFacingCenter && !FixedCamera->GetRelativeLocation().IsNearlyZero())
	{
		FixedCamera->SetRelativeRotation((-FixedCamera->GetRelativeLocation()).Rotation());
	}
}

void ABalhwajeomFixedCameraZone::BeginPlay()
{
	Super::BeginPlay();
	PlacedOrbitRotation = OrbitPivot->GetRelativeRotation();
	CurrentYawOffset = 0.0f;
	ApplyCameraYaw();
}

void ABalhwajeomFixedCameraZone::HandleZoneBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (ABalhwajeomCameraCharacter* Character = Cast<ABalhwajeomCameraCharacter>(OtherActor))
	{
		Character->RegisterCameraZone(this);
	}
}

void ABalhwajeomFixedCameraZone::HandleZoneEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (ABalhwajeomCameraCharacter* Character = Cast<ABalhwajeomCameraCharacter>(OtherActor))
	{
		Character->UnregisterCameraZone(this);
	}
}

void ABalhwajeomFixedCameraZone::AddYawInput(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	CurrentYawOffset += Value * MouseYawSensitivity;
	if (bAllowFullYawRotation)
	{
		// Keep the stored value numerically stable while preserving seamless rotation.
		CurrentYawOffset = FMath::UnwindDegrees(CurrentYawOffset);
	}
	else
	{
		CurrentYawOffset = FMath::Clamp(CurrentYawOffset, MinYawOffset, MaxYawOffset);
	}
	ApplyCameraYaw();
}

void ABalhwajeomFixedCameraZone::ActivateCamera(ABalhwajeomCameraCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	if (bResetYawOnEnter)
	{
		CurrentYawOffset = 0.0f;
		ApplyCameraYaw();
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		PlayerController->SetViewTargetWithBlend(this, BlendTime, VTBlend_Cubic);
	}

	OnCameraActivated(Character);
}

void ABalhwajeomFixedCameraZone::DeactivateCamera(ABalhwajeomCameraCharacter* Character, bool bRestoreCharacterView)
{
	if (!Character)
	{
		return;
	}

	if (bRestoreCharacterView)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			PlayerController->SetViewTargetWithBlend(Character, BlendTime, VTBlend_Cubic);
		}
	}

	OnCameraDeactivated(Character);
}

FVector ABalhwajeomFixedCameraZone::GetPlanarForwardVector() const
{
	FVector Forward = FixedCamera->GetForwardVector();
	Forward.Z = 0.0f;
	return Forward.GetSafeNormal();
}

FVector ABalhwajeomFixedCameraZone::GetPlanarRightVector() const
{
	FVector Right = FixedCamera->GetRightVector();
	Right.Z = 0.0f;
	return Right.GetSafeNormal();
}

void ABalhwajeomFixedCameraZone::ApplyCameraYaw()
{
	FRotator OrbitRotation = PlacedOrbitRotation;
	OrbitRotation.Yaw += CurrentYawOffset;
	OrbitPivot->SetRelativeRotation(OrbitRotation);

	if (bKeepCameraFacingCenter && !FixedCamera->GetRelativeLocation().IsNearlyZero())
	{
		FixedCamera->SetRelativeRotation((-FixedCamera->GetRelativeLocation()).Rotation());
	}
}
