// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraGameMode.h"

#include "BalhwajeomEvidenceCameraHUD.h"
#include "BalhwajeomCameraCharacter.h"
#include "BalhwajeomCameraPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ABalhwajeomCameraGameMode::ABalhwajeomCameraGameMode()
{
	DefaultPawnClass = ABalhwajeomCameraCharacter::StaticClass();
	PlayerControllerClass = ABalhwajeomCameraPlayerController::StaticClass();
	HUDClass = ABalhwajeomEvidenceCameraHUD::StaticClass();
}

AActor* ABalhwajeomCameraGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	for (AActor* Start : PlayerStarts)
	{
		const APlayerStart* PS = Cast<APlayerStart>(Start);
		if (PS && PS->PlayerStartTag == TEXT("Player1"))
		{
			return Start;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}
