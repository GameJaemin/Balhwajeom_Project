#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "InspectionComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnPlayerDistanceStateChanged,
	EPlayerInspectionDistanceState,
	NewState
);


UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UInspectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInspectionComponent();

	// 이 거리 이내에서는 NearLabel을 표시하고 조사할 수 있습니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Inspection|Distance",
		meta = (ClampMin = "0.0")
	)
	float CloseDistance = 300.0f;

	// CloseDistance보다 멀고 이 거리 이내라면 MidLabel을 표시합니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Inspection|Distance",
		meta = (ClampMin = "0.0")
	)
	float MiddleDistance = 700.0f;

	// MiddleDistance보다 멀고 이 거리 이내라면 FarLabel을 표시합니다.
	// 이 거리보다 멀면 라벨을 숨깁니다.
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Inspection|Distance",
		meta = (ClampMin = "0.0")
	)
	float MaxDisplayDistance = 1500.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|Text")
	FText FarLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|Text")
	FText MidLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|Text")
	FText NearLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inspection|Text")
	FText InspectionText;


	/**
	 * Player 기준 거리 단계가 변경되었을 때 발생합니다.
	 *
	 * 실제 거리 계산은 PlayerInteractionComponent가 담당하고,
	 * 이 Component는 결과를 Blueprint에 전달하는 역할만 합니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Inspection|Events")
	FOnPlayerDistanceStateChanged OnPlayerDistanceStateChanged;


	/**
	 * C++ PlayerInteraction 시스템이 거리 상태 변경을 전달할 때 사용합니다.
	 */
	void NotifyPlayerDistanceStateChanged(
		EPlayerInspectionDistanceState NewState
	);
};