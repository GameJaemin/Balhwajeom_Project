#include "Blueprint/JMGameplayEventBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/JMGameplayEventSubsystem.h"

int32 UJMGameplayEventBlueprintLibrary::PublishGameplayEvent(
    const UObject* WorldContextObject,
    FGameplayTag EventTag,
    UObject* Source,
    AActor* Instigator,
    UObject* Target,
    FGameplayTagContainer ContextTags,
    UObject* Payload)
{
    UJMGameplayEventSubsystem* Subsystem = GetGameplayEventSubsystem(WorldContextObject);
    if (!Subsystem)
    {
        return 0;
    }
    FJMGameplayEventMessage Message;
    Message.EventTag = EventTag;
    Message.Source = Source;
    Message.Instigator = Instigator;
    Message.Target = Target;
    Message.ContextTags = MoveTemp(ContextTags);
    Message.Payload = Payload;
    return Subsystem->PublishEvent(Message);
}

UJMGameplayEventSubsystem* UJMGameplayEventBlueprintLibrary::GetGameplayEventSubsystem(const UObject* WorldContextObject)
{
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UJMGameplayEventSubsystem>() : nullptr;
}
