#include "UI/BalhwajeomEmergencyEscapePresenter.h"

#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/BalhwajeomGateDoorActor.h"
#include "TimerManager.h"
#include "UI/BalhwajeomConfirmPromptWidget.h"
#include "UI/BalhwajeomEmergencyEscape.h"
#include "UI/BalhwajeomScreenFadeWidget.h"


UBalhwajeomEmergencyEscapePresenter::UBalhwajeomEmergencyEscapePresenter()
{
	PrimaryComponentTick.bCanEverTick = false;
	PromptWidgetClass = UBalhwajeomConfirmPromptWidget::StaticClass();
	ScreenFadeWidgetClass = TSoftClassPtr<UBalhwajeomScreenFadeWidget>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/UI/Title/WBP_ScreenFade.WBP_ScreenFade_C")));
	PromptTitle = NSLOCTEXT("Balhwajeom", "EmergencyEscapeTitle", "긴급 탈출");
	PromptMessage = NSLOCTEXT(
		"Balhwajeom",
		"EmergencyEscapeMessage",
		"잠시 방문 앞으로 이동하시겠습니까?");
}

bool UBalhwajeomEmergencyEscapePresenter::IsEscapeInProgress() const
{
	return bPromptOpen || bTeleportPending || bFadingBack;
}

void UBalhwajeomEmergencyEscapePresenter::RequestEmergencyEscape()
{
	// One escape at a time. The shortcut is a three-key chord, but a held Backspace would
	// still repeat, and reopening the prompt mid-teleport would strand the input mode.
	if (IsEscapeInProgress())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (!PromptWidget)
	{
		TSubclassOf<UBalhwajeomConfirmPromptWidget> WidgetClass = PromptWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UBalhwajeomConfirmPromptWidget::StaticClass();
		}
		PromptWidget = CreateWidget<UBalhwajeomConfirmPromptWidget>(PlayerController, WidgetClass);
		if (!PromptWidget)
		{
			return;
		}
		PromptWidget->OnConfirmed.AddUObject(this, &ThisClass::HandleConfirmed);
		PromptWidget->OnCancelled.AddUObject(this, &ThisClass::HandleCancelled);
		PromptWidget->OnFadeOutFinished.AddUObject(this, &ThisClass::HandlePromptFadeOutFinished);
	}

	if (!PromptWidget->IsInViewport())
	{
		PromptWidget->AddToPlayerScreen(PromptZOrder);
	}

	bPromptOpen = true;
	bTeleportPending = false;
	PromptWidget->Present(PromptTitle, PromptMessage);
	ApplyPromptInputMode(true);
}

