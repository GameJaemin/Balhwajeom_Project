#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "JMGameplayEventPayloadBase.generated.h"

/** Base class for transient, domain-owned event data. Feature plugins subclass this type. */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Transient)
class JMGAMEPLAYEVENT_API UJMGameplayEventPayloadBase : public UObject
{
    GENERATED_BODY()

public:
    /** Optional stable content identifier used by event consumers such as objective systems. */
    UPROPERTY(BlueprintReadOnly, Category = "JM Gameplay Event|Routing")
    FName ObjectiveTargetIdentifier;

    /** Optional event quantity. Producers should keep this positive when it represents progress. */
    UPROPERTY(BlueprintReadOnly, Category = "JM Gameplay Event|Routing")
    int32 ObjectiveProgressAmount = 1;

    /** Optional payload-owned context merged with the message context by generic consumers. */
    UPROPERTY(BlueprintReadOnly, Category = "JM Gameplay Event|Routing")
    FGameplayTagContainer ObjectiveContextTags;
};
