#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "BalhwajeomGateDoorActor.generated.h"


class UDoorInteractionComponent;
class UInspectionComponent;
class UStaticMeshComponent;
class UWidgetComponent;


/**
 * A placeable door whose opening is gated on story progress.
 *
 * The gate itself lives in UDoorInteractionComponent, which the player's interaction
 * code already consults before offering the [F] prompt -- so a locked door shows no
 * prompt at all. This actor adds the part that component cannot own: a world label that
 * tells the player *why* the door will not open.
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

	/** Label for the current lock/open state. Exposed so tests do not need a widget. */
	UFUNCTION(BlueprintPure, Category = "Gate Door")
	FText ResolveCurrentLabel() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState);

	void ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState);
	void SetInspectionLabel(const FText& LabelText, bool bVisible);
	void RefreshLabelForLockState();

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

	/** Local offset of the label from the door mesh's bounds centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Text")
	FVector ObjectLabelOffset = FVector::ZeroVector;

private:
	EPlayerInspectionDistanceState LastInspectionDistanceState =
		EPlayerInspectionDistanceState::OutOfRange;

	/** Last lock state pushed to the label, so the text is only rebuilt when it changes. */
	bool bLastKnownUnlocked = false;
	bool bLastKnownOpen = false;
	bool bHasLabelState = false;
};
