#pragma once

#include "CoreMinimal.h"
#include "Types/JMInteractionTypes.h"
#include "UObject/Interface.h"
#include "JMInteractableInterface.generated.h"

UINTERFACE(BlueprintType)
class JMINTERACTION_API UJMInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class JMINTERACTION_API IJMInteractableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    bool CanInteract(const FJMInteractionContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    FJMInteractionResult BeginInteract(const FJMInteractionContext& Context);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    FJMInteractionResult CompleteInteract(const FJMInteractionContext& Context);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    void CancelInteract(const FJMInteractionContext& Context);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    FJMInteractionPrompt GetInteractionPrompt(const FJMInteractionContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    float GetInteractionDuration(const FJMInteractionContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    int32 GetInteractionPriority(const FJMInteractionContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    void OnFocusBegin(const FJMInteractionContext& Context);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    void OnFocusEnd(const FJMInteractionContext& Context);
};
