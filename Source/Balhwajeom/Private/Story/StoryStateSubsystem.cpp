#include "Story/StoryStateSubsystem.h"

#include "Engine/GameInstance.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Story/StoryStateTags.h"


void UStoryStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UBalhwajeomInvestigationSubsystem>();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		InvestigationSubsystem =
			GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>();
	}

	if (!InvestigationSubsystem)
	{
		return;
	}

	InvestigationSubsystem->OnPhotoCaptured.AddUniqueDynamic(
		this,
		&ThisClass::HandlePhotoCaptured
	);

	TArray<FCapturedPhotoRecord> CapturedPhotos;
	InvestigationSubsystem->GetCapturedPhotos(CapturedPhotos);
	for (const FCapturedPhotoRecord& PhotoRecord : CapturedPhotos)
	{
		AddPhotographedEvidenceTag(PhotoRecord.ObjectID);
	}
}


void UStoryStateSubsystem::Deinitialize()
{
	if (InvestigationSubsystem)
	{
		InvestigationSubsystem->OnPhotoCaptured.RemoveDynamic(
			this,
			&ThisClass::HandlePhotoCaptured
		);
		InvestigationSubsystem = nullptr;
	}

	Super::Deinitialize();
}


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


bool UStoryStateSubsystem::SetExclusiveStateTag(
	FGameplayTag StateRootTag,
	FGameplayTag StateTag
)
{
	if (!StateRootTag.IsValid() ||
		!StateTag.IsValid() ||
		StateTag == StateRootTag ||
		!StateTag.MatchesTag(StateRootTag))
	{
		return false;
	}

	TArray<FGameplayTag> ActiveTags;
	CurrentStateTags.GetGameplayTagArray(ActiveTags);

	TArray<FGameplayTag> RemovedTags;
	for (const FGameplayTag& ActiveTag : ActiveTags)
	{
		if (ActiveTag != StateTag && ActiveTag.MatchesTag(StateRootTag))
		{
			RemovedTags.Add(ActiveTag);
		}
	}

	const bool bNeedsAdd = !CurrentStateTags.HasTagExact(StateTag);
	if (RemovedTags.IsEmpty() && !bNeedsAdd)
	{
		return false;
	}

	for (const FGameplayTag& RemovedTag : RemovedTags)
	{
		CurrentStateTags.RemoveTag(RemovedTag);
	}
	if (bNeedsAdd)
	{
		CurrentStateTags.AddTag(StateTag);
	}

	// Broadcast only after the complete replacement so listeners never observe
	// an empty or multiply-active exclusive group during the transition.
	for (const FGameplayTag& RemovedTag : RemovedTags)
	{
		OnStateTagRemoved.Broadcast(RemovedTag);
	}
	if (bNeedsAdd)
	{
		OnStateTagAdded.Broadcast(StateTag);
	}

	return true;
}


bool UStoryStateSubsystem::SetPlayerModeTag(FGameplayTag PlayerModeTag)
{
	return SetExclusiveStateTag(
		BalhwajeomGameplayTags::Runtime_Player_Mode,
		PlayerModeTag
	);
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


void UStoryStateSubsystem::HandlePhotoCaptured(
	const FCapturedPhotoRecord& PhotoRecord
)
{
	AddPhotographedEvidenceTag(PhotoRecord.ObjectID);
}


void UStoryStateSubsystem::AddPhotographedEvidenceTag(FName ObjectID)
{
	if (ObjectID.IsNone())
	{
		return;
	}

	const FGameplayTag PhotographedTag = FGameplayTag::RequestGameplayTag(
		FName(*FString::Printf(
			TEXT("Evidence.Photographed.%s"),
			*ObjectID.ToString()
		)),
		false
	);
	if (PhotographedTag.IsValid())
	{
		AddStateTag(PhotographedTag);
	}
}
