#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "PlayerInteractionComponent.generated.h"

class UInspectionComponent;


UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UPlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInteractionComponent();

	/**
	 * 숫자로 주어진 거리를 기준으로
	 * Player 관점의 거리 상태를 판정합니다.
	 */
	static EPlayerInspectionDistanceState ClassifyDistance(
		float Distance,
		float CloseDistance,
		float MiddleDistance,
		float MaxDisplayDistance
	);

	/**
	 * Player 위치와 대상의 Bounds 중심 위치 사이의 실제 거리를 계산한 뒤
	 * Player 관점의 거리 상태를 판정합니다.
	 */
	static EPlayerInspectionDistanceState ClassifyDistanceBetweenPoints(
		const FVector& PlayerLocation,
		const FVector& TargetBoundsCenter,
		float CloseDistance,
		float MiddleDistance,
		float MaxDisplayDistance
	);

	static bool CanInspectDistanceState(
		EPlayerInspectionDistanceState DistanceState
	);

	bool UpdateDistanceStateForInspectable(
		UInspectionComponent* InspectionComponent,
		const FVector& PlayerLocation,
		const FVector& TargetBoundsCenter
	);

	UFUNCTION(BlueprintPure, Category = "Player Interaction")
	EPlayerInspectionDistanceState GetDistanceStateForInspectable(
		UInspectionComponent* InspectionComponent
	) const;

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Player Interaction")
	void RefreshInspectableObjects();

	UFUNCTION(BlueprintPure, Category = "Player Interaction")
	int32 GetInspectableObjectCount() const;

private:
	UPROPERTY()
	TArray<TObjectPtr<UInspectionComponent>> InspectableObjects;

	UPROPERTY()
	TMap<TObjectPtr<UInspectionComponent>, EPlayerInspectionDistanceState> DistanceStates;
};