// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraPlayerController.h"

#include "BalhwajeomCameraCharacter.h"
#include "BalhwajeomEvidenceActor.h"
#include "BalhwajeomPhotoCameraComponent.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Interaction/ItemInspectionIntegration.h"
#include "Components/Widget.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Tablet/BalhwajeomTabletComponent.h"
#include "Tutorial/BalhwajeomTutorialDirector.h"
#include "Tutorial/BalhwajeomTutorialFocusWidget.h"
#include "UI/BalhwajeomKeywordCounterWidget.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomCameraPlayerController::ABalhwajeomCameraPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = false;

	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultInteractionPromptClass(
		TEXT("/Game/Balhwajeom/UI/HUD/WBP_Interact"));
	if (DefaultInteractionPromptClass.Succeeded())
	{
		InteractionPromptWidgetClass = DefaultInteractionPromptClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultInspectionMessageClass(
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_InspectionMessage"));
	if (DefaultInspectionMessageClass.Succeeded())
	{
		InspectionMessageWidgetClass = DefaultInspectionMessageClass.Class;
	}

	// Native by default, so a level needs no Widget Blueprint to get the tutorial layer.
	TutorialFocusWidgetClass = UBalhwajeomTutorialFocusWidget::StaticClass();

	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultBedMemoryHUDClass(
		TEXT("/Game/Balhwajeom/UI/HUD/WBP_HUD2"));
	if (DefaultBedMemoryHUDClass.Succeeded())
	{
		BedMemoryHUDWidgetClass = DefaultBedMemoryHUDClass.Class;
	}
}

void ABalhwajeomCameraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// An intro flow actor can lock presentation before the controller reaches
	// BeginPlay. Do not overwrite its UI-only input mode or hide its cursor.
	if (bGameplayPresentationEnabled)
	{
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
	}

	EnsurePlayerHUD();
	EnsureKeywordCounter();
	EnsureBedMemoryHUD();
	EnsureTutorialFocusLayer();
	EnsureInteractionPrompt();
}

void ABalhwajeomCameraPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundInvestigationSubsystem)
	{
		BoundInvestigationSubsystem->OnWordAcquired.RemoveDynamic(
			this, &ABalhwajeomCameraPlayerController::HandleWordAcquired);
		BoundInvestigationSubsystem->OnPhotoGalleryReset.RemoveDynamic(
			this, &ABalhwajeomCameraPlayerController::HandleInvestigationPhotoGalleryReset);
	}
	BoundInvestigationSubsystem = nullptr;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InspectionMessageTimerHandle);
	}
	if (InspectionMessageWidget)
	{
		InspectionMessageWidget->RemoveFromParent();
	}
	InspectionMessageWidget = nullptr;

	if (KeywordCounterWidget)
	{
		KeywordCounterWidget->RemoveFromParent();
	}
	KeywordCounterWidget = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ABalhwajeomCameraPlayerController::EnsurePlayerHUD()
{
	if (!IsLocalController() || IsValid(PlayerHUDWidget) || !PlayerHUDWidgetClass)
	{
		return;
	}

	PlayerHUDWidget = CreateWidget<UUserWidget>(this, PlayerHUDWidgetClass);
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(
			bGameplayPresentationEnabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		PlayerHUDWidget->AddToViewport(0);
	}
}

