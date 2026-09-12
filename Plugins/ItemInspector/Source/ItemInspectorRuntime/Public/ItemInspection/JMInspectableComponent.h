#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/JMGameplayTypes.h"
#include "Interaction/JMInteractableInterface.h"
#include "JMInspectableComponent.generated.h"

class UJMItemInspectionData;
class ULocalPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMInspectableInspectionEvent, UJMItemInspectionData*, InspectionData);

UCLASS(ClassGroup = (JMGameplay), meta = (BlueprintSpawnableComponent))
class ITEMINSPECTORRUNTIME_API UJMInspectableComponent : public UActorComponent, public IJMInteractableInterface
{
	GENERATED_BODY()

public:
	UJMInspectableComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UJMItemInspectionData> InspectionData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bInspectionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bPauseGameDuringInspection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bBlockPlayerInputDuringInspection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Item Inspection")
	bool bHideSourceActorDuringInspection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Interaction")
	FText PromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Gameplay|Interaction")
	int32 InteractionPriority = 0;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMInspectableInspectionEvent OnInspectionRequested;

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	FJMInteractionResult TryOpenInspection(const FJMInteractionContext& Context);

	/** Integration hook for inspect-before-pickup flows that must not return to the source actor. */
	FJMInteractionResult TryOpenInspectionWithOptions(const FJMInteractionContext& Context, bool bDisableExitTransition);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	bool CanInspect(const FJMInteractionContext& Context) const;

	virtual bool CanInteract_Implementation(const FJMInteractionContext& Context) const override;
	virtual FJMInteractionResult BeginInteract_Implementation(const FJMInteractionContext& Context) override;
	virtual FJMInteractionResult CompleteInteract_Implementation(const FJMInteractionContext& Context) override;
	virtual void CancelInteract_Implementation(const FJMInteractionContext& Context) override;
	virtual FJMInteractionPrompt GetInteractionPrompt_Implementation(const FJMInteractionContext& Context) const override;
	virtual float GetInteractionDuration_Implementation(const FJMInteractionContext& Context) const override;
	virtual int32 GetInteractionPriority_Implementation(const FJMInteractionContext& Context) const override;

protected:
	ULocalPlayer* ResolveLocalPlayer(const FJMInteractionContext& Context) const;
};
