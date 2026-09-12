#include "ItemInspection/JMInspectableComponent.h"

#include "Core/JMGameplayLog.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "ItemInspection/JMItemInspectionSubsystem.h"

UJMInspectableComponent::UJMInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PromptText = NSLOCTEXT("JMGameplay", "DefaultInspectablePrompt", "Inspect");
}

FJMInteractionResult UJMInspectableComponent::TryOpenInspection(const FJMInteractionContext& Context)
{
	return TryOpenInspectionWithOptions(Context, false);
}

FJMInteractionResult UJMInspectableComponent::TryOpenInspectionWithOptions(
	const FJMInteractionContext& Context,
	bool bDisableExitTransition)
{
	if (!CanInspect(Context))
	{
		return FJMInteractionResult::Failure(EJMInteractionResultCode::NotAllowed, NSLOCTEXT("JMGameplay", "InspectNotAllowed", "This object cannot be inspected."));
	}

	ULocalPlayer* LocalPlayer = ResolveLocalPlayer(Context);
	if (!LocalPlayer)
	{
		return FJMInteractionResult::Failure(EJMInteractionResultCode::Failed, NSLOCTEXT("JMGameplay", "InspectLocalPlayerMissing", "A local player is required to open the inspector."));
	}

	UJMItemInspectionSubsystem* InspectionSubsystem = LocalPlayer->GetSubsystem<UJMItemInspectionSubsystem>();
	if (!InspectionSubsystem)
	{
		return FJMInteractionResult::Failure(EJMInteractionResultCode::Failed, NSLOCTEXT("JMGameplay", "InspectSubsystemMissing", "Item inspection subsystem is unavailable."));
	}

	FJMItemInspectionRequest Request;
	Request.InspectionData = InspectionData;
	Request.SourceActor = GetOwner();
	Request.RequestSource = this;
	Request.bPauseGame = bPauseGameDuringInspection;
	Request.bBlockPlayerInput = bBlockPlayerInputDuringInspection;
	Request.bHideSourceActor = bHideSourceActorDuringInspection;
	Request.bDisableExitTransition = bDisableExitTransition;

	OnInspectionRequested.Broadcast(InspectionData);

	if (!InspectionSubsystem->OpenInspectionFromRequest(Request))
	{
		return FJMInteractionResult::Failure(EJMInteractionResultCode::Failed, NSLOCTEXT("JMGameplay", "InspectOpenFailed", "Failed to open the item inspector."));
	}

	return FJMInteractionResult::Success(NSLOCTEXT("JMGameplay", "InspectOpenSucceeded", "Item inspector opened."));
}

bool UJMInspectableComponent::CanInspect_Implementation(const FJMInteractionContext& Context) const
{
	return bInspectionEnabled && IsValid(InspectionData);
}

bool UJMInspectableComponent::CanInteract_Implementation(const FJMInteractionContext& Context) const
{
	return CanInspect(Context);
}

FJMInteractionResult UJMInspectableComponent::BeginInteract_Implementation(const FJMInteractionContext& Context)
{
	return CanInspect(Context)
		? FJMInteractionResult::Success()
		: FJMInteractionResult::Failure(EJMInteractionResultCode::NotAllowed, NSLOCTEXT("JMGameplay", "InspectableBeginFailed", "Inspection is not available."));
}

FJMInteractionResult UJMInspectableComponent::CompleteInteract_Implementation(const FJMInteractionContext& Context)
{
	return TryOpenInspection(Context);
}

void UJMInspectableComponent::CancelInteract_Implementation(const FJMInteractionContext& Context)
{
}

FJMInteractionPrompt UJMInspectableComponent::GetInteractionPrompt_Implementation(const FJMInteractionContext& Context) const
{
	FJMInteractionPrompt Prompt;
	Prompt.DisplayText = PromptText;
	Prompt.Priority = InteractionPriority;
	Prompt.HoldDuration = 0.0f;
	Prompt.bVisible = CanInspect(Context);
	return Prompt;
}

float UJMInspectableComponent::GetInteractionDuration_Implementation(const FJMInteractionContext& Context) const
{
	return 0.0f;
}

int32 UJMInspectableComponent::GetInteractionPriority_Implementation(const FJMInteractionContext& Context) const
{
	return InteractionPriority;
}

ULocalPlayer* UJMInspectableComponent::ResolveLocalPlayer(const FJMInteractionContext& Context) const
{
	APlayerController* PlayerController = Cast<APlayerController>(Context.InstigatorController);
	if (!PlayerController)
	{
		if (const APawn* Pawn = Cast<APawn>(Context.InstigatorActor))
		{
			PlayerController = Cast<APlayerController>(Pawn->GetController());
		}
	}

	if (!PlayerController)
	{
		PlayerController = Cast<APlayerController>(Context.InstigatorActor);
	}

	return PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
}
