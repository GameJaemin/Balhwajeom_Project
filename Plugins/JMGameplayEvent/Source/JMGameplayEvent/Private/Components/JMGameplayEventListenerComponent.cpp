#include "Components/JMGameplayEventListenerComponent.h"

#include "Engine/GameInstance.h"
#include "JMGameplayEvent.h"
#include "Subsystems/JMGameplayEventSubsystem.h"

UJMGameplayEventListenerComponent::UJMGameplayEventListenerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UJMGameplayEventListenerComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoSubscribe)
    {
        RefreshSubscriptions();
    }
}

void UJMGameplayEventListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnsubscribeAll();
    Super::EndPlay(EndPlayReason);
}

bool UJMGameplayEventListenerComponent::AddEventTag(FGameplayTag EventTag)
{
    if (!EventTag.IsValid() || EventTags.HasTagExact(EventTag))
    {
        return false;
    }
    EventTags.AddTag(EventTag);
    if (HasBegunPlay())
    {
        RefreshSubscriptions();
    }
    return true;
}

bool UJMGameplayEventListenerComponent::RemoveEventTag(FGameplayTag EventTag)
{
    if (!EventTags.HasTagExact(EventTag))
    {
        return false;
    }
    EventTags.RemoveTag(EventTag);
    if (HasBegunPlay())
    {
        RefreshSubscriptions();
    }
    return true;
}

void UJMGameplayEventListenerComponent::RefreshSubscriptions()
{
    UnsubscribeAll();
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UJMGameplayEventSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UJMGameplayEventSubsystem>() : nullptr;
    if (!Subsystem)
    {
        return;
    }
    for (const FGameplayTag& EventTag : EventTags)
    {
        FJMGameplayEventNativeDelegate Callback;
        Callback.BindUObject(this, &UJMGameplayEventListenerComponent::HandleGameplayEvent);
        ActiveHandles.Add(Subsystem->SubscribeEvent(EventTag, this, MatchType, MoveTemp(Callback)));
    }
}

void UJMGameplayEventListenerComponent::UnsubscribeAll()
{
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UJMGameplayEventSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UJMGameplayEventSubsystem>() : nullptr;
    if (Subsystem)
    {
        Subsystem->UnsubscribeAll(this);
    }
    ActiveHandles.Reset();
}

void UJMGameplayEventListenerComponent::HandleGameplayEvent(const FJMGameplayEventMessage& Message)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bEnableDebugLog)
    {
        UE_LOG(LogJMGameplayEvent, Log, TEXT("Listener component %s received %s"), *GetNameSafe(this), *Message.EventTag.ToString());
    }
#endif
    OnGameplayEventReceived.Broadcast(Message);
}
