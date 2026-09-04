#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BalhwajeomTabletComponent.generated.h"

class APlayerController;
class APawn;
class UBalhwajeomPhotoCameraComponent;
class UBalhwajeomTabletWidget;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;

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

	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void ToggleTablet();

	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void RequestOpenTablet();

	UFUNCTION(BlueprintCallable, Category = "Tablet")
	void CloseTablet();

	UFUNCTION(BlueprintPure, Category = "Tablet")
	bool IsTabletOpen() const { return bTabletOpen; }

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
};
