#include "Tutorial/BalhwajeomTutorialOverlayTriggers.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"
#include "Story/StoryStateSubsystem.h"

namespace
{
	UStoryStateSubsystem* FindStoryState(const UObject* WorldContextObject)
	{
		const UWorld* World = GEngine
			? GEngine->GetWorldFromContextObject(
				WorldContextObject, EGetWorldErrorMode::ReturnNull)
			: nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UStoryStateSubsystem>() : nullptr;
	}
}

namespace BalhwajeomTutorialOverlayTriggers
{
bool Set(const UObject* WorldContextObject, const FGameplayTag& Tag)
{
	UStoryStateSubsystem* StoryState = FindStoryState(WorldContextObject);
	return StoryState && StoryState->AddStateTag(Tag);
}


bool Clear(const UObject* WorldContextObject, const FGameplayTag& Tag)
{
	UStoryStateSubsystem* StoryState = FindStoryState(WorldContextObject);
	return StoryState && StoryState->RemoveStateTag(Tag);
}


bool Has(const UObject* WorldContextObject, const FGameplayTag& Tag)
{
	const UStoryStateSubsystem* StoryState = FindStoryState(WorldContextObject);
	return StoryState && StoryState->HasStateTagExact(Tag);
}
}
