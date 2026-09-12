#include "Components/JMInteractableComponent.h"

UJMInteractableComponent::UJMInteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UJMInteractableComponent::EvaluateCanInteract_Implementation(const FJMInteractionContext& Context) const { return true; }
FJMInteractionResult UJMInteractableComponent::HandleInteract_Implementation(const FJMInteractionContext& Context) { return FJMInteractionResult::Success(); }
bool UJMInteractableComponent::CanInteract_Implementation(const FJMInteractionContext& Context) const { return bInteractionEnabled && EvaluateCanInteract(Context); }
FJMInteractionResult UJMInteractableComponent::BeginInteract_Implementation(const FJMInteractionContext& Context)
{
    return CanInteract_Implementation(Context) ? FJMInteractionResult::Success() : FJMInteractionResult::Failure(EJMInteractionResultCode::NotAllowed, FText::GetEmpty());
}
FJMInteractionResult UJMInteractableComponent::CompleteInteract_Implementation(const FJMInteractionContext& Context)
{
    if (!CanInteract_Implementation(Context)) return FJMInteractionResult::Failure(EJMInteractionResultCode::NotAllowed, FText::GetEmpty());
    const FJMInteractionResult Result = HandleInteract(Context);
    if (Result.bSucceeded) OnInteracted.Broadcast(Context);
    return Result;
}
void UJMInteractableComponent::CancelInteract_Implementation(const FJMInteractionContext& Context) { }
FJMInteractionPrompt UJMInteractableComponent::GetInteractionPrompt_Implementation(const FJMInteractionContext& Context) const
{
    FJMInteractionPrompt Prompt;
    Prompt.DisplayText = InteractionPrompt;
    Prompt.InteractionTag = InteractionTag;
    Prompt.HoldDuration = FMath::Max(0.0f, InteractionDuration);
    Prompt.Priority = InteractionPriority;
    Prompt.bVisible = CanInteract_Implementation(Context);
    return Prompt;
}
float UJMInteractableComponent::GetInteractionDuration_Implementation(const FJMInteractionContext& Context) const { return FMath::Max(0.0f, InteractionDuration); }
int32 UJMInteractableComponent::GetInteractionPriority_Implementation(const FJMInteractionContext& Context) const { return InteractionPriority; }
void UJMInteractableComponent::OnFocusBegin_Implementation(const FJMInteractionContext& Context) { OnFocused.Broadcast(Context); }
void UJMInteractableComponent::OnFocusEnd_Implementation(const FJMInteractionContext& Context) { OnUnfocused.Broadcast(Context); }
