#include "Components/JMInteractionComponent.h"

#include "Interfaces/JMInteractionInputInterceptorInterface.h"

#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/JMInteractableInterface.h"
#include "Interfaces/JMInteractorInterface.h"
#include "JMGameplayUIEventTags.h"
#include "Payloads/JMInteractionEventPayload.h"
#include "Settings/JMInteractionSettings.h"
#include "Subsystems/JMGameplayEventSubsystem.h"
#include "TimerManager.h"
#include "Types/JMInteractionEventTags.h"
#include "UI/JMInteractionPromptWidgetBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogJMInteractionPrompt, Log, All);

namespace
{
    UObject* FindInteractableOnActor(AActor* Actor)
    {
        if (!IsValid(Actor)) return nullptr;
        if (Actor->GetClass()->ImplementsInterface(UJMInteractableInterface::StaticClass())) return Actor;
        for (UActorComponent* Component : Actor->GetComponents())
        {
            if (IsValid(Component) && Component->GetClass()->ImplementsInterface(UJMInteractableInterface::StaticClass())) return Component;
        }
        return nullptr;
    }

    AActor* ActorFromInteractable(UObject* Object, AActor* Fallback)
    {
        if (AActor* Actor = Cast<AActor>(Object)) return Actor;
        if (const UActorComponent* Component = Cast<UActorComponent>(Object)) return Component->GetOwner();
        return Fallback;
    }

    bool ArePromptsEqual(const FJMInteractionPrompt& A, const FJMInteractionPrompt& B)
    {
        return A.DisplayText.EqualTo(B.DisplayText)
            && A.InteractionTag == B.InteractionTag
            && FMath::IsNearlyEqual(A.HoldDuration, B.HoldDuration)
            && A.Priority == B.Priority
            && A.bVisible == B.bVisible;
    }
}

UJMInteractionComponent::UJMInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UJMInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplySettingsDefaults();
    CreatePromptWidget();
    if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UJMGameplayEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UJMGameplayEventSubsystem>())
        {
            FJMGameplayEventNativeDelegate OpenedCallback;
            OpenedCallback.BindUObject(this, &UJMInteractionComponent::HandleModalOpened);
            ModalOpenedHandle = EventSubsystem->SubscribeEvent(
                JMGameplayUIEventTags::ModalOpened, this, EJMGameplayEventMatchType::Exact, MoveTemp(OpenedCallback));
            FJMGameplayEventNativeDelegate ClosedCallback;
            ClosedCallback.BindUObject(this, &UJMInteractionComponent::HandleModalClosed);
            ModalClosedHandle = EventSubsystem->SubscribeEvent(
                JMGameplayUIEventTags::ModalClosed, this, EJMGameplayEventMatchType::Exact, MoveTemp(ClosedCallback));
        }
    }
    StartAutoRefresh();
}

void UJMInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAutoRefresh();
    CancelInteraction();
    ClearCurrentInteractable();
    if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UJMGameplayEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UJMGameplayEventSubsystem>())
        {
            EventSubsystem->UnsubscribeAll(this);
        }
    }
    if (PromptWidget) PromptWidget->RemoveFromParent();
    PromptWidget = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UJMInteractionComponent::HandleModalOpened(const FJMGameplayEventMessage& Message)
{
    if (ModalSuppressionDepth == 0)
    {
        bPromptSuppressedBeforeModal = bPromptSuppressed;
    }
    ++ModalSuppressionDepth;
    SetPromptSuppressed(true);
}

void UJMInteractionComponent::HandleModalClosed(const FJMGameplayEventMessage& Message)
{
    ModalSuppressionDepth = FMath::Max(0, ModalSuppressionDepth - 1);
    if (ModalSuppressionDepth == 0)
    {
        SetPromptSuppressed(bPromptSuppressedBeforeModal);
    }
}

void UJMInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (TraceMode == EJMInteractionTraceMode::Tick) RefreshCurrentInteractable();
}

