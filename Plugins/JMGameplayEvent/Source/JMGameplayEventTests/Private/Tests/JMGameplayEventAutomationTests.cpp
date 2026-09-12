#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Subsystems/JMGameplayEventSubsystem.h"
#include "Tests/JMGameplayEventTestTags.h"
#include "Tests/JMGameplayEventTestReceiver.h"
#include "UObject/GarbageCollection.h"

namespace
{
    FGameplayTag DoorTag() { return JMGameplayEventTestTags::Door; }
    FGameplayTag DoorOpenedTag() { return JMGameplayEventTestTags::DoorOpened; }
    FGameplayTag DoorClosedTag() { return JMGameplayEventTestTags::DoorClosed; }
    FGameplayTag NestedTag() { return JMGameplayEventTestTags::Nested; }

    FJMGameplayEventSubscriptionHandle Subscribe(
        UJMGameplayEventSubsystem* Subsystem,
        FGameplayTag Tag,
        UJMGameplayEventTestReceiver* Receiver,
        EJMGameplayEventMatchType MatchType,
        void (UJMGameplayEventTestReceiver::*Method)(const FJMGameplayEventMessage&))
    {
        FJMGameplayEventNativeDelegate Callback;
        Callback.BindUObject(Receiver, Method);
        return Subsystem->SubscribeEvent(Tag, Receiver, MatchType, MoveTemp(Callback));
    }

    int32 Publish(UJMGameplayEventSubsystem* Subsystem, FGameplayTag Tag)
    {
        FJMGameplayEventMessage Message;
        Message.EventTag = Tag;
        return Subsystem->PublishEvent(Message);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventExactTest, "JM.GameplayEvent.ExactMatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventExactTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    UJMGameplayEventTestReceiver* Receiver = NewObject<UJMGameplayEventTestReceiver>();
    Subscribe(Subsystem, DoorOpenedTag(), Receiver, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    TestEqual(TEXT("Opened has one listener"), Publish(Subsystem, DoorOpenedTag()), 1);
    TestEqual(TEXT("Closed does not match"), Publish(Subsystem, DoorClosedTag()), 0);
    TestEqual(TEXT("Exactly one callback"), Receiver->ReceivedCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventChildrenTest, "JM.GameplayEvent.IncludeChildren", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventChildrenTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    UJMGameplayEventTestReceiver* Receiver = NewObject<UJMGameplayEventTestReceiver>();
    Subscribe(Subsystem, DoorTag(), Receiver, EJMGameplayEventMatchType::IncludeChildren, &UJMGameplayEventTestReceiver::Receive);
    Publish(Subsystem, DoorOpenedTag());
    Publish(Subsystem, DoorClosedTag());
    TestEqual(TEXT("Both child tags received"), Receiver->ReceivedCount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventUnsubscribeTest, "JM.GameplayEvent.Unsubscribe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventUnsubscribeTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    UJMGameplayEventTestReceiver* Receiver = NewObject<UJMGameplayEventTestReceiver>();
    const FJMGameplayEventSubscriptionHandle Handle = Subscribe(Subsystem, DoorOpenedTag(), Receiver, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    TestTrue(TEXT("Valid handle"), Handle.IsValid());
    TestTrue(TEXT("Unsubscribe succeeds"), Subsystem->UnsubscribeEvent(Handle));
    TestEqual(TEXT("No callback after unsubscribe"), Publish(Subsystem, DoorOpenedTag()), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventMultiListenerTest, "JM.GameplayEvent.MultipleListeners", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventMultiListenerTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    TArray<TStrongObjectPtr<UJMGameplayEventTestReceiver>> Receivers;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UJMGameplayEventTestReceiver* Receiver = NewObject<UJMGameplayEventTestReceiver>();
        Receivers.Emplace(Receiver);
        Subscribe(Subsystem, DoorOpenedTag(), Receiver, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    }
    TestEqual(TEXT("Three callbacks invoked"), Publish(Subsystem, DoorOpenedTag()), 3);
    for (const TStrongObjectPtr<UJMGameplayEventTestReceiver>& Receiver : Receivers)
    {
        TestEqual(TEXT("Each listener receives once"), Receiver->ReceivedCount, 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventMutationTest, "JM.GameplayEvent.SafeMutationDuringDispatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventMutationTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    UJMGameplayEventTestReceiver* First = NewObject<UJMGameplayEventTestReceiver>();
    UJMGameplayEventTestReceiver* Second = NewObject<UJMGameplayEventTestReceiver>();
    First->Subsystem = Subsystem;
    First->HandleToRemove = Subscribe(Subsystem, DoorOpenedTag(), First, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::ReceiveAndUnsubscribe);
    Subscribe(Subsystem, DoorOpenedTag(), Second, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    Publish(Subsystem, DoorOpenedTag());
    Publish(Subsystem, DoorOpenedTag());
    TestEqual(TEXT("Self-unsubscribed listener only runs once"), First->ReceivedCount, 1);
    TestEqual(TEXT("Other listener remains stable"), Second->ReceivedCount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventNestedTest, "JM.GameplayEvent.NestedDispatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventNestedTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    UJMGameplayEventTestReceiver* Publisher = NewObject<UJMGameplayEventTestReceiver>();
    UJMGameplayEventTestReceiver* NestedReceiver = NewObject<UJMGameplayEventTestReceiver>();
    Publisher->Subsystem = Subsystem;
    Publisher->NestedTag = NestedTag();
    Subscribe(Subsystem, DoorOpenedTag(), Publisher, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::ReceiveAndPublishNested);
    Subscribe(Subsystem, NestedTag(), NestedReceiver, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    Publish(Subsystem, DoorOpenedTag());
    TestEqual(TEXT("Nested event delivered normally"), NestedReceiver->ReceivedCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventDuplicateTest, "JM.GameplayEvent.DuplicateSubscriptions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventDuplicateTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    UJMGameplayEventSubsystem* Subsystem = NewObject<UJMGameplayEventSubsystem>(GameInstance.Get());
    UJMGameplayEventTestReceiver* Receiver = NewObject<UJMGameplayEventTestReceiver>();
    Subscribe(Subsystem, DoorOpenedTag(), Receiver, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    Subscribe(Subsystem, DoorOpenedTag(), Receiver, EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    TestEqual(TEXT("Documented duplicate policy invokes both subscriptions"), Publish(Subsystem, DoorOpenedTag()), 2);
    TestEqual(TEXT("Same listener receives once per handle"), Receiver->ReceivedCount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJMGameplayEventDestroyedListenerTest, "JM.GameplayEvent.DestroyedListenerCleanup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJMGameplayEventDestroyedListenerTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    TStrongObjectPtr<UJMGameplayEventSubsystem> Subsystem(NewObject<UJMGameplayEventSubsystem>(GameInstance.Get()));
    TStrongObjectPtr<UJMGameplayEventTestReceiver> Receiver(NewObject<UJMGameplayEventTestReceiver>());
    Subscribe(Subsystem.Get(), DoorOpenedTag(), Receiver.Get(), EJMGameplayEventMatchType::Exact, &UJMGameplayEventTestReceiver::Receive);
    Receiver.Reset();
    CollectGarbage(RF_NoFlags);
    TestEqual(TEXT("Destroyed listener is not called"), Publish(Subsystem.Get(), DoorOpenedTag()), 0);
    TestEqual(TEXT("Invalid subscription is removed"), Subsystem->GetSubscriptionCount(), 0);
    return true;
}
