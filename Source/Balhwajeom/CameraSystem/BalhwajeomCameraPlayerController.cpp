// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomCameraPlayerController.h"

#include "BalhwajeomCameraCharacter.h"
#include "BalhwajeomEvidenceActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomCameraPlayerController::ABalhwajeomCameraPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = false;

	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultInteractionPromptClass(
		TEXT("/Game/Balhwajeom/UI/HUD/WB_Interact"));
	if (DefaultInteractionPromptClass.Succeeded())
	{
		InteractionPromptWidgetClass = DefaultInteractionPromptClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> DefaultBedMemoryHUDClass(
		TEXT("/Game/Balhwajeom/UI/HUD/WB_HUD2"));
	if (DefaultBedMemoryHUDClass.Succeeded())
	{
		BedMemoryHUDWidgetClass = DefaultBedMemoryHUDClass.Class;
	}
}

void ABalhwajeomCameraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);

	EnsurePlayerHUD();
	EnsureBedMemoryHUD();
	EnsureInteractionPrompt();
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
		PlayerHUDWidget->AddToViewport(0);
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

void ABalhwajeomCameraPlayerController::SetBedMemoryHUDActive(bool bActive)
{
	EnsurePlayerHUD();
	EnsureBedMemoryHUD();
	bBedMemoryHUDActive = bActive;

	// Make both roots available while cross-fading. The fully transparent side
	// is collapsed by ApplyBedMemoryHUDAlpha once the transition finishes.
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
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
		// WB_Interact's authored text is currently named TextBlock_50. Keep this
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
				TEXT("%s: WB_Interact has no fade target named '%s'. The center dot will remain visible."),
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
	EnsureBedMemoryHUD();
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

	UInspectionComponent* FocusedInspection = InteractionComponent->GetFocusedInspection();
	if (!IsValid(FocusedInspection) ||
		InteractionComponent->GetDistanceStateForInspectable(FocusedInspection) !=
			EPlayerInspectionDistanceState::Close)
	{
		return false;
	}

	const ABalhwajeomEvidenceActor* EvidenceActor =
		Cast<ABalhwajeomEvidenceActor>(FocusedInspection->GetOwner());
	return IsValid(EvidenceActor) && EvidenceActor->CanRequestInvestigationInteraction();
}

void ABalhwajeomCameraPlayerController::UpdateInteractionPrompt(float DeltaSeconds)
{
	if (!IsValid(InteractionPromptWidget) || !IsValid(InteractionPromptFadeTarget))
	{
		return;
	}

	const bool bShouldShow = ShouldShowInteractionPrompt();
	if (bShouldShow && InteractionPromptFadeTarget->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		InteractionPromptFadeTarget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const float TargetOpacity = bShouldShow ? 1.0f : 0.0f;
	const float NewOpacity = FMath::FInterpTo(
		InteractionPromptFadeTarget->GetRenderOpacity(),
		TargetOpacity,
		DeltaSeconds,
		InteractionPromptFadeSpeed);
	InteractionPromptFadeTarget->SetRenderOpacity(NewOpacity);

	if (!bShouldShow && NewOpacity <= KINDA_SMALL_NUMBER)
	{
		InteractionPromptFadeTarget->SetRenderOpacity(0.0f);
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
