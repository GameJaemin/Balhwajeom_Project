// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BalhwajeomCameraPlayerController.generated.h"

class UUserWidget;

/** Owns mouse-look input and forwards it to the possessed Project Self character. */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomCameraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABalhwajeomCameraPlayerController();

	/** Creates the configured player HUD once for the local controller. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void EnsurePlayerHUD();

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetPlayerHUD() const { return PlayerHUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** UMG HUD selected by the PlayerController Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	/** The single live HUD instance owned by this local PlayerController. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> PlayerHUDWidget;

private:
	void HandleMouseYaw(float Value);
};
