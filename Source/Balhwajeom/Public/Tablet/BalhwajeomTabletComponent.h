#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BalhwajeomTabletComponent.generated.h"

class APlayerController;
class APawn;
class UBalhwajeomPhotoCameraComponent;
class UBalhwajeomTabletWidget;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class USoundBase;

DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomTabletClosed);
DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomTabletStatementSolved);

/**
 * Reusable local-player tablet controller.
 *
 * Attach it to a Pawn or PlayerController. It registers the always-on tablet
 * mapping, owns a high-priority input component, and blocks lower gameplay
 * input components while the tablet is open.
 */
UCLASS(Blueprintable, ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomTabletComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBalhwajeomTabletComponent();

	/** Fired after the close animation has finished and tablet input has been restored. */
	FOnBalhwajeomTabletClosed OnTabletClosed;

	/** Relays UBalhwajeomTabletWidget::OnStatementSolved -- fired the moment any Statement-type
	 * sentence is submitted and validated correct, whether or not every character's statement is
	 * solved yet. The listener is responsible for checking that itself. */
	FOnBalhwajeomTabletStatementSolved OnStatementSolved;

	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void ToggleTablet();

	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void RequestOpenTablet();

	/** Opens the tablet and immediately navigates to the first statement in the default person folder. */
	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void RequestOpenTabletToStatement();

	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void CloseTablet();

	UFUNCTION(BlueprintPure, Category = "Tablet")
	bool IsTabletOpen() const { return bTabletOpen; }

	/** Used by title/cinematic flows to prevent the tablet action from opening. If the tablet is
	 * currently open, disabling this forcibly closes it -- see SetTabletToggleLocked for a version
	 * that blocks only the open/close key without touching an already-open tablet. */
	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void SetTabletInteractionEnabled(bool bEnabled);

	/** Blocks the tablet open/close key (ToggleTablet) without forcibly closing an already-open
	 * tablet, unlike SetTabletInteractionEnabled(false). Used to freeze player input during a
	 * scripted moment -- e.g. the ending's post-statement pause -- while whatever the tablet is
	 * already showing (the success animation) keeps playing undisturbed. */
	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void SetTabletToggleLocked(bool bLocked);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Physical keys are mapped only in this context. Tablet logic binds only the action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet|Input")
	TSoftObjectPtr<UInputMappingContext> TabletMappingContextAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet|Input")
	TSoftObjectPtr<UInputAction> TabletActionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet|Input")
	int32 TabletMappingPriority = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet|Input")
	int32 TabletInputComponentPriority = 1000;

	/** Defaults to WBP_Tablet, but can be replaced per player/component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet|UI")
	TSoftClassPtr<UBalhwajeomTabletWidget> TabletWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Audio")
	TSoftObjectPtr<USoundBase> TabletOpenSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Audio")
	TSoftObjectPtr<USoundBase> TabletCloseSound;

private:
	bool TryInitializeForLocalPlayer();
	void TeardownPlayerInput();
	APlayerController* ResolvePlayerController() const;
	APawn* ResolvePlayerPawn() const;
	UBalhwajeomPhotoCameraComponent* ResolvePhotoCameraComponent() const;
	void RefreshPhotoCameraBinding();
	void ProcessPendingPhotoExit();
	void OpenTabletNow();
	void FinishCloseTablet();
	void SetGameplayInputBlocked(bool bBlocked);

	void HandleTabletAction();
	void HandlePhotoCameraModeExited();
	void HandlePhotoCameraTransitionFinished();
	void HandleTabletCloseAnimationFinished();
	void HandleStatementSolved();

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> TabletInputComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> LoadedTabletMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LoadedTabletAction;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomTabletWidget> TabletWidget;

	TWeakObjectPtr<APlayerController> InitializedPlayerController;
	TWeakObjectPtr<UBalhwajeomPhotoCameraComponent> BoundPhotoCamera;

	bool bTabletOpen = false;
	bool bTabletClosing = false;
	bool bPendingOpenAfterPhotoMode = false;
	bool bOwnsMappingContextRegistration = false;
	bool bSavedShowMouseCursor = false;
	bool bSavedEnableClickEvents = false;
	bool bSavedEnableMouseOverEvents = false;
	bool bTabletInteractionEnabled = true;
	bool bOpenStatementWhenReady = false;
};