bool UJMInteractionComponent::RefreshCurrentInteractable()
{
    if (!bInteractionEnabled)
    {
        ClearCurrentInteractable();
        return false;
    }

    UObject* FoundObject = nullptr;
    AActor* FoundActor = nullptr;
    FHitResult Hit;
    bool bFound = false;
    if (DetectionMode == EJMInteractionDetectionMode::MouseCursorTrace)
    {
        bFound = FindInteractableUnderCursor(FoundObject, FoundActor, Hit);
    }
    else
    {
        FVector Start, Direction;
        bFound = ResolveInteractionView(Start, Direction) && FindInteractable(Start, Direction, FoundObject, FoundActor, Hit);
    }
    SetCurrentInteractable(FoundObject, FoundActor);
    return bFound;
}

bool UJMInteractionComponent::RefreshCurrentInteractableFromTrace(FVector TraceStart, FVector TraceDirection)
{
    if (!bInteractionEnabled) { ClearCurrentInteractable(); return false; }
    UObject* FoundObject = nullptr;
    AActor* FoundActor = nullptr;
    FHitResult Hit;
    const bool bFound = FindInteractable(TraceStart, TraceDirection, FoundObject, FoundActor, Hit);
    SetCurrentInteractable(FoundObject, FoundActor);
    return bFound;
}

FJMInteractionResult UJMInteractionComponent::TryInteract()
{
    return TryBeginInteraction();
}

FJMInteractionResult UJMInteractionComponent::TryBeginInteraction()
{
    if (AActor* OwnerActor = GetOwner())
    {
        TInlineComponentArray<UActorComponent*> Components(OwnerActor);
        for (UActorComponent* Component : Components)
        {
            if (!IsValid(Component) ||
                !Component->GetClass()->ImplementsInterface(
                    UJMInteractionInputInterceptorInterface::StaticClass()))
            {
                continue;
            }
            FJMInteractionResult InterceptedResult;
            if (IJMInteractionInputInterceptorInterface::Execute_TryHandleInteractionInput(
                Component, InterceptedResult))
            {
                return InterceptedResult;
            }
        }
    }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug) UE_LOG(LogJMInteractionPrompt, Log, TEXT("Interaction Input Received: Owner=%s Target=%s"), *GetNameSafe(GetOwner()), *GetNameSafe(CurrentInteractableObject.Get()));
#endif
    if (!bInteractionEnabled)
    {
        const FJMInteractionResult Result = FJMInteractionResult::Failure(EJMInteractionResultCode::Disabled, NSLOCTEXT("JMInteraction", "Disabled", "Interaction is disabled."));
        PublishInteractionEvent(JMInteractionEventTags::Failed, nullptr, Result, FJMInteractionContext());
        return Result;
    }
    if (InteractionState == EJMInteractionState::Interacting || InteractionState == EJMInteractionState::Holding)
    {
        const FJMInteractionResult Result = FJMInteractionResult::Failure(EJMInteractionResultCode::AlreadyInteracting, NSLOCTEXT("JMInteraction", "Busy", "An interaction is already in progress."));
        UObject* ActiveObject = ActiveInteractableObject.Get();
        PublishInteractionEvent(JMInteractionEventTags::Failed, ActiveObject, Result, IsValid(ActiveObject) ? BuildInteractionContext(ActiveObject) : FJMInteractionContext());
        return Result;
    }
    if (!CurrentInteractableObject.IsValid() && TraceMode == EJMInteractionTraceMode::OnInput) RefreshCurrentInteractable();

    UObject* Object = CurrentInteractableObject.Get();
    if (!IsValid(Object))
    {
        const FJMInteractionResult Result = FJMInteractionResult::Failure(EJMInteractionResultCode::NoTarget, NSLOCTEXT("JMInteraction", "NoTarget", "No interactable target was found."));
        PublishInteractionEvent(JMInteractionEventTags::Failed, nullptr, Result, FJMInteractionContext());
        return Result;
    }
    const FJMInteractionContext Context = BuildInteractionContext(Object);
    if (!IJMInteractableInterface::Execute_CanInteract(Object, Context))
    {
        const FJMInteractionResult Result = FJMInteractionResult::Failure(EJMInteractionResultCode::NotAllowed, NSLOCTEXT("JMInteraction", "NotAllowed", "The target cannot be interacted with."));
        PublishInteractionEvent(JMInteractionEventTags::Failed, Object, Result, Context);
        return Result;
    }

    InteractionState = EJMInteractionState::Interacting;
    ActiveInteractableObject = Object;
    ActiveInteractableActor = Context.TargetActor;
    const FJMInteractionResult BeginResult = IJMInteractableInterface::Execute_BeginInteract(Object, Context);
    if (!BeginResult.bSucceeded)
    {
        ActiveInteractableObject.Reset();
        ActiveInteractableActor.Reset();
        InteractionState = EJMInteractionState::Idle;
        OnInteractionFinished.Broadcast(Object, BeginResult);
        PublishInteractionEvent(JMInteractionEventTags::Failed, Object, BeginResult, Context);
        UpdateInteractionPrompt(true);
        return BeginResult;
    }
    if (IJMInteractableInterface::Execute_GetInteractionDuration(Object, Context) <= KINDA_SMALL_NUMBER) return CompleteInteraction();
    InteractionState = EJMInteractionState::Holding;
    return BeginResult;
}

