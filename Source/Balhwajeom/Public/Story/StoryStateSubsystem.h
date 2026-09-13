#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StoryStateSubsystem.generated.h"


class UBalhwajeomInvestigationSubsystem;


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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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

	/**
	 * Records that an evidence state's world story has been presented, as
	 * "Evidence.StoryPlayed.<StateID>".
	 *
	 * A Repeatable world story can be replayed and never changes state, so this is the
	 * only signal that the player has actually heard it. Unregistered tags are ignored.
	 */
	UFUNCTION(BlueprintCallable, Category = "Story State")
	bool AddEvidenceStoryPlayedTag(FName StateID);

	/**
	 * Records that an object's world story has been heard at all, as
	 * "Evidence.StoryHeard.<ObjectID>".
	 *
	 * The per-state tag above cannot answer "has the player heard this object's story?"
	 * on its own, because the same story can play from more than one state -- before a
	 * photo from the cleaned state, and again afterwards from the memory state. Progress
	 * gates should use this one so they do not depend on which route the player took.
	 */
	UFUNCTION(BlueprintCallable, Category = "Story State")
	bool AddEvidenceStoryHeardTag(FName ObjectID);

	UPROPERTY(BlueprintAssignable, Category = "Story State|Events")
	FOnStoryStateTagChanged OnStateTagAdded;

	UPROPERTY(BlueprintAssignable, Category = "Story State|Events")
	FOnStoryStateTagChanged OnStateTagRemoved;

private:
	UFUNCTION()
	void HandlePhotoCaptured(const FCapturedPhotoRecord& PhotoRecord);

	UFUNCTION()
	void HandleSentenceSolved(FName SentenceID);

	/**
	 * Mirrors every evidence state transition into the story state as a tag, so
	 * progression gates can be authored as plain FGameplayTagQuery conditions.
	 */
	UFUNCTION()
	void HandleEvidenceStateChanged(
		FGuid ChangedInstanceID,
		FName PreviousStateID,
		FName NewStateID
	);

	void AddPhotographedEvidenceTag(FName ObjectID);
	void AddSentenceSolvedEvidenceTag(FName PhotoID);

	/** Adds "Evidence.State.<StateID>". Unregistered tags are silently ignored. */
	void AddEvidenceStateTag(FName StateID);

	UPROPERTY(Transient)
	FGameplayTagContainer CurrentStateTags;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomInvestigationSubsystem> InvestigationSubsystem;
};
