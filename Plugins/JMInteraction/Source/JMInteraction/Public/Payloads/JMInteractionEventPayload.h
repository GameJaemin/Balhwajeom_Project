#pragma once

#include "CoreMinimal.h"
#include "Payloads/JMGameplayEventPayloadBase.h"
#include "Types/JMInteractionTypes.h"
#include "JMInteractionEventPayload.generated.h"

UCLASS(BlueprintType)
class JMINTERACTION_API UJMInteractionEventPayload : public UJMGameplayEventPayloadBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "JM Interaction|Events")
    FJMInteractionResult Result;

    UPROPERTY(BlueprintReadOnly, Category = "JM Interaction|Events")
    FJMInteractionContext Context;
};
