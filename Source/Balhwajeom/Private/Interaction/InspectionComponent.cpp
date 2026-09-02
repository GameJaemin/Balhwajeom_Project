#include "Interaction/InspectionComponent.h"


UInspectionComponent::UInspectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UInspectionComponent::NotifyPlayerDistanceStateChanged(
	EPlayerInspectionDistanceState NewState
)
{
	OnPlayerDistanceStateChanged.Broadcast(NewState);
}