void ABalhwajeomCameraPlayerController::EnsureKeywordCounter()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!KeywordCounterWidget)
	{
		KeywordCounterWidget = CreateWidget<UBalhwajeomKeywordCounterWidget>(
			this, UBalhwajeomKeywordCounterWidget::StaticClass());
		if (KeywordCounterWidget)
		{
			KeywordCounterWidget->SetVisibility(
				bGameplayPresentationEnabled && !bBedMemoryHUDActive
					? ESlateVisibility::HitTestInvisible
					: ESlateVisibility::Collapsed);
			KeywordCounterWidget->AddToViewport(2);
		}
	}

	UBalhwajeomInvestigationSubsystem* Investigation = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
		: nullptr;
	if (Investigation && BoundInvestigationSubsystem != Investigation)
	{
		if (BoundInvestigationSubsystem)
		{
			BoundInvestigationSubsystem->OnWordAcquired.RemoveDynamic(
				this, &ABalhwajeomCameraPlayerController::HandleWordAcquired);
			BoundInvestigationSubsystem->OnPhotoGalleryReset.RemoveDynamic(
				this, &ABalhwajeomCameraPlayerController::HandleInvestigationPhotoGalleryReset);
		}
		BoundInvestigationSubsystem = Investigation;
		BoundInvestigationSubsystem->OnWordAcquired.AddUniqueDynamic(
			this, &ABalhwajeomCameraPlayerController::HandleWordAcquired);
		BoundInvestigationSubsystem->OnPhotoGalleryReset.AddUniqueDynamic(
			this, &ABalhwajeomCameraPlayerController::HandleInvestigationPhotoGalleryReset);
		RefreshKeywordCounter();
	}
}

void ABalhwajeomCameraPlayerController::RefreshKeywordCounter()
{
	if (KeywordCounterWidget)
	{
		KeywordCounterWidget->RefreshCount();
	}
}

void ABalhwajeomCameraPlayerController::HandleWordAcquired(
	const FAcquiredWordRecord& WordRecord)
{
	if (WordRecord.SourceType == EWordAcquisitionSource::PhotoCapture)
	{
		// The investigation state changes immediately, but the HUD should not reveal
		// that change before the captured-photo card finishes flying into TAB.
		bPhotoKeywordCountRefreshPending = true;
		return;
	}
	RefreshKeywordCounter();
}

void ABalhwajeomCameraPlayerController::HandleInvestigationPhotoGalleryReset()
{
	// ResetPersistentPhotoGallery() drops every photo-sourced word without going through
	// AcquireWord, so it never fires OnWordAcquired; without this, the counter is left
	// showing whatever total was loaded from the previous session's save.
	bPhotoKeywordCountRefreshPending = false;
	RefreshKeywordCounter();
}

void ABalhwajeomCameraPlayerController::FlushPendingPhotoKeywordCount()
{
	if (!bPhotoKeywordCountRefreshPending)
	{
		return;
	}

	bPhotoKeywordCountRefreshPending = false;
	RefreshKeywordCounter();
}

