#include "Tablet/BalhwajeomTabletComponent.h"

#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Tablet/BalhwajeomTabletWidget.h"

UBalhwajeomTabletComponent::UBalhwajeomTabletComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;

	TabletMappingContextAsset = TSoftObjectPtr<UInputMappingContext>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Core/Input/IMC_Tablet.IMC_Tablet")));
	TabletActionAsset = TSoftObjectPtr<UInputAction>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Core/Input/IA_Tablet.IA_Tablet")));
	TabletWidgetClass = TSoftClassPtr<UBalhwajeomTabletWidget>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/UI/Tablet/WBP_Tablet.WBP_Tablet_C")));
}

void UBalhwajeomTabletComponent::BeginPlay()
{
	Super::BeginPlay();
	TryInitializeForLocalPlayer();
}

void UBalhwajeomTabletComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundPhotoCamera.IsValid())
	{
		BoundPhotoCamera->OnCameraModeExited.RemoveAll(this);
		BoundPhotoCamera->OnCameraTransitionFinished.RemoveAll(this);
	}

	if (bTabletOpen || bPendingOpenAfterPhotoMode)
	{
		bPendingOpenAfterPhotoMode = false;
		FinishCloseTablet();
	}

	if (TabletWidget)
	{
		TabletWidget->OnTabletCloseAnimationFinished.RemoveAll(this);
		TabletWidget->RemoveFromParent();
		TabletWidget = nullptr;
	}

	TeardownPlayerInput();
	Super::EndPlay(EndPlayReason);
}

void UBalhwajeomTabletComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* CurrentController = ResolvePlayerController();
	if (CurrentController != InitializedPlayerController.Get())
	{
		if (bTabletOpen || bPendingOpenAfterPhotoMode)
		{
			bPendingOpenAfterPhotoMode = false;
			FinishCloseTablet();
		}

		TeardownPlayerInput();
		TryInitializeForLocalPlayer();
	}
	else if (!InitializedPlayerController.IsValid())
	{
		TryInitializeForLocalPlayer();
	}

	RefreshPhotoCameraBinding();
	ProcessPendingPhotoExit();
}

bool UBalhwajeomTabletComponent::TryInitializeForLocalPlayer()
{
	APlayerController* PlayerController = ResolvePlayerController();
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return false;
	}

	if (InitializedPlayerController.Get() == PlayerController && TabletInputComponent)
	{
		return true;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return false;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!IsValid(InputSubsystem))
	{
		return false;
	}

	LoadedTabletAction = TabletActionAsset.LoadSynchronous();
	LoadedTabletMappingContext = TabletMappingContextAsset.LoadSynchronous();
	if (!IsValid(LoadedTabletAction) || !IsValid(LoadedTabletMappingContext))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("%s requires IA_Tablet and IMC_Tablet assets."),
			*GetNameSafe(this));
		return false;
	}

	bOwnsMappingContextRegistration = !InputSubsystem->HasMappingContext(LoadedTabletMappingContext);
	if (bOwnsMappingContextRegistration)
	{
		InputSubsystem->AddMappingContext(LoadedTabletMappingContext, TabletMappingPriority);
	}

	TabletInputComponent = NewObject<UEnhancedInputComponent>(
		PlayerController,
		MakeUniqueObjectName(PlayerController, UEnhancedInputComponent::StaticClass(), TEXT("TabletInputComponent")));
	TabletInputComponent->Priority = TabletInputComponentPriority;
	TabletInputComponent->bBlockInput = false;
	TabletInputComponent->RegisterComponent();
	TabletInputComponent->BindAction(
		LoadedTabletAction,
		ETriggerEvent::Started,
		this,
		&UBalhwajeomTabletComponent::HandleTabletAction);
	PlayerController->PushInputComponent(TabletInputComponent);

	InitializedPlayerController = PlayerController;
	RefreshPhotoCameraBinding();
	return true;
}

