#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "Types/JMInteractionTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "JMInteractionSettings.generated.h"

class UJMInteractionPromptWidgetBase;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Interaction", ToolTip = "Configure the reusable JM interaction system."))
class JMINTERACTION_API UJMInteractionSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UJMInteractionSettings();
    static const UJMInteractionSettings* Get();
    virtual FName GetCategoryName() const override;

    UPROPERTY(EditAnywhere, Config, Category = "Detection", meta = (ClampMin = "1.0", Units = "cm"))
    float DefaultInteractionDistance = 500.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Detection", meta = (ClampMin = "0.0", Units = "cm"))
    float DefaultTraceRadius = 0.0f;

    UPROPERTY(EditAnywhere, Config, Category = "Detection")
    TEnumAsByte<ECollisionChannel> DefaultTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, Config, Category = "Detection")
    EJMInteractionDetectionMode DefaultDetectionMode = EJMInteractionDetectionMode::CenterScreenTrace;

    UPROPERTY(EditAnywhere, Config, Category = "Detection")
    EJMInteractionTraceMode DefaultTraceMode = EJMInteractionTraceMode::Timer;

    UPROPERTY(EditAnywhere, Config, Category = "Detection", meta = (ClampMin = "0.01", Units = "s"))
    float DefaultTraceInterval = 0.1f;

    UPROPERTY(EditAnywhere, Config, Category = "Debug")
    bool bDefaultDebugTrace = false;

    UPROPERTY(EditAnywhere, Config, Category = "Prompt UI")
    TSoftClassPtr<UJMInteractionPromptWidgetBase> DefaultPromptWidgetClass;

    UPROPERTY(EditAnywhere, Config, Category = "Prompt UI")
    FJMInteractionPromptStyle DefaultPromptStyle;
};