FJMInteractionResult UJMInteractionComponent::CompleteInteraction()
{
    UObject* Object = ActiveInteractableObject.Get();
    if (!IsValid(Object))
    {
        ActiveInteractableObject.Reset();
        ActiveInteractableActor.Reset();
        InteractionState = bInteractionEnabled ? EJMInteractionState::Idle : EJMInteractionState::Disabled;
        const FJMInteractionResult Result = FJMInteractionResult::Failure(EJMInteractionResultCode::TargetInvalid, NSLOCTEXT("JMInteraction", "InvalidTarget", "The active target is no longer valid."));
        PublishInteractionEvent(JMInteractionEventTags::Failed, nullptr, Result, FJMInteractionContext());
        return Result;
    }
    const FJMInteractionContext Context = BuildInteractionContext(Object);
    const FJMInteractionResult Result = IJMInteractableInterface::Execute_CompleteInteract(Object, Context);
    ActiveInteractableObject.Reset();
    ActiveInteractableActor.Reset();
    InteractionState = bInteractionEnabled
        ? (CurrentInteractableObject.IsValid() ? EJMInteractionState::Targeting : EJMInteractionState::Idle)
        : EJMInteractionState::Disabled;
    OnInteractionFinished.Broadcast(Object, Result);
    PublishInteractionEvent(Result.bSucceeded ? JMInteractionEventTags::Succeeded : JMInteractionEventTags::Failed, Object, Result, Context);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug) UE_LOG(LogJMInteractionPrompt, Log, TEXT("Interact Executed: Target=%s Success=%s"), *GetNameSafe(Object), Result.bSucceeded ? TEXT("true") : TEXT("false"));
#endif
    if (bInteractionEnabled) RefreshCurrentInteractable();
    return Result;
}

void UJMInteractionComponent::CancelInteraction()
{
    if (UObject* Object = ActiveInteractableObject.Get(); IsValid(Object))
        IJMInteractableInterface::Execute_CancelInteract(Object, BuildInteractionContext(Object));
    ActiveInteractableObject.Reset();
    ActiveInteractableActor.Reset();
    InteractionState = bInteractionEnabled
        ? (CurrentInteractableObject.IsValid() ? EJMInteractionState::Targeting : EJMInteractionState::Idle)
        : EJMInteractionState::Disabled;
}

void UJMInteractionComponent::SetInteractionEnabled(bool bNewEnabled)
{
    if (bInteractionEnabled == bNewEnabled) return;
    bInteractionEnabled = bNewEnabled;
    if (!bNewEnabled)
    {
        CancelInteraction();
        ClearCurrentInteractable();
        InteractionState = EJMInteractionState::Disabled;
        StopAutoRefresh();
    }
    else
    {
        InteractionState = EJMInteractionState::Idle;
        StartAutoRefresh();
    }
}

void UJMInteractionComponent::SetPromptSuppressed(bool bNewSuppressed)
{
    if (bPromptSuppressed == bNewSuppressed) return;

    bPromptSuppressed = bNewSuppressed;
    if (bPromptSuppressed)
    {
        HideInteractionPrompt();
    }
    else
    {
        // The cached target normally remains unchanged while a modal UI is open.
        UpdateInteractionPrompt(true);
    }
}