void UBalhwajeomTabletComponent::TeardownPlayerInput()
{
	APlayerController* PlayerController = InitializedPlayerController.Get();
	if (IsValid(PlayerController))
	{
		if (TabletInputComponent)
		{
			PlayerController->PopInputComponent(TabletInputComponent);
		}

		if (bOwnsMappingContextRegistration && LoadedTabletMappingContext)
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
					LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					InputSubsystem->RemoveMappingContext(LoadedTabletMappingContext);
				}
			}
		}
	}

	if (TabletInputComponent)
	{
		TabletInputComponent->DestroyComponent();
		TabletInputComponent = nullptr;
	}

	bOwnsMappingContextRegistration = false;
	InitializedPlayerController.Reset();
}

APlayerController* UBalhwajeomTabletComponent::ResolvePlayerController() const
{
	if (APlayerController* OwnerController = Cast<APlayerController>(GetOwner()))
	{
		return OwnerController;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return Cast<APlayerController>(OwnerPawn->GetController());
	}

	return nullptr;
}

APawn* UBalhwajeomTabletComponent::ResolvePlayerPawn() const
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return OwnerPawn;
	}

	if (const APlayerController* OwnerController = Cast<APlayerController>(GetOwner()))
	{
		return OwnerController->GetPawn();
	}

	return nullptr;
}

UBalhwajeomPhotoCameraComponent* UBalhwajeomTabletComponent::ResolvePhotoCameraComponent() const
{
	if (APawn* PlayerPawn = ResolvePlayerPawn())
	{
		return PlayerPawn->FindComponentByClass<UBalhwajeomPhotoCameraComponent>();
	}

	return nullptr;
}

void UBalhwajeomTabletComponent::RefreshPhotoCameraBinding()
{
	UBalhwajeomPhotoCameraComponent* CurrentPhotoCamera = ResolvePhotoCameraComponent();
	if (CurrentPhotoCamera == BoundPhotoCamera.Get())
	{
		return;
	}

	if (BoundPhotoCamera.IsValid())
	{
		BoundPhotoCamera->OnCameraModeExited.RemoveAll(this);
		BoundPhotoCamera->OnCameraTransitionFinished.RemoveAll(this);
	}

	BoundPhotoCamera = CurrentPhotoCamera;
	if (CurrentPhotoCamera)
	{
		CurrentPhotoCamera->OnCameraModeExited.AddUObject(
			this,
			&UBalhwajeomTabletComponent::HandlePhotoCameraModeExited);
		CurrentPhotoCamera->OnCameraTransitionFinished.AddUObject(
			this,
			&UBalhwajeomTabletComponent::HandlePhotoCameraTransitionFinished);
	}
}

void UBalhwajeomTabletComponent::ToggleTablet()
{
	if (bPendingOpenAfterPhotoMode)
	{
		return;
	}

	if (bTabletClosing)
	{
		bTabletClosing = false;
		if (TabletWidget)
		{
			TabletWidget->CancelTabletCloseAnimation();
		}
	}
	else if (bTabletOpen)
	{
		CloseTablet();
	}
	else
	{
		RequestOpenTablet();
	}
}

void UBalhwajeomTabletComponent::RequestOpenTablet()
{
	if (bTabletOpen || bPendingOpenAfterPhotoMode)
	{
		return;
	}

	if (!TryInitializeForLocalPlayer())
	{
		return;
	}

	RefreshPhotoCameraBinding();
	if (UBalhwajeomPhotoCameraComponent* PhotoCamera = BoundPhotoCamera.Get())
	{
		if (PhotoCamera->IsInCameraMode() || PhotoCamera->IsCameraTransitioning())
		{
			bPendingOpenAfterPhotoMode = true;
			SetGameplayInputBlocked(true);

			if (!PhotoCamera->IsCameraTransitioning())
			{
				PhotoCamera->RequestExitCameraMode();
			}
			return;
		}
	}

	OpenTabletNow();
}

