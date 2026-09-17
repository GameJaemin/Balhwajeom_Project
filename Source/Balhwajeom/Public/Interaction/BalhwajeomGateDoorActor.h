#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/GateDoorLockedFeedback.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "BalhwajeomGateDoorActor.generated.h"


class UDoorInteractionComponent;
class UInspectionComponent;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGateDoorOpened);


/**
 * A placeable door whose opening is gated on story progress.
 *
 * The gate itself lives in UDoorInteractionComponent, which the player's interaction
 * code already consults before offering the [F] prompt. A locked door remains
 * interactable and this actor shows a short screen-space explanation when opening is
 * attempted before its story condition is met.
 *
 * Rotation happens around the actor's own pivot, so place the actor at the hinge and
 * offset DoorMesh to where the panel actually is.
 */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomGateDoorActor : public AActor
{
	GENERATED_BODY()

public:
	ABalhwajeomGateDoorActor();

	UFUNCTION(BlueprintPure, Category = "Gate Door")
	UDoorInteractionComponent* GetDoorInteraction() const { return DoorInteraction; }

	UFUNCTION(BlueprintPure, Category = "Gate Door")
	UInspectionComponent* GetInspectionComponent() const { return InspectionComponent; }

	UFUNCTION(BlueprintPure, Category = "Gate Door")
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }

	const TArray<FGateDoorLockedFeedbackStage>& GetLockedFeedbackStages() const
	{
		return LockedFeedbackStages;
	}

	/** Label for the current lock/open state. Exposed so tests do not need a widget. */
	UFUNCTION(BlueprintPure, Category = "Gate Door")
	FText ResolveCurrentLabel() const;

	/** Fired exactly once, the moment this door finishes its open animation
	 * (DoorInteraction::IsOpen() first becomes true). Lets unrelated systems (e.g. BGM) react to
	 * the door opening without the door needing to know about them. */
	UPROPERTY(BlueprintAssignable, Category = "Gate Door")
	FOnGateDoorOpened OnDoorFullyOpened;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState);

	UFUNCTION()
	void HandleLockedInteractionRequested();

	UFUNCTION()
	void AdvanceLockedFeedbackFade();

	void BeginLockedFeedbackFade();
	void HideLockedFeedback();

	TSubclassOf<UUserWidget> ResolveLockedFeedbackWidgetClass() const;
	FText ResolveLockedFeedbackMessage() const;
	void ApplyLockedFeedbackMessage(const FText& Message);
	void ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState);
	void SetInspectionLabel(const FText& LabelText, bool bVisible);
	void RefreshLabelForLockState();
	void CheckDoorFullyOpenedBroadcast();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate Door|Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate Door|Components")
	TObjectPtr<UDoorInteractionComponent> DoorInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate Door|Components")
	TObjectPtr<UInspectionComponent> InspectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate Door|Components")
	TObjectPtr<UWidgetComponent> ObjectLabelWidget;

	/** Shown while the door's unlock condition is not met. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Text")
	FText LockedLabel;

	/** Shown once the door can be opened. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Text")
	FText UnlockedLabel;

	/** Shown after the door has been opened. Empty hides the label. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Text")
	FText OpenedLabel;

	/**
	 * Put across the top of the screen the moment this door's story condition is met, e.g.
	 * "문이 열리는 소리가 난 것 같다..". The player is usually somewhere else when a door
	 * unlocks, so without a nudge the change goes unnoticed.
	 *
	 * Empty means no notification, which is what every door placed before this existed gets.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Text")
	FText UnlockedNotificationText;

	/** Local offset of the label from the door mesh's bounds centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Text")
	FVector ObjectLabelOffset = FVector::ZeroVector;

	/** Full-screen feedback shown when this locked door is interacted with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback")
	TSubclassOf<UUserWidget> LockedFeedbackWidgetClass;

	/** Ordered guidance. The first entry whose required tags are incomplete is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback")
	TArray<FGateDoorLockedFeedbackStage> LockedFeedbackStages;

	/** How long the message stays fully opaque, i.e. excluding the two fades around it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback",
		meta = (ClampMin = "0.1", UIMin = "0.1", Units = "s"))
	float LockedFeedbackDisplayDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float LockedFeedbackFadeInDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float LockedFeedbackFadeOutDuration = 0.4f;

	/** Above the tutorial dim and interaction prompt, below modal screens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback")
	int32 LockedFeedbackZOrder = 1100;

private:
	EPlayerInspectionDistanceState LastInspectionDistanceState =
		EPlayerInspectionDistanceState::OutOfRange;

	/** Last lock state pushed to the label, so the text is only rebuilt when it changes. */
	bool bLastKnownUnlocked = false;
	bool bLastKnownOpen = false;
	bool bHasLabelState = false;

	/** Guards OnDoorFullyOpened so it only ever fires once per door. */
	bool bHasBroadcastDoorOpened = false;

	/** Guards the unlock notification the same way, in case a condition flickers. */
	bool bHasAnnouncedUnlock = false;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LockedFeedbackWidget;

	/** Seconds into the current fade-in/hold/fade-out cycle. */
	float LockedFeedbackElapsedSeconds = 0.0f;

	FTimerHandle LockedFeedbackFadeTimerHandle;
};