void ABalhwajeomCameraPlayerController::SetGameplayPresentationEnabled(bool bEnabled)
{
	bGameplayPresentationEnabled = bEnabled;

	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(
			bEnabled && !bBedMemoryHUDActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (KeywordCounterWidget)
	{
		KeywordCounterWidget->SetVisibility(
			bEnabled && !bBedMemoryHUDActive
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (BedMemoryHUDWidget)
	{
		BedMemoryHUDWidget->SetVisibility(
			bEnabled && bBedMemoryHUDActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TutorialFocusWidget)
	{
		TutorialFocusWidget->SetVisibility(
			bEnabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetVisibility(
			bEnabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ABalhwajeomCameraPlayerController::EnsureBedMemoryHUD()
{
	if (!IsLocalController() || IsValid(BedMemoryHUDWidget) || !BedMemoryHUDWidgetClass)
	{
		return;
	}

	BedMemoryHUDWidget = CreateWidget<UUserWidget>(this, BedMemoryHUDWidgetClass);
	if (BedMemoryHUDWidget)
	{
		BedMemoryHUDWidget->SetRenderOpacity(0.0f);
		BedMemoryHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		BedMemoryHUDWidget->AddToViewport(1);
	}
}


void ABalhwajeomCameraPlayerController::EnsureTutorialFocusLayer()
{
	if (!IsLocalController() || IsValid(TutorialFocusWidget) || !TutorialFocusWidgetClass)
	{
		return;
	}

	TutorialFocusWidget = CreateWidget<UUserWidget>(this, TutorialFocusWidgetClass);
	if (TutorialFocusWidget)
	{
		TutorialFocusWidget->SetVisibility(
			bGameplayPresentationEnabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// ZOrder 5 keeps the dim above the HUD icons (0) and the bed HUD (1) but below
		// WBP_Interact (20), so the [F] prompt and centre dot stay readable while dimmed.
		TutorialFocusWidget->AddToViewport(5);
	}
}


float ABalhwajeomCameraPlayerController::GetInteractionPromptAlpha() const
{
	// The un-pulsed fade value. Reading the widget's render opacity instead would make
	// the tutorial dim inherit the prompt's blink.
	return InteractionPromptAlpha;
}


void ABalhwajeomCameraPlayerController::ShowInspectionMessage(const FText& Message)
{
	if (!IsLocalController() || Message.IsEmpty() || !InspectionMessageWidgetClass)
	{
		return;
	}

	if (!InspectionMessageWidget)
	{
		InspectionMessageWidget = CreateWidget<UUserWidget>(this, InspectionMessageWidgetClass);
		if (!InspectionMessageWidget)
		{
			return;
		}
		InspectionMessageWidget->AddToViewport(InspectionMessageViewportZOrder);
	}

	if (UTextBlock* MessageText = Cast<UTextBlock>(
		InspectionMessageWidget->GetWidgetFromName(InspectionMessageTextWidgetName)))
	{
		MessageText->SetText(Message);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: WBP_InspectionMessage has no TextBlock named '%s'."),
			*GetName(),
			*InspectionMessageTextWidgetName.ToString());
	}

	InspectionMessageWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld()))
	{
		FGameViewportWidgetSlot Slot = ViewportSubsystem->GetWidgetSlot(InspectionMessageWidget);
		Slot.ZOrder = InspectionMessageViewportZOrder;
		ViewportSubsystem->SetWidgetSlot(InspectionMessageWidget, Slot);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InspectionMessageTimerHandle,
			this,
			&ThisClass::HideInspectionMessage,
			FMath::Max(0.1f, InspectionMessageDisplayDuration),
			false);
	}
}


void ABalhwajeomCameraPlayerController::HideInspectionMessage()
{
	if (InspectionMessageWidget)
	{
		InspectionMessageWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ABalhwajeomCameraPlayerController::SetBedMemoryHUDActive(bool bActive)
{
	EnsurePlayerHUD();
	EnsureKeywordCounter();
	EnsureBedMemoryHUD();
	bBedMemoryHUDActive = bActive;

	// Make both roots available while cross-fading. The fully transparent side
	// is collapsed by ApplyBedMemoryHUDAlpha once the transition finishes.
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (KeywordCounterWidget)
	{
		KeywordCounterWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (BedMemoryHUDWidget)
	{
		BedMemoryHUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (BedMemoryHUDFadeDuration <= KINDA_SMALL_NUMBER)
	{
		BedMemoryHUDAlpha = bActive ? 1.0f : 0.0f;
		ApplyBedMemoryHUDAlpha(BedMemoryHUDAlpha);
	}
}

void ABalhwajeomCameraPlayerController::ApplyBedMemoryHUDAlpha(float Alpha)
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetRenderOpacity(1.0f - ClampedAlpha);
		if (ClampedAlpha >= 1.0f - KINDA_SMALL_NUMBER)
		{
			PlayerHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (KeywordCounterWidget)
	{
		KeywordCounterWidget->SetRenderOpacity(1.0f - ClampedAlpha);
		if (ClampedAlpha >= 1.0f - KINDA_SMALL_NUMBER)
		{
			KeywordCounterWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (BedMemoryHUDWidget)
	{
		BedMemoryHUDWidget->SetRenderOpacity(ClampedAlpha);
		if (ClampedAlpha <= KINDA_SMALL_NUMBER)
		{
			BedMemoryHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ABalhwajeomCameraPlayerController::UpdateBedMemoryHUD(float DeltaSeconds)
{
	if (!bGameplayPresentationEnabled)
	{
		if (PlayerHUDWidget) PlayerHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (KeywordCounterWidget) KeywordCounterWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (BedMemoryHUDWidget) BedMemoryHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float TargetAlpha = bBedMemoryHUDActive ? 1.0f : 0.0f;
	if (FMath::IsNearlyEqual(BedMemoryHUDAlpha, TargetAlpha))
	{
		BedMemoryHUDAlpha = TargetAlpha;
		ApplyBedMemoryHUDAlpha(BedMemoryHUDAlpha);
		return;
	}

	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (KeywordCounterWidget)
	{
		KeywordCounterWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (BedMemoryHUDWidget)
	{
		BedMemoryHUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const float FadeSpeed = BedMemoryHUDFadeDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: 1.0f / BedMemoryHUDFadeDuration;
	BedMemoryHUDAlpha = FMath::FInterpConstantTo(
		BedMemoryHUDAlpha, TargetAlpha, DeltaSeconds, FadeSpeed);
	ApplyBedMemoryHUDAlpha(BedMemoryHUDAlpha);
}

void ABalhwajeomCameraPlayerController::EnsureInteractionPrompt()
{
	if (!IsLocalController() || IsValid(InteractionPromptWidget) || !InteractionPromptWidgetClass)
	{
		return;
	}

	InteractionPromptWidget = CreateWidget<UUserWidget>(this, InteractionPromptWidgetClass);
	if (InteractionPromptWidget)
	{
		// Keep the root visible so the center dot never disappears. Only the authored
		// interaction text (or a designer-selected container) participates in the fade.
		InteractionPromptWidget->SetRenderOpacity(1.0f);
		InteractionPromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		InteractionPromptFadeTarget =
			InteractionPromptWidget->GetWidgetFromName(InteractionPromptFadeTargetName);
		// Older prompt widgets named their text TextBlock_50. Keep this
		// fallback so an older BP_OrbitViewPlayerController CDO that inherited the
		// previous, incorrect "Text" default still resolves the real text widget.
		if (!InteractionPromptFadeTarget)
		{
			InteractionPromptFadeTarget =
				InteractionPromptWidget->GetWidgetFromName(TEXT("TextBlock_50"));
		}
		if (InteractionPromptFadeTarget)
		{
			InteractionPromptFadeTarget->SetRenderOpacity(0.0f);
			InteractionPromptFadeTarget->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("%s: the interaction prompt widget has no fade target named '%s', ")
				TEXT("so nothing in it fades. Set InteractionPromptFadeTargetName to a widget it has."),
				*GetName(),
				*InteractionPromptFadeTargetName.ToString());
		}
		InteractionPromptWidget->AddToViewport(10);
	}
}

void ABalhwajeomCameraPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Possession/local-player assignment can complete after BeginPlay in travel
	// and test worlds, so keep creation idempotent and retry when needed.
	EnsurePlayerHUD();
	EnsureKeywordCounter();
	EnsureBedMemoryHUD();
	EnsureTutorialFocusLayer();
	EnsureInteractionPrompt();
	UpdateInteractionPrompt(DeltaSeconds);
	UpdateBedMemoryHUD(DeltaSeconds);
}

bool ABalhwajeomCameraPlayerController::ShouldShowInteractionPrompt() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return false;
	}

	const UPlayerInteractionComponent* InteractionComponent =
		ControlledPawn->FindComponentByClass<UPlayerInteractionComponent>();
	if (!IsValid(InteractionComponent))
	{
		return false;
	}

	if (InteractionComponent->HasFocusedItemInspection()) return true;
	UInspectionComponent* FocusedInspection = InteractionComponent->GetFocusedInspection();
	if (!IsValid(FocusedInspection) ||
		InteractionComponent->GetDistanceStateForInspectable(FocusedInspection) !=
			EPlayerInspectionDistanceState::Close)
	{
		return false;
	}

	AActor* FocusedActor = FocusedInspection->GetOwner();
	if (const UDoorInteractionComponent* DoorInteraction =
		IsValid(FocusedActor) ? FocusedActor->FindComponentByClass<UDoorInteractionComponent>() : nullptr)
	{
		return DoorInteraction->CanInteract();
	}

	const ABalhwajeomEvidenceActor* EvidenceActor = Cast<ABalhwajeomEvidenceActor>(FocusedActor);
	return IsValid(EvidenceActor) && EvidenceActor->CanRequestInvestigationInteraction();
}

bool ABalhwajeomCameraPlayerController::IsInteractionPromptSuppressedByTablet() const
{
	const UBalhwajeomTabletComponent* TabletComponent =
		FindComponentByClass<UBalhwajeomTabletComponent>();
	if (!TabletComponent)
	{
		const APawn* ControlledPawn = GetPawn();
		TabletComponent = IsValid(ControlledPawn)
			? ControlledPawn->FindComponentByClass<UBalhwajeomTabletComponent>()
			: nullptr;
	}

	return IsValid(TabletComponent) && TabletComponent->IsTabletOpen();
}

bool ABalhwajeomCameraPlayerController::IsInteractionPromptSuppressedByPhotoCamera() const
{
	const APawn* ControlledPawn = GetPawn();
	const UBalhwajeomPhotoCameraComponent* PhotoCamera = IsValid(ControlledPawn)
		? ControlledPawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>()
		: nullptr;

	return IsValid(PhotoCamera) &&
		(PhotoCamera->IsInCameraMode() || PhotoCamera->IsCameraTransitioning());
}

void ABalhwajeomCameraPlayerController::UpdateInteractionPrompt(float DeltaSeconds)
{
	if (!IsValid(InteractionPromptWidget))
	{
		return;
	}
	if (!bGameplayPresentationEnabled)
	{
		InteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (BalhwajeomItemInspection::IsOpen(this) || IsInteractionPromptSuppressedByTablet() ||
		IsInteractionPromptSuppressedByPhotoCamera())
	{
		InteractionPromptAlpha = 0.0f;
		// Full-screen modes own this layer: hide both the center dot and text.
		// Reset the text so closing the tablet starts a clean fade-in only when the
		// currently focused evidence is still interactable.
		if (IsValid(InteractionPromptFadeTarget))
		{
			InteractionPromptFadeTarget->SetRenderOpacity(0.0f);
			InteractionPromptFadeTarget->SetVisibility(ESlateVisibility::Hidden);
		}
		InteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (InteractionPromptWidget->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		InteractionPromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	const bool bShouldShow = ShouldShowInteractionPrompt();
	const float TargetOpacity = bShouldShow ? 1.0f : 0.0f;
	InteractionPromptAlpha = FMath::FInterpTo(
		InteractionPromptAlpha,
		TargetOpacity,
		DeltaSeconds,
		InteractionPromptFadeSpeed);

	// A tutorial step can ask for the [F] prompt itself to pulse.
	float DisplayOpacity = InteractionPromptAlpha;
	if (ABalhwajeomTutorialDirector::GetTutorialHintTarget(this) ==
		EBalhwajeomTutorialHintTarget::InteractPrompt)
	{
		DisplayOpacity *= ABalhwajeomTutorialDirector::GetTutorialHighlightPulse(this);
	}

	if (!bShouldShow && InteractionPromptAlpha <= KINDA_SMALL_NUMBER)
	{
		InteractionPromptAlpha = 0.0f;
		DisplayOpacity = 0.0f;
	}

	// A prompt widget without a fade target still keeps a correct alpha above, so the
	// tutorial dim works even when the widget is a plain centre dot.
	if (!IsValid(InteractionPromptFadeTarget))
	{
		return;
	}

	if (bShouldShow &&
		InteractionPromptFadeTarget->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		InteractionPromptFadeTarget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	InteractionPromptFadeTarget->SetRenderOpacity(DisplayOpacity);

	if (!bShouldShow && InteractionPromptAlpha <= 0.0f)
	{
		InteractionPromptFadeTarget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABalhwajeomCameraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	check(InputComponent);
	InputComponent->BindAxis(TEXT("Turn"), this, &ABalhwajeomCameraPlayerController::HandleMouseYaw);
}

void ABalhwajeomCameraPlayerController::HandleMouseYaw(float Value)
{
	if (ABalhwajeomCameraCharacter* ControlledCharacter = Cast<ABalhwajeomCameraCharacter>(GetPawn()))
	{
		ControlledCharacter->ApplyMouseYawInput(Value);
	}
}
