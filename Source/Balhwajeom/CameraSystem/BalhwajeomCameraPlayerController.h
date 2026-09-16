// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "BalhwajeomCameraPlayerController.generated.h"

class UUserWidget;
class UWidget;
class UTexture2D;
class UBalhwajeomInvestigationSubsystem;
class UBalhwajeomKeywordCounterWidget;
class UStoryStateSubsystem;

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

	/** Creates the bed-memory HUD once for the local controller. */
	UFUNCTION(BlueprintCallable, Category = "UI|Bed Memory")
	void EnsureBedMemoryHUD();

	/** Cross-fades between the exploration HUD and the bed-memory HUD. */
	UFUNCTION(BlueprintCallable, Category = "UI|Bed Memory")
	void SetBedMemoryHUDActive(bool bActive);

	/** Creates the interaction prompt once for the local controller. */
	UFUNCTION(BlueprintCallable, Category = "UI|Interaction")
	void EnsureInteractionPrompt();

	/**
	 * Creates the tutorial dim/highlight layer once for the local controller.
	 * It sits at ZOrder 5: above the player HUD icons and the world, below WB_Interact,
	 * so dimming the screen leaves the [F] prompt and the centre dot fully readable.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Tutorial")
	void EnsureTutorialFocusLayer();

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetPlayerHUD() const { return PlayerHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|Bed Memory")
	UUserWidget* GetBedMemoryHUD() const { return BedMemoryHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|Bed Memory")
	bool IsBedMemoryHUDActive() const { return bBedMemoryHUDActive; }

	UFUNCTION(BlueprintPure, Category = "UI|Interaction")
	UUserWidget* GetInteractionPrompt() const { return InteractionPromptWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|Tutorial")
	UUserWidget* GetTutorialFocusLayer() const { return TutorialFocusWidget; }

	/**
	 * Current 0..1 fade alpha of the [F] prompt text.
	 *
	 * The prompt is only faded in while the player is actually looking at a close,
	 * interactable target, so a tutorial dim multiplied by this value appears and
	 * disappears in exact sync with the prompt -- no second fade, no flicker.
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Interaction")
	float GetInteractionPromptAlpha() const;

	/** Shows the authored evidence-inspection text above the tutorial dim layer. */
	UFUNCTION(BlueprintCallable, Category = "UI|Inspection")
	void ShowInspectionMessage(const FText& Message);

	/** Hides or restores every gameplay HUD layer owned by this controller. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetGameplayPresentationEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsGameplayPresentationEnabled() const { return bGameplayPresentationEnabled; }

	/** Applies a photo-capture keyword count after its card has reached the TAB HUD. */
	void FlushPendingPhotoKeywordCount();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	/** UMG HUD selected by the PlayerController Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	/** The single live HUD instance owned by this local PlayerController. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> PlayerHUDWidget;

	/** Live acquired/total keyword count shown above the bottom-right TAB hint. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Keywords")
	TObjectPtr<UBalhwajeomKeywordCounterWidget> KeywordCounterWidget;

	/** Alternate HUD shown while the player is resting on a bed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Bed Memory")
	TSubclassOf<UUserWidget> BedMemoryHUDWidgetClass;

	/** The single live bed-memory HUD instance owned by this controller. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Bed Memory")
	TObjectPtr<UUserWidget> BedMemoryHUDWidget;

	/** Seconds for a complete WB_HUD/WB_HUD2 cross-fade. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Bed Memory",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float BedMemoryHUDFadeDuration = 0.45f;

	/** UMG prompt displayed while a usable evidence actor is focused at close range. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	TSubclassOf<UUserWidget> InteractionPromptWidgetClass;

	/** The single live interaction prompt owned by this local PlayerController. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Interaction")
	TObjectPtr<UUserWidget> InteractionPromptWidget;

	/** Dim/highlight layer used by the tutorial flow. Leave unset to disable the layer entirely. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Tutorial")
	TSubclassOf<UUserWidget> TutorialFocusWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Tutorial")
	TObjectPtr<UUserWidget> TutorialFocusWidget;

	/** Widget inside WB_Interact that fades; the rest of the widget (including the center dot) stays visible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	FName InteractionPromptFadeTargetName = TEXT("TextBlock_50");

	UPROPERTY(Transient)
	TObjectPtr<UWidget> InteractionPromptFadeTarget;

	/** True while WB_Interact is drawn below the tutorial dim for the camera hint. */
	bool bInteractionPromptBehindTutorialDim = false;

	/** Larger values make the prompt reach its target opacity more quickly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction", meta = (ClampMin = "0.1"))
	float InteractionPromptFadeSpeed = 8.0f;

	/** Authored result widget displayed after interacting with an Evidence Actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inspection")
	TSubclassOf<UUserWidget> InspectionMessageWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|Inspection")
	TObjectPtr<UUserWidget> InspectionMessageWidget;

	/** TextBlock inside WBP_InspectionMessage that receives the evidence text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inspection")
	FName InspectionMessageTextWidgetName = TEXT("MessageText");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inspection",
		meta = (ClampMin = "0.1", UIMin = "0.1", Units = "s"))
	float InspectionMessageDisplayDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inspection")
	int32 InspectionMessageViewportZOrder = 1200;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FBedMemoryTestAccessor;
#endif

	void HandleMouseYaw(float Value);
	void EnsureKeywordCounter();
	void RefreshKeywordCounter();
	void BindHudModeEvents();
	void RefreshHudModeIcons();
	void HideInspectionMessage();

	UFUNCTION()
	void HandleHudModeTagChanged(FGameplayTag StateTag);

	UFUNCTION()
	void HandleWordAcquired(const FAcquiredWordRecord& WordRecord);

	/** Bound to OnPhotoGalleryReset so the keyword counter drops the stale pre-reset total. */
	UFUNCTION()
	void HandleInvestigationPhotoGalleryReset();
	bool ShouldShowInteractionPrompt() const;
	bool IsInteractionPromptSuppressedByTablet() const;
	bool IsInteractionPromptSuppressedByPhotoCamera() const;
	void UpdateInteractionPrompt(float DeltaSeconds);
	void UpdateBedMemoryHUD(float DeltaSeconds);
	void ApplyBedMemoryHUDAlpha(float Alpha);

	/** Fade value of the [F] prompt before any tutorial blink is applied. */
	float InteractionPromptAlpha = 0.0f;
	float BedMemoryHUDAlpha = 0.0f;
	bool bBedMemoryHUDActive = false;
	bool bGameplayPresentationEnabled = true;
	bool bPhotoKeywordCountRefreshPending = false;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomInvestigationSubsystem> BoundInvestigationSubsystem;

	/** Story state source used to keep the two WBP_HUID mode buttons in sync. */
	UPROPERTY(Transient)
	TObjectPtr<UStoryStateSubsystem> BoundHudStoryStateSubsystem;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mode Buttons")
	TObjectPtr<UTexture2D> CameraButtonIdleTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mode Buttons")
	TObjectPtr<UTexture2D> CameraButtonClickedTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mode Buttons")
	TObjectPtr<UTexture2D> TabletButtonIdleTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mode Buttons")
	TObjectPtr<UTexture2D> TabletButtonClickedTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mode Buttons")
	FName CameraButtonImageName = TEXT("Image_Camera");

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mode Buttons")
	FName TabletButtonImageName = TEXT("Image_TAB");

	FTimerHandle InspectionMessageTimerHandle;
};
