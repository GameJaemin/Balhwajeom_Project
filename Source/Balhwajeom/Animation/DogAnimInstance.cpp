// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/DogAnimInstance.h"

#include "GameFramework/Actor.h"

void UDogAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AActor* Owner = GetOwningActor();
	GroundSpeed = Owner ? Owner->GetVelocity().Size2D() : 0.0f;
	bIsMoving = GroundSpeed >= MovingThreshold;
}
