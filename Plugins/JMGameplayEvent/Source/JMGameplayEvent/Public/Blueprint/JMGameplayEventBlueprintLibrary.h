#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/JMGameplayEventTypes.h"
#include "JMGameplayEventBlueprintLibrary.generated.h"

class UJMGameplayEventSubsystem;

UCLASS()
class JMGAMEPLAYEVENT_API UJMGameplayEventBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "JM Gameplay Event", meta = (WorldContext = "WorldContextObject", DisplayName = "Publish Gameplay Event"))
    static int32 PublishGameplayEvent(
        const UObject* WorldContextObject,
        FGameplayTag EventTag,
        UObject* Source,
        AActor* Instigator,
        UObject* Target,
        FGameplayTagContainer ContextTags,
        UObject* Payload);

    UFUNCTION(BlueprintPure, Category = "JM Gameplay Event", meta = (WorldContext = "WorldContextObject"))
    static UJMGameplayEventSubsystem* GetGameplayEventSubsystem(const UObject* WorldContextObject);
};
