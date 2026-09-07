#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "StoryStateSubsystemTestTypes.generated.h"


UCLASS(Transient)
class UStoryStateSubsystemTestObserver final : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleStateTagAdded(FGameplayTag StateTag);

	UFUNCTION()
	void HandleStateTagRemoved(FGameplayTag StateTag);

	int32 AddedCount = 0;
	int32 RemovedCount = 0;
	FGameplayTag LastAddedTag;
	FGameplayTag LastRemovedTag;
};
