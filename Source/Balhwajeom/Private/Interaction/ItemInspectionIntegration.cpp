#include "Interaction/ItemInspectionIntegration.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ItemInspection/JMInspectableComponent.h"
#include "ItemInspection/JMItemInspectionSubsystem.h"
#include "Tablet/BalhwajeomTabletComponent.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"

namespace
{
	const APlayerController* ResolveController(const AActor* Actor)
	{
		if (const APawn* Pawn = Cast<APawn>(Actor)) return Cast<APlayerController>(Pawn->GetController());
		return Cast<APlayerController>(Actor);
	}
	FJMInteractionContext MakeContext(APawn* Pawn, AActor* Target)
	{
		FJMInteractionContext Context;
		Context.InstigatorActor = Pawn;
		Context.InstigatorController = Pawn ? Pawn->GetController() : nullptr;
		Context.TargetActor = Target;
		Context.InteractionLocation = IsValid(Target) ? Target->GetActorLocation() : FVector::ZeroVector;
		return Context;
	}
	UJMInspectableComponent* ResolveInspectable(AActor* Target, APawn* Pawn)
	{
		const APlayerController* Controller = ResolveController(Pawn);
		if (!IsValid(Target) || !Controller || !Controller->IsLocalController()
			|| BalhwajeomItemInspection::IsOpen(Pawn) || BalhwajeomItemInspection::IsOtherModalOpen(Pawn)) return nullptr;
		TInlineComponentArray<UJMInspectableComponent*> Components(Target);
		UJMInspectableComponent* Best = nullptr;
		const FJMInteractionContext Context = MakeContext(Pawn, Target);
		for (UJMInspectableComponent* Component : Components)
		{
			if (IsValid(Component) && Component->CanInspect(Context)
				&& (!Best || Component->InteractionPriority > Best->InteractionPriority)) Best = Component;
		}
		return Best;
	}
}
bool BalhwajeomItemInspection::IsOpen(const AActor* PlayerActor)
{
	const APlayerController* Controller = ResolveController(PlayerActor);
	const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UJMItemInspectionSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UJMItemInspectionSubsystem>() : nullptr;
	return Subsystem && Subsystem->IsInspectionOpen();
}
bool BalhwajeomItemInspection::IsOtherModalOpen(const AActor* PlayerActor)
{
	const APlayerController* Controller = ResolveController(PlayerActor);
	const APawn* Pawn = Controller ? Controller->GetPawn() : Cast<APawn>(PlayerActor);
	const UBalhwajeomTabletComponent* Tablet = Controller ? Controller->FindComponentByClass<UBalhwajeomTabletComponent>() : nullptr;
	if (!Tablet && Pawn) Tablet = Pawn->FindComponentByClass<UBalhwajeomTabletComponent>();
	const UBalhwajeomPhotoCameraComponent* Camera = Pawn ? Pawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>() : nullptr;
	return (Tablet && Tablet->IsTabletOpen()) || (Camera && (Camera->IsInCameraMode() || Camera->IsCameraTransitioning()));
}
bool BalhwajeomItemInspection::CanInspect(AActor* Target, APawn* Pawn)
{
	return ResolveInspectable(Target, Pawn) != nullptr;
}
bool BalhwajeomItemInspection::TryInspect(AActor* Target, APawn* Pawn)
{
	UJMInspectableComponent* Component = ResolveInspectable(Target, Pawn);
	return Component && Component->TryOpenInspection(MakeContext(Pawn, Target)).bSucceeded;
}