UObject* UJMInteractionComponent::GetCurrentInteractableObject() const { return CurrentInteractableObject.Get(); }
AActor* UJMInteractionComponent::GetCurrentInteractableActor() const { return CurrentInteractableActor.Get(); }
EJMInteractionState UJMInteractionComponent::GetInteractionState() const { return InteractionState; }

FJMInteractionPrompt UJMInteractionComponent::GetCurrentPrompt() const
{
    UObject* Object = CurrentInteractableObject.Get();
    return IsValid(Object) ? IJMInteractableInterface::Execute_GetInteractionPrompt(Object, BuildInteractionContext(Object)) : FJMInteractionPrompt();
}

void UJMInteractionComponent::ApplySettingsDefaults()
{
    const UJMInteractionSettings* Settings = UJMInteractionSettings::Get();
    if (!Settings) return;
    if (!bOverrideProjectSettings)
    {
        TraceDistance = Settings->DefaultInteractionDistance;
        TraceRadius = Settings->DefaultTraceRadius;
        TraceChannel = Settings->DefaultTraceChannel;
        DetectionMode = Settings->DefaultDetectionMode;
        TraceMode = Settings->DefaultTraceMode;
        TraceInterval = Settings->DefaultTraceInterval;
        bDrawDebug = Settings->bDefaultDebugTrace;
    }
    if (!InteractionPromptWidgetClass && !Settings->DefaultPromptWidgetClass.IsNull())
        InteractionPromptWidgetClass = Settings->DefaultPromptWidgetClass.LoadSynchronous();
    if (!bOverridePromptStyle)
        PromptStyle = Settings->DefaultPromptStyle;
}

void UJMInteractionComponent::StartAutoRefresh()
{
    if (!bInteractionEnabled) return;
    StopAutoRefresh();
    if (TraceMode == EJMInteractionTraceMode::Tick)
    {
        RefreshCurrentInteractable();
        SetComponentTickEnabled(true);
    }
    else if (TraceMode == EJMInteractionTraceMode::Timer && GetWorld())
    {
        RefreshCurrentInteractable();
        GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &UJMInteractionComponent::HandleAutoRefresh, FMath::Max(0.01f, TraceInterval), true);
    }
}

void UJMInteractionComponent::StopAutoRefresh()
{
    SetComponentTickEnabled(false);
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);
}

void UJMInteractionComponent::HandleAutoRefresh() { RefreshCurrentInteractable(); }

void UJMInteractionComponent::SetCurrentInteractable(UObject* NewObject, AActor* NewActor)
{
    UObject* OldObject = CurrentInteractableObject.Get();
    if (OldObject == NewObject)
    {
        UpdateInteractionPrompt(false);
        return;
    }
    if (IsValid(OldObject))
    {
        const FJMInteractionContext OldContext = BuildInteractionContext(OldObject);
        IJMInteractableInterface::Execute_OnFocusEnd(OldObject, OldContext);
        PublishInteractionEvent(JMInteractionEventTags::FocusEnded, OldObject, FJMInteractionResult(), OldContext);
    }
    CurrentInteractableObject = NewObject;
    CurrentInteractableActor = NewActor;
    InteractionState = IsValid(NewObject) ? EJMInteractionState::Targeting : EJMInteractionState::Idle;
    if (IsValid(NewObject))
    {
        const FJMInteractionContext NewContext = BuildInteractionContext(NewObject);
        IJMInteractableInterface::Execute_OnFocusBegin(NewObject, NewContext);
        PublishInteractionEvent(JMInteractionEventTags::FocusStarted, NewObject, FJMInteractionResult(), NewContext);
    }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug)
    {
        if (IsValid(NewObject))
        {
            UE_LOG(LogJMInteractionPrompt, Log, TEXT("New Interactable Focused: %s"), *GetNameSafe(NewObject));
        }
        else
        {
            UE_LOG(LogJMInteractionPrompt, Log, TEXT("Interactable Focus Cleared: %s"), *GetNameSafe(OldObject));
        }
    }
#endif
    OnCurrentInteractableChanged.Broadcast(OldObject, NewObject);
    UpdateInteractionPrompt(true);
}

void UJMInteractionComponent::ClearCurrentInteractable() { SetCurrentInteractable(nullptr, nullptr); }

