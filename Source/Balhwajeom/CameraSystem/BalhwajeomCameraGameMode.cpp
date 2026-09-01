// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraGameMode.h"

#include "BalhwajeomEvidenceCameraHUD.h"
#include "BalhwajeomCameraCharacter.h"
#include "BalhwajeomCameraPlayerController.h"

ABalhwajeomCameraGameMode::ABalhwajeomCameraGameMode()
{
	DefaultPawnClass = ABalhwajeomCameraCharacter::StaticClass();
	PlayerControllerClass = ABalhwajeomCameraPlayerController::StaticClass();
	HUDClass = ABalhwajeomEvidenceCameraHUD::StaticClass();
}
