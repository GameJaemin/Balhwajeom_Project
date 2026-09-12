#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JMGameplayEventSettings.generated.h"

UCLASS(Config = JMGameplayEvent, DefaultConfig, meta = (DisplayName = "JM Gameplay Event"))
class JMGAMEPLAYEVENT_API UJMGameplayEventSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UJMGameplayEventSettings();

    virtual FName GetCategoryName() const override { return TEXT("JM Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("JM Gameplay Event"); }

    UPROPERTY(Config, EditAnywhere, Category = "Logging")
    bool bEnableDebugLogging = false;

    UPROPERTY(Config, EditAnywhere, Category = "Logging", meta = (EditCondition = "bEnableDebugLogging"))
    bool bLogPublishedEvents = false;

    UPROPERTY(Config, EditAnywhere, Category = "Logging", meta = (EditCondition = "bEnableDebugLogging"))
    bool bLogUnhandledEvents = false;

    UPROPERTY(Config, EditAnywhere, Category = "Safety")
    bool bWarnDuplicateSubscription = true;

    UPROPERTY(Config, EditAnywhere, Category = "Safety", meta = (ClampMin = "1", ClampMax = "128"))
    int32 MaximumNestedDispatchDepth = 16;
};
