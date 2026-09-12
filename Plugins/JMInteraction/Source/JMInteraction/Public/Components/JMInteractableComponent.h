#pragma once

#include "Components/ActorComponent.h"
#include "Interfaces/JMInteractableInterface.h"
#include "JMInteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMInteractableEvent, const FJMInteractionContext&, Context);

UCLASS(ClassGroup = (JMInteraction), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class JMINTERACTION_API UJMInteractableComponent : public UActorComponent, public IJMInteractableInterface
{
    GENERATED_BODY()

public:
    UJMInteractableComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    bool bInteractionEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    FText InteractionPrompt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    FGameplayTag InteractionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction", meta = (ClampMin = "0.0"))
    float InteractionDuration = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    int32 InteractionPriority = 0;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction")
    FJMInteractableEvent OnInteracted;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction")
    FJMInteractableEvent OnFocused;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction")
    FJMInteractableEvent OnUnfocused;

    UFUNCTION(BlueprintNativeEvent, Category = "JM Interaction")
    bool EvaluateCanInteract(const FJMInteractionContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, Category = "JM Interaction")
    FJMInteractionResult HandleInteract(const FJMInteractionContext& Context);

    virtual bool CanInteract_Implementation(const FJMInteractionContext& Context) const override;
    virtual FJMInteractionResult BeginInteract_Implementation(const FJMInteractionContext& Context) override;
    virtual FJMInteractionResult CompleteInteract_Implementation(const FJMInteractionContext& Context) override;
    virtual void CancelInteract_Implementation(const FJMInteractionContext& Context) override;
    virtual FJMInteractionPrompt GetInteractionPrompt_Implementation(const FJMInteractionContext& Context) const override;
    virtual float GetInteractionDuration_Implementation(const FJMInteractionContext& Context) const override;
    virtual int32 GetInteractionPriority_Implementation(const FJMInteractionContext& Context) const override;
    virtual void OnFocusBegin_Implementation(const FJMInteractionContext& Context) override;
    virtual void OnFocusEnd_Implementation(const FJMInteractionContext& Context) override;
};