void UBalhwajeomEmergencyEscapePresenter::ApplyPromptInputMode(const bool bOpen)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController)
	{
		return;
	}

	if (bOpen)
	{
		bPreviousShowMouseCursor = PlayerController->bShowMouseCursor;

		// Recorded rather than assumed: another system may already be ignoring input, and
		// the escape must not hand movement back to a player who was not meant to have it.
		bChangedMoveInput = !PlayerController->IsMoveInputIgnored();
		bChangedLookInput = !PlayerController->IsLookInputIgnored();
		if (bChangedMoveInput)
		{
			PlayerController->SetIgnoreMoveInput(true);
		}
		if (bChangedLookInput)
		{
			PlayerController->SetIgnoreLookInput(true);
		}

		PlayerController->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		if (PromptWidget)
		{
			InputMode.SetWidgetToFocus(PromptWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		return;
	}

	if (bChangedMoveInput)
	{
		PlayerController->SetIgnoreMoveInput(false);
		bChangedMoveInput = false;
	}
	if (bChangedLookInput)
	{
		PlayerController->SetIgnoreLookInput(false);
		bChangedLookInput = false;
	}
	PlayerController->bShowMouseCursor = bPreviousShowMouseCursor;

	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	PlayerController->SetInputMode(InputMode);
}

void UBalhwajeomEmergencyEscapePresenter::HandleConfirmed()
{
	ClosePrompt(true);
}

void UBalhwajeomEmergencyEscapePresenter::HandleCancelled()
{
	ClosePrompt(false);
}

void UBalhwajeomEmergencyEscapePresenter::ClosePrompt(const bool bTeleport)
{
	bTeleportPending = bTeleport;
	bPromptOpen = false;
	if (PromptWidget)
	{
		PromptWidget->BeginFadeOut();
	}
}

void UBalhwajeomEmergencyEscapePresenter::HandlePromptFadeOutFinished()
{
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
	}

	if (!bTeleportPending)
	{
		// "No" is the whole answer: give the controls straight back.
		ApplyPromptInputMode(false);
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	const TSubclassOf<UBalhwajeomScreenFadeWidget> FadeClass = ScreenFadeWidgetClass.LoadSynchronous();
	if (!PlayerController || !FadeClass)
	{
		// No way to hide the move, so do it plainly rather than not at all.
		PerformTeleport();
		bTeleportPending = false;
		ApplyPromptInputMode(false);
		return;
	}

	if (!ScreenFadeWidget)
	{
		ScreenFadeWidget = CreateWidget<UBalhwajeomScreenFadeWidget>(PlayerController, FadeClass);
		if (!ScreenFadeWidget)
		{
			PerformTeleport();
			bTeleportPending = false;
			ApplyPromptInputMode(false);
			return;
		}
		ScreenFadeWidget->OnFadeToBlackFinished.AddDynamic(this, &ThisClass::HandleFadeToBlackFinished);
		ScreenFadeWidget->OnFadeFromBlackFinished.AddDynamic(this, &ThisClass::HandleFadeFromBlackFinished);
	}

	if (!ScreenFadeWidget->IsInViewport())
	{
		// Above everything, including the tutorial overlay's 2000.
		ScreenFadeWidget->AddToPlayerScreen(9999);
	}
	ScreenFadeWidget->FadeToBlack(FadeToBlackSeconds);
}

void UBalhwajeomEmergencyEscapePresenter::HandleFadeToBlackFinished()
{
	if (!bTeleportPending)
	{
		return;
	}
	bTeleportPending = false;
	PerformTeleport();

	// A beat of solid black on both sides of the move. Fading back on the same frame can
	// catch the first frame at the new spot before the camera boom has settled behind it.
	if (UWorld* World = GetWorld(); World && BlackHoldSeconds > 0.0f)
	{
		bFadingBack = true;
		World->GetTimerManager().SetTimer(
			BlackHoldTimerHandle, this, &ThisClass::BeginFadeBackIn, BlackHoldSeconds, false);
		return;
	}

	BeginFadeBackIn();
}

void UBalhwajeomEmergencyEscapePresenter::BeginFadeBackIn()
{
	bFadingBack = true;
	if (ScreenFadeWidget)
	{
		ScreenFadeWidget->FadeFromBlack(FadeFromBlackSeconds);
		return;
	}

	HandleFadeFromBlackFinished();
}

void UBalhwajeomEmergencyEscapePresenter::HandleFadeFromBlackFinished()
{
	bFadingBack = false;
	if (ScreenFadeWidget)
	{
		ScreenFadeWidget->RemoveFromParent();
	}
	ApplyPromptInputMode(false);
}

bool UBalhwajeomEmergencyEscapePresenter::ResolveEscapeDestination(FTransform& OutTransform) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!World || !Pawn)
	{
		return false;
	}

	TArray<FTransform> DoorTransforms;
	TArray<FVector> DoorLocations;
	for (TActorIterator<ABalhwajeomGateDoorActor> It(World); It; ++It)
	{
		DoorTransforms.Add(It->GetActorTransform());
		DoorLocations.Add(It->GetActorLocation());
	}

	const int32 NearestIndex = BalhwajeomEmergencyEscape::ResolveNearestDoorIndex(
		Pawn->GetActorLocation(), DoorLocations);
	if (NearestIndex != INDEX_NONE)
	{
		OutTransform = BalhwajeomEmergencyEscape::ResolveEscapeTransform(
			DoorTransforms[NearestIndex], EscapeLocalOffset, EscapeLocalYaw);
		return true;
	}

	// No gate door in this level (the intro map, for one). A start point is still better
	// than leaving the player wedged.
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		OutTransform = FTransform(It->GetActorRotation(), It->GetActorLocation());
		return true;
	}

	return false;
}

void UBalhwajeomEmergencyEscapePresenter::PerformTeleport()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	FTransform Destination;
	if (!ResolveEscapeDestination(Destination))
	{
		return;
	}

	// Landing in camera mode would drop the player into a first-person view they never
	// raised, and its rotation flags would fight the teleported facing.
	if (UBalhwajeomPhotoCameraComponent* PhotoCamera =
		Pawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>())
	{
		PhotoCamera->RequestExitCameraMode();
	}

	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			// Carried momentum would shove the player straight back into whatever they were
			// stuck on the moment they arrive.
			Movement->StopMovementImmediately();
			Movement->Velocity = FVector::ZeroVector;
		}
	}

	// TeleportTo rather than SetActorLocation: it resolves encroachment, so the way out of
	// being stuck cannot itself put the player inside the geometry at the far end.
	Pawn->TeleportTo(Destination.GetLocation(), Destination.Rotator());
	PlayerController->SetControlRotation(
		FRotator(0.0f, Destination.Rotator().Yaw, 0.0f));
}

void UBalhwajeomEmergencyEscapePresenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlackHoldTimerHandle);
	}
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
		PromptWidget = nullptr;
	}
	if (ScreenFadeWidget)
	{
		ScreenFadeWidget->RemoveFromParent();
		ScreenFadeWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
