// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraPlayerController.h"

#include "BalhwajeomCameraCharacter.h"
#include "BalhwajeomEvidenceActor.h"
#include "BalhwajeomPhotoCameraComponent.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Interaction/ItemInspectionIntegration.h"
#include "Components/Widget.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Interaction/WorldInteractable.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"
#include "Tablet/BalhwajeomTabletComponent.h"
#include "Tutorial/BalhwajeomTutorialDirector.h"
#include "Tutorial/BalhwajeomTutorialFocusWidget.h"
#include "UI/BalhwajeomKeywordCounterWidget.h"
#include "UI/BalhwajeomInteractionModalWidget.h"
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

	static ConstructorHelpers::FObjectFinder<UTexture2D> CameraIdleTextureFinder(
		TEXT("/Game/Balhwajeom/UI/JE/IMG/Room/cam_button_idle.cam_button_idle"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CameraClickedTextureFinder(
		TEXT("/Game/Balhwajeom/UI/JE/IMG/Room/cam_button_click.cam_button_click"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> TabletIdleTextureFinder(
		TEXT("/Game/Balhwajeom/UI/JE/IMG/Room/tab_button.tab_button"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> TabletClickedTextureFinder(
		TEXT("/Game/Balhwajeom/UI/JE/IMG/Room/tab_button_click.tab_button_click"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> InteractionReticleDotTextureFinder(
		TEXT("/Game/Balhwajeom/UI/HUD/Textures/T_Interact_Dot.T_Interact_Dot"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> InteractionReticleMagnifierTextureFinder(
		TEXT("/Game/Balhwajeom/UI/HUD/Textures/T_Interact_Magnifier.T_Interact_Magnifier"));

	CameraButtonIdleTexture = CameraIdleTextureFinder.Object;
	CameraButtonClickedTexture = CameraClickedTextureFinder.Object;
	TabletButtonIdleTexture = TabletIdleTextureFinder.Object;
	TabletButtonClickedTexture = TabletClickedTextureFinder.Object;
	InteractionReticleDotTexture = InteractionReticleDotTextureFinder.Object;
	InteractionReticleMagnifierTexture = InteractionReticleMagnifierTextureFinder.Object;
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
	BindHudModeEvents();
	RefreshHudModeIcons();
	EnsureKeywordCounter();
	EnsureBedMemoryHUD();
	EnsureTutorialFocusLayer();
	EnsureInteractionPrompt();
}

void ABalhwajeomCameraPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionModalWidget)
	{
		InteractionModalWidget->OnCloseRequested.RemoveAll(this);
		InteractionModalWidget->RemoveFromParent();
		InteractionModalWidget = nullptr;
	}
	if (BoundHudStoryStateSubsystem)
	{
		BoundHudStoryStateSubsystem->OnStateTagAdded.RemoveDynamic(
			this, &ThisClass::HandleHudModeTagChanged);
		BoundHudStoryStateSubsystem->OnStateTagRemoved.RemoveDynamic(
			this, &ThisClass::HandleHudModeTagChanged);
	}
	BoundHudStoryStateSubsystem = nullptr;

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

bool ABalhwajeomCameraPlayerController::ShowInteractionModal(
	TSubclassOf<UUserWidget> ContentWidgetClass,
	const FText& DocumentText,
	const TArray<FText>& NewlyGrantedKeywords)
{
	if (!IsLocalController() || !bGameplayPresentationEnabled ||
		IsInteractionModalOpen() || !ContentWidgetClass)
	{
		return false;
	}

	UBalhwajeomInteractionModalWidget* Modal =
		CreateWidget<UBalhwajeomInteractionModalWidget>(
			this, UBalhwajeomInteractionModalWidget::StaticClass());
	if (!Modal || !Modal->Present(
		ContentWidgetClass, DocumentText, NewlyGrantedKeywords))
	{
		return false;
	}

	InteractionModalWidget = Modal;
	InteractionModalWidget->OnCloseRequested.AddUObject(
		this, &ThisClass::HandleInteractionModalCloseRequested);
	InteractionModalWidget->AddToViewport(1300);

	bInteractionModalChangedMoveIgnore = !IsMoveInputIgnored();
	bInteractionModalChangedLookIgnore = !IsLookInputIgnored();
	bInteractionModalPreviousMouseCursor = bShowMouseCursor;
	if (bInteractionModalChangedMoveIgnore)
	{
		SetIgnoreMoveInput(true);
	}
	if (bInteractionModalChangedLookIgnore)
	{
		SetIgnoreLookInput(true);
	}
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(InteractionModalWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	InteractionModalWidget->SetKeyboardFocus();
	return true;
}

bool ABalhwajeomCameraPlayerController::IsInteractionModalOpen() const
{
	return IsValid(InteractionModalWidget) && InteractionModalWidget->IsInViewport();
}

void ABalhwajeomCameraPlayerController::CloseInteractionModal()
{
	if (!InteractionModalWidget)
	{
		return;
	}

	InteractionModalWidget->OnCloseRequested.RemoveAll(this);
	InteractionModalWidget->RemoveFromParent();
	InteractionModalWidget = nullptr;
	if (bInteractionModalChangedMoveIgnore)
	{
		SetIgnoreMoveInput(false);
	}
	if (bInteractionModalChangedLookIgnore)
	{
		SetIgnoreLookInput(false);
	}
	bShowMouseCursor = bInteractionModalPreviousMouseCursor;
	bInteractionModalChangedMoveIgnore = false;
	bInteractionModalChangedLookIgnore = false;

	if (bGameplayPresentationEnabled)
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		SetInputMode(InputMode);
	}
}

void ABalhwajeomCameraPlayerController::HandleInteractionModalCloseRequested()
{
	CloseInteractionModal();
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
		RefreshHudModeIcons();
	}
}

void ABalhwajeomCameraPlayerController::BindHudModeEvents()
{
	UStoryStateSubsystem* StoryState = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UStoryStateSubsystem>()
		: nullptr;
	if (!StoryState || BoundHudStoryStateSubsystem == StoryState)
	{
		return;
	}

	if (BoundHudStoryStateSubsystem)
	{
		BoundHudStoryStateSubsystem->OnStateTagAdded.RemoveDynamic(
			this, &ThisClass::HandleHudModeTagChanged);
		BoundHudStoryStateSubsystem->OnStateTagRemoved.RemoveDynamic(
			this, &ThisClass::HandleHudModeTagChanged);
	}

	BoundHudStoryStateSubsystem = StoryState;
	BoundHudStoryStateSubsystem->OnStateTagAdded.AddUniqueDynamic(
		this, &ThisClass::HandleHudModeTagChanged);
	BoundHudStoryStateSubsystem->OnStateTagRemoved.AddUniqueDynamic(
		this, &ThisClass::HandleHudModeTagChanged);
}

void ABalhwajeomCameraPlayerController::HandleHudModeTagChanged(FGameplayTag StateTag)
{
	if (StateTag.MatchesTag(BalhwajeomGameplayTags::Runtime_Player_Mode))
	{
		RefreshHudModeIcons();
	}
}

void ABalhwajeomCameraPlayerController::RefreshHudModeIcons()
{
	if (!PlayerHUDWidget)
	{
		return;
	}

	const UStoryStateSubsystem* StoryState = BoundHudStoryStateSubsystem
		? BoundHudStoryStateSubsystem.Get()
		: (GetGameInstance() ? GetGameInstance()->GetSubsystem<UStoryStateSubsystem>() : nullptr);
	const bool bPhotoCameraMode = StoryState && StoryState->HasStateTagExact(
		BalhwajeomGameplayTags::Runtime_Player_Mode_PhotoCamera);
	const bool bTabletMode = StoryState && StoryState->HasStateTagExact(
		BalhwajeomGameplayTags::Runtime_Player_Mode_Tablet);

	if (UImage* CameraImage = Cast<UImage>(
		PlayerHUDWidget->GetWidgetFromName(CameraButtonImageName)))
	{
		UTexture2D* Texture = bPhotoCameraMode
			? CameraButtonClickedTexture.Get()
			: CameraButtonIdleTexture.Get();
		if (Texture)
		{
			// Preserve the size authored in WBP_HUID; only replace its resource.
			CameraImage->SetBrushFromTexture(Texture, false);
		}
	}

	if (UImage* TabletImage = Cast<UImage>(
		PlayerHUDWidget->GetWidgetFromName(TabletButtonImageName)))
	{
		UTexture2D* Texture = bTabletMode
			? TabletButtonClickedTexture.Get()
			: TabletButtonIdleTexture.Get();
		if (Texture)
		{
			TabletImage->SetBrushFromTexture(Texture, false);
		}
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
		InteractionPromptTextWidget = Cast<UTextBlock>(
			InteractionPromptWidget->GetWidgetFromName(InteractionPromptTextWidgetName));
		InteractionReticleWidget = Cast<UImage>(
			InteractionPromptWidget->GetWidgetFromName(InteractionReticleWidgetName));
		// Older prompt widgets named their text TextBlock_50. Keep this
		// fallback so an older BP_OrbitViewPlayerController CDO that inherited the
		// previous, incorrect "Text" default still resolves the real text widget.
		if (!InteractionPromptFadeTarget)
		{
			InteractionPromptFadeTarget =
				InteractionPromptWidget->GetWidgetFromName(TEXT("TextBlock_50"));
		}
		if (!InteractionPromptTextWidget)
		{
			InteractionPromptTextWidget = Cast<UTextBlock>(InteractionPromptFadeTarget);
		}
		// Keep compatibility with the current WBP_Interact asset while allowing the
		// designer-facing widget name to be migrated to InteractionReticle.
		if (!InteractionReticleWidget)
		{
			InteractionReticleWidget = Cast<UImage>(
				InteractionPromptWidget->GetWidgetFromName(TEXT("Image_108")));
		}
		if (InteractionPromptTextWidget)
		{
			DefaultInteractionPromptText = InteractionPromptTextWidget->GetText();
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
		bInteractionReticleStateInitialized = false;
		RefreshInteractionReticle(false);
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

FText ABalhwajeomCameraPlayerController::ResolveInteractionPromptActionText() const
{
	const APawn* ControlledPawn = GetPawn();
	const UPlayerInteractionComponent* InteractionComponent = IsValid(ControlledPawn)
		? ControlledPawn->FindComponentByClass<UPlayerInteractionComponent>()
		: nullptr;
	if (!IsValid(InteractionComponent))
	{
		return FText::GetEmpty();
	}

	UInspectionComponent* FocusedInspection = InteractionComponent->GetFocusedInspection();
	AActor* FocusedActor = IsValid(FocusedInspection) ? FocusedInspection->GetOwner() : nullptr;
	if (!IsValid(FocusedActor))
	{
		return FText::GetEmpty();
	}

	if (const ABalhwajeomEvidenceActor* EvidenceActor = Cast<ABalhwajeomEvidenceActor>(FocusedActor))
	{
		return EvidenceActor->GetInteractionPromptText();
	}
	if (const UDoorInteractionComponent* DoorInteraction =
		FocusedActor->FindComponentByClass<UDoorInteractionComponent>())
	{
		return DoorInteraction->GetInteractionPromptText();
	}
	if (FocusedActor->Implements<UWorldInteractable>())
	{
		return IWorldInteractable::Execute_GetInteractionPromptText(FocusedActor);
	}
	return FText::GetEmpty();
}

void ABalhwajeomCameraPlayerController::RefreshInteractionPromptText(
	bool bHasValidInteractionTarget)
{
	if (!InteractionPromptTextWidget || !bHasValidInteractionTarget)
	{
		// Losing focus starts an opacity fade. Keep the last valid target's text
		// during that fade instead of briefly replacing it with WBP_Interact's
		// generic default while pixels are still visible. The next valid target
		// refreshes the text before its fade-in begins.
		return;
	}

	const FText ActionText = ResolveInteractionPromptActionText();
	FText DesiredText = DefaultInteractionPromptText;
	if (!ActionText.IsEmptyOrWhitespace())
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Action"), ActionText);
		DesiredText = FText::Format(InteractionPromptFormat, Arguments);
	}

	if (!InteractionPromptTextWidget->GetText().EqualTo(DesiredText))
	{
		InteractionPromptTextWidget->SetText(DesiredText);
	}
}

void ABalhwajeomCameraPlayerController::RefreshInteractionReticle(
	bool bHasValidInteractionTarget)
{
	if (!InteractionReticleWidget)
	{
		return;
	}

	UTexture2D* DesiredTexture = bHasValidInteractionTarget
		? InteractionReticleMagnifierTexture.Get()
		: InteractionReticleDotTexture.Get();
	if (!DesiredTexture)
	{
		return;
	}

	if (bInteractionReticleStateInitialized &&
		bInteractionReticleShowsInteractable == bHasValidInteractionTarget &&
		InteractionReticleWidget->GetBrush().GetResourceObject() == DesiredTexture)
	{
		return;
	}

	InteractionReticleWidget->SetBrushFromTexture(DesiredTexture, false);
	bInteractionReticleStateInitialized = true;
	bInteractionReticleShowsInteractable = bHasValidInteractionTarget;
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

	const EBalhwajeomTutorialHintTarget TutorialHintTarget =
		ABalhwajeomTutorialDirector::GetTutorialHintTarget(this);
	const bool bShouldBeBehindTutorialDim =
		TutorialHintTarget == EBalhwajeomTutorialHintTarget::PhotoCameraIcon;
	if (bInteractionPromptBehindTutorialDim != bShouldBeBehindTutorialDim)
	{
		// The tutorial dim is ZOrder 5. Put WB_Interact just below it only while
		// the camera icon owns the player's attention, then restore its normal layer.
		if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld()))
		{
			FGameViewportWidgetSlot Slot = ViewportSubsystem->GetWidgetSlot(InteractionPromptWidget);
			Slot.ZOrder = bShouldBeBehindTutorialDim ? 4 : 10;
			ViewportSubsystem->SetWidgetSlot(InteractionPromptWidget, Slot);
			bInteractionPromptBehindTutorialDim = bShouldBeBehindTutorialDim;
		}
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
	RefreshInteractionReticle(bShouldShow);
	RefreshInteractionPromptText(bShouldShow);
	const float TargetOpacity = bShouldShow ? 1.0f : 0.0f;
	InteractionPromptAlpha = FMath::FInterpTo(
		InteractionPromptAlpha,
		TargetOpacity,
		DeltaSeconds,
		InteractionPromptFadeSpeed);

	// A tutorial step can ask for the [F] prompt itself to pulse.
	float DisplayOpacity = InteractionPromptAlpha;
	if (TutorialHintTarget == EBalhwajeomTutorialHintTarget::InteractPrompt)
	{
		DisplayOpacity *= ABalhwajeomTutorialDirector::GetTutorialHighlightPulse(this);
	}
	else if (bShouldBeBehindTutorialDim)
	{
		// The dim is intentionally translucent, so a bright prompt can still show through
		// even at the lower Z-order. Hide only the authored [F] text pixels while keeping
		// its alpha and the rest of the interaction/input update alive.
		DisplayOpacity = 0.0f;
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

	if (bShouldShow && !bShouldBeBehindTutorialDim &&
		InteractionPromptFadeTarget->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		InteractionPromptFadeTarget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	InteractionPromptFadeTarget->SetRenderOpacity(DisplayOpacity);

	if (bShouldBeBehindTutorialDim || (!bShouldShow && InteractionPromptAlpha <= 0.0f))
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
