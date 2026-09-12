#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryStateSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnStoryStateTagChanged,
	FGameplayTag,
	StateTag
);


UCLASS()
class BALHWAJEOM_API UStoryStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Story State")
	bool AddStateTag(FGameplayTag StateTag);

	UFUNCTION(BlueprintCallable, Category = "Story State")
	bool RemoveStateTag(FGameplayTag StateTag);

	/**
	 * Replaces every active tag below StateRootTag with StateTag.
	 * StateTag must be a child of StateRootTag, and the resulting group
	 * contains at most one exact tag before change events are broadcast.
	 */
	UFUNCTION(BlueprintCallable, Category = "Story State")
	bool SetExclusiveStateTag(
		FGameplayTag StateRootTag,
		FGameplayTag StateTag
	);

	/** Selects exactly one tag below Runtime.Player.Mode. */
	UFUNCTION(BlueprintCallable, Category = "Story State|Player Mode")
	bool SetPlayerModeTag(FGameplayTag PlayerModeTag);

	UFUNCTION(BlueprintPure, Category = "Story State")
	bool HasStateTag(FGameplayTag StateTag) const;

	UFUNCTION(BlueprintPure, Category = "Story State")
	bool HasStateTagExact(FGameplayTag StateTag) const;

	UFUNCTION(BlueprintPure, Category = "Story State")
	bool HasAnyStateTags(const FGameplayTagContainer& StateTags) const;

	UFUNCTION(BlueprintPure, Category = "Story State")
	bool HasAllStateTags(const FGameplayTagContainer& StateTags) const;

	UFUNCTION(BlueprintPure, Category = "Story State")
	bool MatchesStateQuery(const FGameplayTagQuery& StateQuery) const;

	UFUNCTION(BlueprintPure, Category = "Story State")
	FGameplayTagContainer GetCurrentStateTags() const;

	UFUNCTION(BlueprintCallable, Category = "Story State")
	bool ClearStateTags();

	UPROPERTY(BlueprintAssignable, Category = "Story State|Events")
	FOnStoryStateTagChanged OnStateTagAdded;

	UPROPERTY(BlueprintAssignable, Category = "Story State|Events")
	FOnStoryStateTagChanged OnStateTagRemoved;

private:
	UPROPERTY(Transient)
	FGameplayTagContainer CurrentStateTags;
};
