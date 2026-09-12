#pragma once

#include "CoreMinimal.h"
#include "Types/JMGameplayEventTypes.h"
#include "JMGameplayEventTestReceiver.generated.h"

class UJMGameplayEventSubsystem;

UCLASS()
class UJMGameplayEventTestReceiver : public UObject
{
    GENERATED_BODY()

public:
    int32 ReceivedCount = 0;
    FGameplayTag LastTag;
    FJMGameplayEventSubscriptionHandle HandleToRemove;

    UPROPERTY()
    TObjectPtr<UJMGameplayEventSubsystem> Subsystem = nullptr;

    FGameplayTag NestedTag;

    void Receive(const FJMGameplayEventMessage& Message);
    void ReceiveAndUnsubscribe(const FJMGameplayEventMessage& Message);
    void ReceiveAndPublishNested(const FJMGameplayEventMessage& Message);
};