bool UJMInteractionComponent::FindInteractable(FVector Start, FVector Direction, UObject*& OutObject, AActor*& OutActor, FHitResult& OutHit) const
{
    OutObject = nullptr; OutActor = nullptr;
    UWorld* World = GetWorld(); AActor* Owner = GetOwner();
    const FVector SafeDirection = Direction.GetSafeNormal();
    if (!World || !Owner || SafeDirection.IsNearlyZero()) return false;
    const FVector End = Start + SafeDirection * FMath::Max(1.0f, TraceDistance);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(JMInteractionTrace), false, Owner);
    const bool bHit = TraceRadius > KINDA_SMALL_NUMBER
        ? World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), Params)
        : World->LineTraceSingleByChannel(OutHit, Start, End, TraceChannel, Params);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug) DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, TraceInterval + 0.02f, 0, 1.5f);
#endif
    if (!bHit) return false;
    OutObject = ResolveInteractableFromHit(OutHit);
    if (!IsValid(OutObject)) { OutObject = nullptr; return false; }
    OutActor = ActorFromInteractable(OutObject, OutHit.GetActor());
    return true;
}

bool UJMInteractionComponent::FindInteractableUnderCursor(UObject*& OutObject, AActor*& OutActor, FHitResult& OutHit) const
{
    OutObject = nullptr; OutActor = nullptr;
    APlayerController* PC = ResolvePlayerController();
    if (!PC || !PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(TraceChannel), false, OutHit)) return false;
    OutObject = ResolveInteractableFromHit(OutHit);
    OutActor = ActorFromInteractable(OutObject, OutHit.GetActor());
    FVector ViewLocation, ViewDirection;
    ResolveInteractionView(ViewLocation, ViewDirection);
    if (!IsValid(OutObject) || !IsValid(OutActor) || FVector::DistSquared(ViewLocation, OutHit.ImpactPoint) > FMath::Square(TraceDistance))
    {
        OutObject = nullptr; OutActor = nullptr; return false;
    }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug && GetWorld()) DrawDebugLine(GetWorld(), ViewLocation, OutHit.ImpactPoint, FColor::Green, false, TraceInterval + 0.02f, 0, 1.5f);
#endif
    return true;
}

UObject* UJMInteractionComponent::ResolveInteractableFromHit(const FHitResult& Hit) const
{
    TArray<AActor*, TInlineAllocator<4>> Actors;
    if (const UActorComponent* Component = Hit.GetComponent()) Actors.AddUnique(Component->GetOwner());
    if (AActor* Actor = Hit.GetActor())
    {
        Actors.AddUnique(Actor);
        if (Actor->GetOwner()) Actors.AddUnique(Actor->GetOwner());
        if (Actor->GetAttachParentActor()) Actors.AddUnique(Actor->GetAttachParentActor());
    }
    for (AActor* Actor : Actors) if (UObject* Object = FindInteractableOnActor(Actor)) return Object;
    return nullptr;
}

FJMInteractionContext UJMInteractionComponent::BuildInteractionContext(UObject* Object) const
{
    FJMInteractionContext Context;
    AActor* Owner = GetOwner();
    Context.InstigatorActor = Owner;
    Context.TargetActor = ActorFromInteractable(Object, nullptr);
    if (const APawn* Pawn = Cast<APawn>(Owner)) Context.InstigatorController = Pawn->GetController();
    else Context.InstigatorController = Cast<AController>(Owner);
    ResolveInteractionView(Context.InteractionLocation, Context.InteractionDirection);
    if (Owner && Owner->GetClass()->ImplementsInterface(UJMInteractorInterface::StaticClass()))
        Context.InstigatorTags = IJMInteractorInterface::Execute_GetInteractorTags(Owner);
    return Context;
}

bool UJMInteractionComponent::ResolveInteractionView(FVector& OutLocation, FVector& OutDirection) const
{
    AActor* Owner = GetOwner(); if (!IsValid(Owner)) return false;
    if (Owner->GetClass()->ImplementsInterface(UJMInteractorInterface::StaticClass()))
    {
        IJMInteractorInterface::Execute_GetInteractionView(Owner, OutLocation, OutDirection);
        if (!OutDirection.IsNearlyZero()) { OutDirection.Normalize(); return true; }
    }
    if (APlayerController* PC = ResolvePlayerController())
    {
        FRotator Rotation; PC->GetPlayerViewPoint(OutLocation, Rotation); OutDirection = Rotation.Vector(); return true;
    }
    if (const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>())
    {
        OutLocation = Camera->GetComponentLocation(); OutDirection = Camera->GetForwardVector(); return true;
    }
    OutLocation = Owner->GetActorLocation(); OutDirection = Owner->GetActorForwardVector(); return !OutDirection.IsNearlyZero();
}

