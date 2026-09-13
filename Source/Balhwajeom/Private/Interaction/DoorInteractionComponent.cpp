#include "Interaction/DoorInteractionComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"


UDoorInteractionComponent::UDoorInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


void UDoorInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* DoorActor = GetOwner();
	if (!IsValid(DoorActor))
	{
		return;
	}

	if (USceneComponent* RootComponent = DoorActor->GetRootComponent())
	{
		RootComponent->SetMobility(EComponentMobility::Movable);
	}

	ClosedRotation = DoorActor->GetActorRotation();
	OpenRotation = ClosedRotation;
	OpenRotation.Yaw += OpenYawAngle;
}


bool UDoorInteractionComponent::CanInteract() const
{
	return IsValid(GetOwner()) && !bIsOpening && !bIsOpen;
}


bool UDoorInteractionComponent::RequestInteraction()
{
	if (!CanInteract())
	{
		return false;
	}

	AActor* DoorActor = GetOwner();
	if (OpenDuration <= KINDA_SMALL_NUMBER)
	{
		DoorActor->SetActorRotation(OpenRotation);
		bIsOpen = true;
		return true;
	}

	ElapsedOpenTime = 0.0f;
	bIsOpening = true;
	SetComponentTickEnabled(true);
	return true;
}


void UDoorInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* DoorActor = GetOwner();
	if (!bIsOpening || !IsValid(DoorActor))
	{
		SetComponentTickEnabled(false);
		return;
	}

	ElapsedOpenTime += DeltaTime;
	const float Alpha = FMath::Clamp(ElapsedOpenTime / OpenDuration, 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	const FQuat NewRotation = FQuat::Slerp(
		ClosedRotation.Quaternion(),
		OpenRotation.Quaternion(),
		SmoothAlpha
	);

	DoorActor->SetActorRotation(NewRotation);

	if (Alpha >= 1.0f)
	{
		DoorActor->SetActorRotation(OpenRotation);
		bIsOpening = false;
		bIsOpen = true;
		SetComponentTickEnabled(false);
	}
}