void UBalhwajeomTabletComponent::ProcessPendingPhotoExit()
{
	if (!bPendingOpenAfterPhotoMode)
	{
		return;
	}

	UBalhwajeomPhotoCameraComponent* PhotoCamera = ResolvePhotoCameraComponent();
	if (!PhotoCamera)
	{
		OpenTabletNow();
		return;
	}

	if (PhotoCamera->IsCameraTransitioning())
	{
		return;
	}

	if (PhotoCamera->IsInCameraMode())
	{
		PhotoCamera->RequestExitCameraMode();
		return;
	}

	OpenTabletNow();
}

void UBalhwajeomTabletComponent::OpenTabletNow()
{
	APlayerController* PlayerController = InitializedPlayerController.Get();
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		bPendingOpenAfterPhotoMode = false;
		SetGameplayInputBlocked(false);
		return;
	}

	if (!TabletWidget)
	{
		TSubclassOf<UBalhwajeomTabletWidget> WidgetClass = TabletWidgetClass.LoadSynchronous();
		if (!WidgetClass)
		{
			WidgetClass = UBalhwajeomTabletWidget::StaticClass();
		}

		TabletWidget = CreateWidget<UBalhwajeomTabletWidget>(PlayerController, WidgetClass);
		if (TabletWidget)
		{
			TabletWidget->SetVisibility(ESlateVisibility::Collapsed);
			TabletWidget->AddToPlayerScreen(100);
			TabletWidget->OnTabletCloseAnimationFinished.AddUObject(
				this,
				&ThisClass::HandleTabletCloseAnimationFinished);
		}
	}

	if (!TabletWidget)
	{
		bPendingOpenAfterPhotoMode = false;
		SetGameplayInputBlocked(false);
		return;
	}

	bSavedShowMouseCursor = PlayerController->bShowMouseCursor;
	bSavedEnableClickEvents = PlayerController->bEnableClickEvents;
	bSavedEnableMouseOverEvents = PlayerController->bEnableMouseOverEvents;

	bPendingOpenAfterPhotoMode = false;
	bTabletOpen = true;
	SetGameplayInputBlocked(true);

	TabletWidget->SetVisibility(ESlateVisibility::Visible);
	TabletWidget->PlayTabletOpenAnimation();

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(TabletWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
}

void UBalhwajeomTabletComponent::CloseTablet()
{
	bPendingOpenAfterPhotoMode = false;
	if (!bTabletOpen || bTabletClosing)
	{
		return;
	}

	bTabletClosing = true;
	if (TabletWidget && TabletWidget->PlayTabletCloseAnimation())
	{
		return;
	}

	bTabletClosing = false;
	FinishCloseTablet();
}

void UBalhwajeomTabletComponent::FinishCloseTablet()
{
	bTabletClosing = false;

	APlayerController* PlayerController = InitializedPlayerController.Get();
	if (TabletWidget)
	{
		TabletWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(PlayerController) && bTabletOpen)
	{
		PlayerController->bShowMouseCursor = bSavedShowMouseCursor;
		PlayerController->bEnableClickEvents = bSavedEnableClickEvents;
		PlayerController->bEnableMouseOverEvents = bSavedEnableMouseOverEvents;

		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		PlayerController->SetInputMode(InputMode);
	}

	bTabletOpen = false;
	SetGameplayInputBlocked(false);
}

void UBalhwajeomTabletComponent::HandleTabletCloseAnimationFinished()
{
	if (bTabletClosing)
	{
		FinishCloseTablet();
	}
}

void UBalhwajeomTabletComponent::SetGameplayInputBlocked(bool bBlocked)
{
	if (TabletInputComponent)
	{
		TabletInputComponent->bBlockInput = bBlocked;
	}
}

void UBalhwajeomTabletComponent::HandleTabletAction()
{
	ToggleTablet();
}

void UBalhwajeomTabletComponent::HandlePhotoCameraModeExited()
{
	// The exit event occurs at the black midpoint. Wait for the full fade-in
	// transition before presenting the tablet.
}

void UBalhwajeomTabletComponent::HandlePhotoCameraTransitionFinished()
{
	ProcessPendingPhotoExit();
}
