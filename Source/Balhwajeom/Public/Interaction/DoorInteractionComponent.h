#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DoorInteractionComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorLockedInteractionRequested);


/** Opens the owning actor around its existing pivot when the player interacts. */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UDoorInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorInteractionComponent();

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door",
		meta = (Units = "deg")
	)
	float OpenYawAngle = -90.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Door",
		meta = (ClampMin = "0.0", Units = "s")
	)
	float OpenDuration = 1.5f;

	/**
	 * Story state condition that must hold before this door can be opened.
	 * An empty query leaves the door permanently unlocked, so existing doors are unaffected.
	 *
	 * A locked door still accepts an interaction attempt. RequestInteraction then broadcasts
	 * OnLockedInteractionRequested instead of opening it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Gate")
	FGameplayTagQuery UnlockQuery;

	/**
	 * Simple form of the same gate: every tag here must be present in the story state.
	 * Both conditions are combined with AND, and leaving both empty means "always unlocked".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Gate")
	FGameplayTagContainer UnlockRequiresTags;

	UFUNCTION(BlueprintPure, Category = "Door|Gate")
	bool IsUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bIsOpen; }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpening() const { return bIsOpening; }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool CanInteract() const;

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool RequestInteraction();

	/** Fired when the player presses interact while the unlock condition is not met. */
	UPROPERTY(BlueprintAssignable, Category = "Door|Gate")
	FOnDoorLockedInteractionRequested OnLockedInteractionRequested;

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

private:
	FRotator ClosedRotation = FRotator::ZeroRotator;
	FRotator OpenRotation = FRotator::ZeroRotator;
	float ElapsedOpenTime = 0.0f;
	bool bIsOpening = false;
	bool bIsOpen = false;
};
