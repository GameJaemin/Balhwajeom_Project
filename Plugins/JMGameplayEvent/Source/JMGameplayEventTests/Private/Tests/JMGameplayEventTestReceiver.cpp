#include "Tests/JMGameplayEventTestReceiver.h"

#include "Subsystems/JMGameplayEventSubsystem.h"

void UJMGameplayEventTestReceiver::Receive(const FJMGameplayEventMessage& Message)
{
    ++ReceivedCount;
    LastTag = Message.EventTag;
}

void UJMGameplayEventTestReceiver::ReceiveAndUnsubscribe(const FJMGameplayEventMessage& Message)
{
    Receive(Message);
    if (Subsystem)
    {
        Subsystem->UnsubscribeEvent(HandleToRemove);
    }
}

void UJMGameplayEventTestReceiver::ReceiveAndPublishNested(const FJMGameplayEventMessage& Message)
{
    Receive(Message);
    if (Subsystem && NestedTag.IsValid())
    {
        FJMGameplayEventMessage NestedMessage;
        NestedMessage.EventTag = NestedTag;
        Subsystem->PublishEvent(NestedMessage);
    }
}
