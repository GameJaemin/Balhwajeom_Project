#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "JMGameplayEventTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EJMGameplayEventMatchType : uint8
{
    Exact UMETA(DisplayName = "Exact Match"),
    IncludeChildren UMETA(DisplayName = "Include Child Tags")
};

USTRUCT(BlueprintType)
struct JMGAMEPLAYEVENT_API FJMGameplayEventSubscriptionHandle
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "JM Gameplay Event")
    FGuid Id;

    bool IsValid() const { return Id.IsValid(); }
    void Reset() { Id.Invalidate(); }

    friend bool operator==(const FJMGameplayEventSubscriptionHandle& A, const FJMGameplayEventSubscriptionHandle& B)
    {
        return A.Id == B.Id;
    }
};

USTRUCT(BlueprintType)
struct JMGAMEPLAYEVENT_API FJMGameplayEventMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay Event")
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay Event")
    TObjectPtr<UObject> Source = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay Event")
    TObjectPtr<AActor> Instigator = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay Event")
    TObjectPtr<UObject> Target = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay Event")
    FGameplayTagContainer ContextTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay Event")
    TObjectPtr<UObject> Payload = nullptr;
};

DECLARE_DELEGATE_OneParam(FJMGameplayEventNativeDelegate, const FJMGameplayEventMessage&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMGameplayEventDynamicDelegate, const FJMGameplayEventMessage&, Message);
