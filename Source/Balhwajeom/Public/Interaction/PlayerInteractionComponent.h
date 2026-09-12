#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "PlayerInteractionComponent.generated.h"

class UInspectionComponent;
class UInputAction;
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnInspectionSucceeded,
	FText,
	InspectionText
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInspectionDismissRequested);

UCLASS(
	Blueprintable,
	ClassGroup = (Interaction),
	meta = (BlueprintSpawnableComponent)
)
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Player Interaction")
	bool RequestInspect();

	UPROPERTY(BlueprintAssignable, Category = "Player Interaction|Events")
	FOnInspectionSucceeded OnInspectionSucceeded;

	/** Requests that Blueprint presentation close any visible inspection text. */
	UPROPERTY(BlueprintAssignable, Category = "Player Interaction|Events")
	FOnInspectionDismissRequested OnInspectionDismissRequested;

	/** Blueprint presentation hook for closing the inspection message. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Interaction|Events")
	void ReceiveInspectionDismissRequested();

	UFUNCTION(BlueprintCallable, Category = "Player Interaction")
	void RefreshInspectableObjects();

	UFUNCTION(BlueprintPure, Category = "Player Interaction")
	int32 GetInspectableObjectCount() const;

	void SetFocusedInspection(
		UInspectionComponent* NewFocusedInspection
	);

	UFUNCTION(BlueprintCallable, Category = "Player Interaction")
	bool TryInspect(FText& OutInspectionText);

	UFUNCTION(BlueprintPure, Category = "Player Interaction")
	UInspectionComponent* GetFocusedInspection() const;

	UInspectionComponent* ResolveFocusedInspection(
		UInspectionComponent* HitInspection
	) const;

	// Interaction input settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Interaction|Input")
	TObjectPtr<UInputMappingContext> InteractionMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Interaction|Input")
	TObjectPtr<UInputAction> InteractAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Interaction|Input")
	int32 InteractionMappingPriority = 1;


	// Enhanced Input에서 Started가 발생했을 때 사용할 실제 처리 함수.
	// 자동 테스트에서도 이 함수를 직접 검증합니다.
	bool HandleInteractStarted();

private:
	UFUNCTION()
	void HandleStoryStateTagAdded(FGameplayTag StateTag);

	UPROPERTY()
	TArray<TObjectPtr<UInspectionComponent>> InspectableObjects;

	UPROPERTY()
	TMap<TObjectPtr<UInspectionComponent>, EPlayerInspectionDistanceState> DistanceStates;

	UPROPERTY()
	TObjectPtr<UInspectionComponent> FocusedInspection = nullptr;

	void SetupInteractionInput();
	void OnInteractActionStarted();
	bool IsInteractionSuppressedByPhotoCamera() const;

	bool bInteractionInputInitialized = false;
};
