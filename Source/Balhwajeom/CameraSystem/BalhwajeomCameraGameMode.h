// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BalhwajeomCameraGameMode.generated.h"

UCLASS()
class BALHWAJEOM_API ABalhwajeomCameraGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABalhwajeomCameraGameMode();

protected:
	/** Prefers the PlayerStart tagged "Player1" over whichever one the default implementation would pick. */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
