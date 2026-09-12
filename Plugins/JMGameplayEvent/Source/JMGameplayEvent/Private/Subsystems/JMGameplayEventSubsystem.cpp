#include "Subsystems/JMGameplayEventSubsystem.h"

#include "JMGameplayEvent.h"
#include "Settings/JMGameplayEventSettings.h"

namespace
{
    struct FJMDispatchScope
    {
        explicit FJMDispatchScope(int32& InDepth) : Depth(InDepth) { ++Depth; }
        ~FJMDispatchScope() { --Depth; }
        int32& Depth;
    };
}

void UJMGameplayEventSubsystem::Deinitialize()
{
    bDeinitializing = true;
    SubscriptionsByTag.Reset();
    Super::Deinitialize();
}

int32 UJMGameplayEventSubsystem::PublishEvent(const FJMGameplayEventMessage& Message)
{
    if (!ensureMsgf(IsInGameThread(), TEXT("JMGameplayEvent must be published on the game thread.")) || bDeinitializing)
    {
        return 0;
    }
    if (!Message.EventTag.IsValid())
    {
        UE_LOG(LogJMGameplayEvent, Warning, TEXT("Publish rejected: EventTag is invalid."));
        return 0;
    }

    const UJMGameplayEventSettings* Settings = GetDefault<UJMGameplayEventSettings>();
    const int32 MaxDepth = Settings ? FMath::Max(1, Settings->MaximumNestedDispatchDepth) : 16;
    if (DispatchDepth >= MaxDepth)
    {
        UE_LOG(LogJMGameplayEvent, Error, TEXT("Publish blocked at nested dispatch depth %d: %s"), DispatchDepth, *Message.EventTag.ToString());
        return 0;
    }

    RemoveInvalidSubscriptions();

    struct FDispatchEntry
    {
        FGameplayTag BucketTag;
        FGuid Id;
    };
    TArray<FDispatchEntry, TInlineAllocator<16>> Snapshot;
    for (const TPair<FGameplayTag, FSubscriptionArray>& Pair : SubscriptionsByTag)
    {
        const bool bExactBucket = Pair.Key == Message.EventTag;
        const bool bChildBucket = Message.EventTag.MatchesTag(Pair.Key);
        for (const FSubscription& Subscription : Pair.Value)
        {
            if (bExactBucket || (Subscription.MatchType == EJMGameplayEventMatchType::IncludeChildren && bChildBucket))
            {
                Snapshot.Add({Pair.Key, Subscription.Id});
            }
        }
    }

    if (ShouldLog() && Settings->bLogPublishedEvents)
    {
        UE_LOG(LogJMGameplayEvent, Log, TEXT("[JMGameplayEvent] Publish: %s Source: %s Instigator: %s Listeners: %d"),
            *Message.EventTag.ToString(), *GetNameSafe(Message.Source), *GetNameSafe(Message.Instigator), Snapshot.Num());
    }
    if (Snapshot.IsEmpty() && ShouldLog() && Settings->bLogUnhandledEvents)
    {
        UE_LOG(LogJMGameplayEvent, Log, TEXT("[JMGameplayEvent] %s was published with no listeners."), *Message.EventTag.ToString());
    }

    FJMDispatchScope Scope(DispatchDepth);
    int32 InvokedCount = 0;
    for (const FDispatchEntry& Entry : Snapshot)
    {
        FSubscription* Subscription = FindSubscription(Entry.BucketTag, Entry.Id);
        if (!Subscription || !Subscription->Listener.IsValid() || !Subscription->Callback.IsBound())
        {
            continue;
        }
        // A callback may remove its own subscription. Execute a local delegate copy
        // so removing the array element cannot destroy the delegate currently on stack.
        FJMGameplayEventNativeDelegate Callback = Subscription->Callback;
        Callback.Execute(Message);
        ++InvokedCount;
        if (bDeinitializing)
        {
            break;
        }
    }
    RemoveInvalidSubscriptions();
    return InvokedCount;
}

