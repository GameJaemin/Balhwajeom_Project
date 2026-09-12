#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DoorInteractionComponent.generated.h"


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

	UFUNCTION(BlueprintPure, Category = "Door")
	bool CanInteract() const;

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool RequestInteraction();

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
