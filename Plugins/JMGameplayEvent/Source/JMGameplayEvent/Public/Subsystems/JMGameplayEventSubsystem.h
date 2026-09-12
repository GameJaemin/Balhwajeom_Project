#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Types/JMGameplayEventTypes.h"
#include "JMGameplayEventSubsystem.generated.h"

/**
 * Synchronous, game-thread-only, process-local gameplay event bus.
 * It intentionally performs no RPC or replication.
 */
UCLASS()
class JMGAMEPLAYEVENT_API UJMGameplayEventSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /** Publishes synchronously and returns the number of listeners invoked. */
    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    int32 PublishEvent(const FJMGameplayEventMessage& Message);

    FJMGameplayEventSubscriptionHandle SubscribeEvent(
        FGameplayTag EventTag,
        UObject* Listener,
        EJMGameplayEventMatchType MatchType,
        FJMGameplayEventNativeDelegate Callback);

    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    bool UnsubscribeEvent(FJMGameplayEventSubscriptionHandle Handle);

    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    int32 UnsubscribeAll(UObject* Listener);

    int32 GetSubscriptionCount() const;

private:
    struct FSubscription
    {
        FGuid Id;
        FGameplayTag EventTag;
        EJMGameplayEventMatchType MatchType = EJMGameplayEventMatchType::Exact;
        TWeakObjectPtr<UObject> Listener;
        FJMGameplayEventNativeDelegate Callback;
    };

    using FSubscriptionArray = TArray<FSubscription>;
    TMap<FGameplayTag, FSubscriptionArray> SubscriptionsByTag;
    int32 DispatchDepth = 0;
    bool bDeinitializing = false;

    FSubscription* FindSubscription(const FGameplayTag& BucketTag, const FGuid& Id);
    void RemoveInvalidSubscriptions();
    bool IsDuplicate(FGameplayTag EventTag, UObject* Listener, EJMGameplayEventMatchType MatchType) const;
    bool ShouldLog() const;
};
