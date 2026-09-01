// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BalhwajeomCameraPlayerController.generated.h"

/** Owns mouse-look input and forwards it to the possessed Project Self character. */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomCameraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABalhwajeomCameraPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void HandleMouseYaw(float Value);
};
