#include "Story/StoryStateSubsystem.h"


bool UStoryStateSubsystem::AddStateTag(FGameplayTag StateTag)
{
	if (!StateTag.IsValid() || CurrentStateTags.HasTagExact(StateTag))
	{
		return false;
	}

	CurrentStateTags.AddTag(StateTag);
	OnStateTagAdded.Broadcast(StateTag);

	return true;
}


bool UStoryStateSubsystem::RemoveStateTag(FGameplayTag StateTag)
{
	if (!StateTag.IsValid() || !CurrentStateTags.HasTagExact(StateTag))
	{
		return false;
	}

	CurrentStateTags.RemoveTag(StateTag);
	OnStateTagRemoved.Broadcast(StateTag);

	return true;
}


bool UStoryStateSubsystem::HasStateTag(FGameplayTag StateTag) const
{
	return StateTag.IsValid() && CurrentStateTags.HasTag(StateTag);
}


bool UStoryStateSubsystem::HasStateTagExact(FGameplayTag StateTag) const
{
	return StateTag.IsValid() && CurrentStateTags.HasTagExact(StateTag);
}


bool UStoryStateSubsystem::HasAnyStateTags(
	const FGameplayTagContainer& StateTags
) const
{
	return CurrentStateTags.HasAny(StateTags);
}


bool UStoryStateSubsystem::HasAllStateTags(
	const FGameplayTagContainer& StateTags
) const
{
	return CurrentStateTags.HasAll(StateTags);
}


bool UStoryStateSubsystem::MatchesStateQuery(
	const FGameplayTagQuery& StateQuery
) const
{
	return StateQuery.Matches(CurrentStateTags);
}


FGameplayTagContainer UStoryStateSubsystem::GetCurrentStateTags() const
{
	return CurrentStateTags;
}


bool UStoryStateSubsystem::ClearStateTags()
{
	if (CurrentStateTags.IsEmpty())
	{
		return false;
	}

	const FGameplayTagContainer RemovedTags = CurrentStateTags;
	CurrentStateTags.Reset();

	for (const FGameplayTag& RemovedTag : RemovedTags)
	{
		OnStateTagRemoved.Broadcast(RemovedTag);
	}

	return true;
}
