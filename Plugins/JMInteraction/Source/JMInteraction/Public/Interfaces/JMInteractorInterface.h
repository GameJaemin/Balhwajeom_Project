#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "JMInteractorInterface.generated.h"

UINTERFACE(BlueprintType)
class JMINTERACTION_API UJMInteractorInterface : public UInterface
{
    GENERATED_BODY()
};

class JMINTERACTION_API IJMInteractorInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    void GetInteractionView(FVector& OutLocation, FVector& OutDirection) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction")
    FGameplayTagContainer GetInteractorTags() const;
};
