#include "Interaction/InspectionComponent.h"

#include "Engine/GameInstance.h"
#include "Story/StoryStateSubsystem.h"


UInspectionComponent::UInspectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


FInspectionData UInspectionComponent::GetDefaultInspectionData() const
{
	FInspectionData DefaultData;
	DefaultData.FarLabel = FarLabel;
	DefaultData.MidLabel = MidLabel;
	DefaultData.NearLabel = NearLabel;
	DefaultData.InspectionText = InspectionText;
	return DefaultData;
}


FInspectionData UInspectionComponent::ResolveInspectionDataForState(
	const FGameplayTagContainer& StateTags
) const
{
	for (const FConditionalInspectionData& Entry : ConditionalData)
	{
		if (Entry.Condition.IsEmpty())
		{
			continue;
		}

		if (Entry.Condition.Matches(StateTags))
		{
			return Entry.Data;
		}
	}

	return GetDefaultInspectionData();
}


FInspectionData UInspectionComponent::GetCurrentInspectionData() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return GetDefaultInspectionData();
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return GetDefaultInspectionData();
	}

	const UStoryStateSubsystem* StoryState =
		GameInstance->GetSubsystem<UStoryStateSubsystem>();

	if (!IsValid(StoryState))
	{
		return GetDefaultInspectionData();
	}

	return ResolveInspectionDataForState(
		StoryState->GetCurrentStateTags()
	);
}


void UInspectionComponent::NotifyPlayerDistanceStateChanged(
	EPlayerInspectionDistanceState NewState
)
{
	OnPlayerDistanceStateChanged.Broadcast(NewState);
}