APlayerController* UJMInteractionComponent::ResolvePlayerController() const
{
    if (const APawn* Pawn = Cast<APawn>(GetOwner())) return Cast<APlayerController>(Pawn->GetController());
    return Cast<APlayerController>(GetOwner());
}

void UJMInteractionComponent::CreatePromptWidget()
{
    if (!bCreatePromptWidget || GetNetMode() == NM_DedicatedServer) return;
    APlayerController* PC = ResolvePlayerController();
    if (!PC || !PC->IsLocalController()) return;
    TSubclassOf<UJMInteractionPromptWidgetBase> WidgetClass = InteractionPromptWidgetClass;
    if (!WidgetClass) WidgetClass = UJMInteractionPromptWidgetBase::StaticClass();
    PromptWidget = CreateWidget<UJMInteractionPromptWidgetBase>(PC, WidgetClass);
    if (PromptWidget)
    {
        PromptWidget->AddToViewport();
        PromptWidget->ApplyPromptStyle(PromptStyle);
        PromptWidget->ClearInteractionPrompt();
    }
}

void UJMInteractionComponent::UpdateInteractionPrompt(bool bForceUpdate)
{
    if (bPromptSuppressed)
    {
        // SetPromptSuppressed already hid the widget. Keep tracing without repeatedly clearing/logging it.
        return;
    }

    const FJMInteractionPrompt Prompt = GetCurrentPrompt();
    if (!bForceUpdate && bHasDisplayedPrompt && ArePromptsEqual(Prompt, LastDisplayedPrompt))
    {
        return;
    }

    LastDisplayedPrompt = Prompt;
    bHasDisplayedPrompt = true;
    OnPromptChanged.Broadcast(Prompt);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug) UE_LOG(LogJMInteractionPrompt, Log, TEXT("Prompt Text Updated: '%s' Visible=%s"), *Prompt.DisplayText.ToString(), Prompt.bVisible ? TEXT("true") : TEXT("false"));
#endif
    if (Prompt.bVisible && !Prompt.DisplayText.IsEmpty()) ShowInteractionPrompt(Prompt);
    else HideInteractionPrompt();
}

void UJMInteractionComponent::ShowInteractionPrompt(const FJMInteractionPrompt& Prompt)
{
    if (PromptWidget) PromptWidget->SetInteractionPrompt(Prompt);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug) UE_LOG(LogJMInteractionPrompt, Log, TEXT("Prompt Shown: '%s'"), *Prompt.DisplayText.ToString());
#endif
}

void UJMInteractionComponent::HideInteractionPrompt()
{
    if (PromptWidget) PromptWidget->ClearInteractionPrompt();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (bDrawDebug) UE_LOG(LogJMInteractionPrompt, Log, TEXT("Prompt Hidden"));
#endif
}

void UJMInteractionComponent::PublishInteractionEvent(FGameplayTag EventTag, UObject* InteractableObject, const FJMInteractionResult& Result, const FJMInteractionContext& Context)
{
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UJMGameplayEventSubsystem* EventSubsystem = GameInstance ? GameInstance->GetSubsystem<UJMGameplayEventSubsystem>() : nullptr;
    if (!EventSubsystem || !EventTag.IsValid()) return;
    UJMInteractionEventPayload* Payload = NewObject<UJMInteractionEventPayload>(this);
    Payload->Result = Result;
    Payload->Context = Context;
    FJMGameplayEventMessage Message;
    Message.EventTag = EventTag;
    Message.Source = this;
    Message.Instigator = GetOwner();
    Message.Target = IsValid(InteractableObject) ? InteractableObject : Context.TargetActor.Get();
    Message.Payload = Payload;
    EventSubsystem->PublishEvent(Message);
}
