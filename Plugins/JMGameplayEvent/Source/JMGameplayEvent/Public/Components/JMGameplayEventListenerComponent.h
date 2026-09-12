#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/JMGameplayEventTypes.h"
#include "JMGameplayEventListenerComponent.generated.h"

UCLASS(ClassGroup = (JMGameplay), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class JMGAMEPLAYEVENT_API UJMGameplayEventListenerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UJMGameplayEventListenerComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JM Gameplay Event")
    FGameplayTagContainer EventTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JM Gameplay Event")
    EJMGameplayEventMatchType MatchType = EJMGameplayEventMatchType::Exact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JM Gameplay Event")
    bool bAutoSubscribe = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JM Gameplay Event")
    bool bEnableDebugLog = false;

    UPROPERTY(BlueprintAssignable, Category = "JM Gameplay Event")
    FJMGameplayEventDynamicDelegate OnGameplayEventReceived;

    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    bool AddEventTag(FGameplayTag EventTag);

    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    bool RemoveEventTag(FGameplayTag EventTag);

    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    void RefreshSubscriptions();

    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event")
    void UnsubscribeAll();

private:
    TArray<FJMGameplayEventSubscriptionHandle> ActiveHandles;
    void HandleGameplayEvent(const FJMGameplayEventMessage& Message);
};
