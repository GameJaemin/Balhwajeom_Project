// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "BalhwajeomCameraPlayerController.generated.h"

class UUserWidget;
class UWidget;
class UImage;
class UTextBlock;
class UTexture2D;
class UBalhwajeomInvestigationSubsystem;
class UBalhwajeomInteractionModalWidget;
class UBalhwajeomKeywordCounterWidget;
class UStoryStateSubsystem;

DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomInteractionModalClosed);

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

	/** Adds the tutorial overlay presenter on first run, for controllers that have none. */
	void EnsureTutorialOverlayPresenter();

	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetPlayerHUD() const { return PlayerHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|Bed Memory")
	UUserWidget* GetBedMemoryHUD() const { return BedMemoryHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI|Bed Memory")
	bool IsBedMemoryHUDActive() const { return bBedMemoryHUDActive; }

	UFUNCTION(BlueprintPure, Category = "UI|Interaction")
	UUserWidget* GetInteractionPrompt() const { return InteractionPromptWidget; }

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

	/** Opens a blocking interaction widget and decorates it with newly acquired keywords. */
	bool ShowInteractionModal(
		TSubclassOf<UUserWidget> ContentWidgetClass,
		const FText& DocumentText,
		const TArray<FText>& NewlyGrantedKeywords);

	UFUNCTION(BlueprintPure, Category = "UI|Interaction")
	bool IsInteractionModalOpen() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Interaction")
	void CloseInteractionModal();

	/** Native completion point for presentations that must start after the modal is gone. */
	FOnBalhwajeomInteractionModalClosed OnInteractionModalClosed;

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

	/** Widget inside WB_Interact that fades; the rest of the widget (including the center dot) stays visible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	FName InteractionPromptFadeTargetName = TEXT("TextBlock_50");

	UPROPERTY(Transient)
	TObjectPtr<UWidget> InteractionPromptFadeTarget;

	/** TextBlock that receives the state-specific action text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	FName InteractionPromptTextWidgetName = TEXT("TextBlock_50");

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InteractionPromptTextWidget;

	/** Center-screen image that switches between the idle dot and interactable magnifier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	FName InteractionReticleWidgetName = TEXT("InteractionReticle");

	UPROPERTY(Transient)
	TObjectPtr<UImage> InteractionReticleWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Interaction")
	TObjectPtr<UTexture2D> InteractionReticleDotTexture;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Interaction")
	TObjectPtr<UTexture2D> InteractionReticleMagnifierTexture;

	/** Applied when a target supplies an action; {Action} is replaced at runtime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Interaction")
	FText InteractionPromptFormat = NSLOCTEXT(
		"BalhwajeomInteraction", "PromptFormat", "[ F ] {Action}");

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
	friend struct FInteractionReticleTestAccessor;
	friend struct FInteractionModalTestAccessor;
#endif

	void HandleMouseYaw(float Value);
	void EnsureKeywordCounter();
	void RefreshKeywordCounter();
	void BindHudModeEvents();
	void RefreshHudModeIcons();
	void HideInspectionMessage();
	void HandleInteractionModalCloseRequested();

	UFUNCTION()
	void HandleHudModeTagChanged(FGameplayTag StateTag);

	UFUNCTION()
	void HandleWordAcquired(const FAcquiredWordRecord& WordRecord);

	/** Bound to OnPhotoGalleryReset so the keyword counter drops the stale pre-reset total. */
	UFUNCTION()
	void HandleInvestigationPhotoGalleryReset();
	bool ShouldShowInteractionPrompt() const;
	FText ResolveInteractionPromptActionText() const;
	void RefreshInteractionPromptText(bool bHasValidInteractionTarget);
	void RefreshInteractionReticle(bool bHasValidInteractionTarget);
	bool IsInteractionPromptSuppressedByTablet() const;
	bool IsInteractionPromptSuppressedByPhotoCamera() const;
	void UpdateInteractionPrompt(float DeltaSeconds);
	void UpdateBedMemoryHUD(float DeltaSeconds);
	void ApplyBedMemoryHUDAlpha(float Alpha);

	float InteractionPromptAlpha = 0.0f;
	bool bInteractionReticleStateInitialized = false;
	bool bInteractionReticleShowsInteractable = false;
	FText DefaultInteractionPromptText;
	float BedMemoryHUDAlpha = 0.0f;
	bool bBedMemoryHUDActive = false;
	bool bGameplayPresentationEnabled = true;
	bool bPhotoKeywordCountRefreshPending = false;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomInvestigationSubsystem> BoundInvestigationSubsystem;

	/** Story state source used to keep the two WBP_HUID mode buttons in sync. */
	UPROPERTY(Transient)
	TObjectPtr<UStoryStateSubsystem> BoundHudStoryStateSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomInteractionModalWidget> InteractionModalWidget;

	bool bInteractionModalChangedMoveIgnore = false;
	bool bInteractionModalChangedLookIgnore = false;
	bool bInteractionModalPreviousMouseCursor = false;

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