FJMGameplayEventSubscriptionHandle UJMGameplayEventSubsystem::SubscribeEvent(
    FGameplayTag EventTag,
    UObject* Listener,
    EJMGameplayEventMatchType MatchType,
    FJMGameplayEventNativeDelegate Callback)
{
    FJMGameplayEventSubscriptionHandle Handle;
    if (!ensureMsgf(IsInGameThread(), TEXT("JMGameplayEvent subscriptions must be changed on the game thread.")) ||
        bDeinitializing || !EventTag.IsValid() || !IsValid(Listener) || !Callback.IsBound())
    {
        return Handle;
    }

    const UJMGameplayEventSettings* Settings = GetDefault<UJMGameplayEventSettings>();
    if (Settings && Settings->bWarnDuplicateSubscription && IsDuplicate(EventTag, Listener, MatchType))
    {
        UE_LOG(LogJMGameplayEvent, Warning, TEXT("Duplicate subscription allowed: Listener=%s Tag=%s Match=%s"),
            *GetNameSafe(Listener), *EventTag.ToString(), MatchType == EJMGameplayEventMatchType::Exact ? TEXT("Exact") : TEXT("IncludeChildren"));
    }

    Handle.Id = FGuid::NewGuid();
    FSubscription& NewSubscription = SubscriptionsByTag.FindOrAdd(EventTag).AddDefaulted_GetRef();
    NewSubscription.Id = Handle.Id;
    NewSubscription.EventTag = EventTag;
    NewSubscription.MatchType = MatchType;
    NewSubscription.Listener = Listener;
    NewSubscription.Callback = MoveTemp(Callback);

    if (ShouldLog())
    {
        UE_LOG(LogJMGameplayEvent, Log, TEXT("[JMGameplayEvent] Subscribe Listener: %s Tag: %s Match: %s"),
            *GetNameSafe(Listener), *EventTag.ToString(), MatchType == EJMGameplayEventMatchType::Exact ? TEXT("Exact") : TEXT("IncludeChildren"));
    }
    return Handle;
}

bool UJMGameplayEventSubsystem::UnsubscribeEvent(FJMGameplayEventSubscriptionHandle Handle)
{
    if (!ensureMsgf(IsInGameThread(), TEXT("JMGameplayEvent subscriptions must be changed on the game thread.")) || !Handle.IsValid())
    {
        return false;
    }
    for (auto It = SubscriptionsByTag.CreateIterator(); It; ++It)
    {
        FSubscriptionArray& Bucket = It.Value();
        const int32 Removed = Bucket.RemoveAll([&Handle](const FSubscription& Entry) { return Entry.Id == Handle.Id; });
        if (Bucket.IsEmpty())
        {
            It.RemoveCurrent();
        }
        if (Removed > 0)
        {
            return true;
        }
    }
    return false;
}

int32 UJMGameplayEventSubsystem::UnsubscribeAll(UObject* Listener)
{
    if (!ensureMsgf(IsInGameThread(), TEXT("JMGameplayEvent subscriptions must be changed on the game thread.")) || !Listener)
    {
        return 0;
    }
    int32 RemovedCount = 0;
    for (auto It = SubscriptionsByTag.CreateIterator(); It; ++It)
    {
        FSubscriptionArray& Bucket = It.Value();
        RemovedCount += Bucket.RemoveAll([Listener](const FSubscription& Entry) { return Entry.Listener.Get() == Listener; });
        if (Bucket.IsEmpty())
        {
            It.RemoveCurrent();
        }
    }
    return RemovedCount;
}

int32 UJMGameplayEventSubsystem::GetSubscriptionCount() const
{
    int32 Count = 0;
    for (const TPair<FGameplayTag, FSubscriptionArray>& Pair : SubscriptionsByTag)
    {
        Count += Pair.Value.Num();
    }
    return Count;
}

UJMGameplayEventSubsystem::FSubscription* UJMGameplayEventSubsystem::FindSubscription(const FGameplayTag& BucketTag, const FGuid& Id)
{
    FSubscriptionArray* Bucket = SubscriptionsByTag.Find(BucketTag);
    return Bucket ? Bucket->FindByPredicate([&Id](const FSubscription& Entry) { return Entry.Id == Id; }) : nullptr;
}

void UJMGameplayEventSubsystem::RemoveInvalidSubscriptions()
{
    for (auto It = SubscriptionsByTag.CreateIterator(); It; ++It)
    {
        It.Value().RemoveAll([](const FSubscription& Entry)
        {
            return !Entry.Listener.IsValid() || !Entry.Callback.IsBound();
        });
        if (It.Value().IsEmpty())
        {
            It.RemoveCurrent();
        }
    }
}

bool UJMGameplayEventSubsystem::IsDuplicate(FGameplayTag EventTag, UObject* Listener, EJMGameplayEventMatchType MatchType) const
{
    const FSubscriptionArray* Bucket = SubscriptionsByTag.Find(EventTag);
    return Bucket && Bucket->ContainsByPredicate([Listener, MatchType](const FSubscription& Entry)
    {
        return Entry.Listener.Get() == Listener && Entry.MatchType == MatchType;
    });
}

bool UJMGameplayEventSubsystem::ShouldLog() const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    const UJMGameplayEventSettings* Settings = GetDefault<UJMGameplayEventSettings>();
    return Settings && Settings->bEnableDebugLogging;
#endif
}
