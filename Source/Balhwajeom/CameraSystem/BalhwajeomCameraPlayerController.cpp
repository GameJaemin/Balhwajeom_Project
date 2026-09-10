// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraPlayerController.h"

#include "BalhwajeomCameraCharacter.h"
#include "Blueprint/UserWidget.h"

ABalhwajeomCameraPlayerController::ABalhwajeomCameraPlayerController()
{
	bShowMouseCursor = false;
}

void ABalhwajeomCameraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);

	EnsurePlayerHUD();
}

void ABalhwajeomCameraPlayerController::EnsurePlayerHUD()
{
	if (!IsLocalController() || IsValid(PlayerHUDWidget) || !PlayerHUDWidgetClass)
	{
		return;
	}

	PlayerHUDWidget = CreateWidget<UUserWidget>(this, PlayerHUDWidgetClass);
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->AddToViewport(0);
	}
}

void ABalhwajeomCameraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	check(InputComponent);
	InputComponent->BindAxis(TEXT("Turn"), this, &ABalhwajeomCameraPlayerController::HandleMouseYaw);
}

void ABalhwajeomCameraPlayerController::HandleMouseYaw(float Value)
{
	if (ABalhwajeomCameraCharacter* ControlledCharacter = Cast<ABalhwajeomCameraCharacter>(GetPawn()))
	{
		ControlledCharacter->ApplyMouseYawInput(Value);
	}
}
