// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DogAnimInstance.generated.h"

/** Runtime movement data consumed by ABP_Dog's Idle/Walk blend graph. */
UCLASS(Blueprintable, BlueprintType)
class BALHWAJEOM_API UDogAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Dog|Movement")
	float GroundSpeed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Dog|Movement")
	bool bIsMoving = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dog|Movement", meta = (ClampMin = "0.0"))
	float MovingThreshold = 5.0f;
};
