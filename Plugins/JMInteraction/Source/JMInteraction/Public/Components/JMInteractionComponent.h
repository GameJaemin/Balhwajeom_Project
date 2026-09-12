#pragma once

#include "Components/ActorComponent.h"
#include "Types/JMGameplayEventTypes.h"
#include "Types/JMInteractionTypes.h"
#include "JMInteractionComponent.generated.h"

class UJMInteractionPromptWidgetBase;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJMInteractableChangedSignature, UObject*, OldInteractable, UObject*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMInteractionPromptChangedSignature, FJMInteractionPrompt, Prompt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJMInteractionFinishedSignature, UObject*, Interactable, FJMInteractionResult, Result);

UCLASS(ClassGroup = (JMInteraction), meta = (BlueprintSpawnableComponent))
class JMINTERACTION_API UJMInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UJMInteractionComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction")
    FJMInteractableChangedSignature OnCurrentInteractableChanged;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction")
    FJMInteractionPromptChangedSignature OnPromptChanged;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction")
    FJMInteractionFinishedSignature OnInteractionFinished;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    bool bInteractionEnabled = true;

    /** If false, detection values are initialized from Project Settings at BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Settings")
    bool bOverrideProjectSettings = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Detection", meta = (ClampMin = "1.0", Units = "cm"))
    float TraceDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Detection", meta = (ClampMin = "0.0", Units = "cm"))
    float TraceRadius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Detection")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Detection")
    EJMInteractionDetectionMode DetectionMode = EJMInteractionDetectionMode::CenterScreenTrace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Detection")
    EJMInteractionTraceMode TraceMode = EJMInteractionTraceMode::Timer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Detection", meta = (ClampMin = "0.01", Units = "s"))
    float TraceInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Debug")
    bool bDrawDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|UI")
    bool bCreatePromptWidget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|UI")
    TSubclassOf<UJMInteractionPromptWidgetBase> InteractionPromptWidgetClass;

    /** Uses the complete style below instead of Project Settings > Plugins > JM Interaction > Prompt UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|UI")
    bool bOverridePromptStyle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|UI", meta = (EditCondition = "bOverridePromptStyle"))
    FJMInteractionPromptStyle PromptStyle;

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    bool RefreshCurrentInteractable();

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    bool RefreshCurrentInteractableFromTrace(FVector TraceStart, FVector TraceDirection);

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    FJMInteractionResult TryInteract();

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    FJMInteractionResult TryBeginInteraction();

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    FJMInteractionResult CompleteInteraction();

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    void CancelInteraction();

    UFUNCTION(BlueprintCallable, Category = "JM Interaction")
    void SetInteractionEnabled(bool bNewEnabled);

    /** Temporarily hides only the prompt. Modal UI should restore its previous value on close. */
    UFUNCTION(BlueprintCallable, Category = "JM Interaction|UI")
    void SetPromptSuppressed(bool bNewSuppressed);

    UFUNCTION(BlueprintPure, Category = "JM Interaction|UI")
    bool IsPromptSuppressed() const { return bPromptSuppressed; }

    UFUNCTION(BlueprintPure, Category = "JM Interaction")
    UObject* GetCurrentInteractableObject() const;

    UFUNCTION(BlueprintPure, Category = "JM Interaction")
    AActor* GetCurrentInteractableActor() const;

    UFUNCTION(BlueprintPure, Category = "JM Interaction")
    FJMInteractionPrompt GetCurrentPrompt() const;

    UFUNCTION(BlueprintPure, Category = "JM Interaction")
    EJMInteractionState GetInteractionState() const;

protected:
    void ApplySettingsDefaults();
    void StartAutoRefresh();
    void StopAutoRefresh();
    void HandleAutoRefresh();
    void SetCurrentInteractable(UObject* NewInteractable, AActor* NewActor);
    void ClearCurrentInteractable();
    bool FindInteractable(FVector TraceStart, FVector TraceDirection, UObject*& OutInteractable, AActor*& OutActor, FHitResult& OutHitResult) const;
    bool FindInteractableUnderCursor(UObject*& OutInteractable, AActor*& OutActor, FHitResult& OutHitResult) const;
    UObject* ResolveInteractableFromHit(const FHitResult& HitResult) const;
    FJMInteractionContext BuildInteractionContext(UObject* InteractableObject) const;
    bool ResolveInteractionView(FVector& OutLocation, FVector& OutDirection) const;
    APlayerController* ResolvePlayerController() const;
    void CreatePromptWidget();
    void UpdateInteractionPrompt(bool bForceUpdate = false);
    void ShowInteractionPrompt(const FJMInteractionPrompt& Prompt);
    void HideInteractionPrompt();
    void PublishInteractionEvent(FGameplayTag EventTag, UObject* InteractableObject, const FJMInteractionResult& Result, const FJMInteractionContext& Context);
    void HandleModalOpened(const FJMGameplayEventMessage& Message);
    void HandleModalClosed(const FJMGameplayEventMessage& Message);

    UPROPERTY(Transient)
    TWeakObjectPtr<UObject> CurrentInteractableObject;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> CurrentInteractableActor;

    UPROPERTY(Transient)
    TWeakObjectPtr<UObject> ActiveInteractableObject;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> ActiveInteractableActor;

    UPROPERTY(Transient)
    TObjectPtr<UJMInteractionPromptWidgetBase> PromptWidget;

    UPROPERTY(Transient)
    EJMInteractionState InteractionState = EJMInteractionState::Idle;

    UPROPERTY(Transient)
    FJMInteractionPrompt LastDisplayedPrompt;

    UPROPERTY(Transient)
    bool bHasDisplayedPrompt = false;

    UPROPERTY(Transient)
    bool bPromptSuppressed = false;
    bool bPromptSuppressedBeforeModal = false;
    int32 ModalSuppressionDepth = 0;
    FJMGameplayEventSubscriptionHandle ModalOpenedHandle;
    FJMGameplayEventSubscriptionHandle ModalClosedHandle;

    FTimerHandle TraceTimerHandle;
};
