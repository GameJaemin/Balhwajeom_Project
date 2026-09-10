// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BalhwajeomCameraPlayerController.generated.h"

class UUserWidget;
class UWidget;

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

	/** Creates the interaction prompt once for the local controller. */
	UFUNCTION(BlueprintCallable, Category = "UI|Interaction")
	void EnsureInteractionPrompt();

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetPlayerHUD() const { return PlayerHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|Interaction")
	UUserWidget* GetInteractionPrompt() const { return InteractionPromptWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	/** UMG HUD selected by the PlayerController Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	/** The single live HUD instance owned by this local PlayerController. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> PlayerHUDWidget;

	/** UMG prompt displayed while a usable evidence actor is focused at close range. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	TSubclassOf<UUserWidget> InteractionPromptWidgetClass;

	/** The single live interaction prompt owned by this local PlayerController. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Interaction")
	TObjectPtr<UUserWidget> InteractionPromptWidget;

	/** Widget inside WB_Interact that fades; the rest of the widget (including the center dot) stays visible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	FName InteractionPromptFadeTargetName = TEXT("TextBlock_50");

	UPROPERTY(Transient)
	TObjectPtr<UWidget> InteractionPromptFadeTarget;

	/** Larger values make the prompt reach its target opacity more quickly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction", meta = (ClampMin = "0.1"))
	float InteractionPromptFadeSpeed = 8.0f;

private:
	void HandleMouseYaw(float Value);
	bool ShouldShowInteractionPrompt() const;
	void UpdateInteractionPrompt(float DeltaSeconds);
};
