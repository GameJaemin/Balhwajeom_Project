#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/JMInteractionTypes.h"
#include "JMInteractionInputInterceptorInterface.generated.h"

/**
 * Optional host-input preprocessor. Implementations may consume an interaction
 * command before world targeting; JMInteraction has no dependency on them.
 */
UINTERFACE(BlueprintType)
class JMINTERACTION_API UJMInteractionInputInterceptorInterface : public UInterface
{
    GENERATED_BODY()
};

class JMINTERACTION_API IJMInteractionInputInterceptorInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="JM Interaction|Input")
    bool TryHandleInteractionInput(FJMInteractionResult& OutResult);
};
