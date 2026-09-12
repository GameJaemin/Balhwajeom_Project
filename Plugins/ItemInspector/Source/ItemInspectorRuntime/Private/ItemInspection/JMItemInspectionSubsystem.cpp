#include "ItemInspection/JMItemInspectionSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/JMInteractionComponent.h"
#include "Core/ItemInspectorSettings.h"
#include "Core/JMGameplayLog.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "ItemInspection/JMItemInspectionPreviewActor.h"
#include "ItemInspection/JMItemInspectionTransitionWidget.h"
#include "ItemInspection/JMItemInspectionWidgetBase.h"
#include "JMGameplayUIEventTags.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/JMGameplayEventSubsystem.h"

void UJMItemInspectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJMItemInspectionSubsystem::HandleWorldCleanup);
	SessionHealthTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UJMItemInspectionSubsystem::TickSessionHealth));
}

void UJMItemInspectionSubsystem::Deinitialize()
{
	bDeinitializing = true;
	FTSTicker::GetCoreTicker().RemoveTicker(SessionHealthTickerHandle);
	SessionHealthTickerHandle.Reset();
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
	CloseInspection(EJMItemInspectionCloseReason::WorldTearDown);
	Super::Deinitialize();
}

bool UJMItemInspectionSubsystem::OpenInspection(UJMItemInspectionData* InspectionData)
{
	FJMItemInspectionRequest Request;
	Request.InspectionData = InspectionData;
	if (const UItemInspectorSettings* Settings = UItemInspectorSettings::Get())
	{
		Request.bPauseGame = Settings->bDefaultPauseGame;
		Request.bBlockPlayerInput = Settings->bDefaultBlockPlayerInput;
	}

	return OpenInspectionFromRequest(Request);
}

bool UJMItemInspectionSubsystem::OpenInspectionFromRequest(const FJMItemInspectionRequest& Request)
{
	if (bDeinitializing)
	{
		return false;
	}
	if (bCloseInProgress || State == EJMItemInspectionState::TransitioningOut || State == EJMItemInspectionState::Closing)
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionClosing", "The previous inspection is still closing."));
		return false;
	}
	if (!IsValid(Request.InspectionData))
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionDataMissing", "Inspection data is missing."));
		return false;
	}

	if (IsInspectionOpen())
	{
		const UItemInspectorSettings* Settings = UItemInspectorSettings::Get();
		const EJMItemInspectionDuplicatePolicy DuplicatePolicy = Settings ? Settings->DuplicatePolicy : EJMItemInspectionDuplicatePolicy::RejectNewRequest;
		if (DuplicatePolicy == EJMItemInspectionDuplicatePolicy::RejectNewRequest)
		{
			FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionAlreadyOpen", "An inspection session is already open."));
			return false;
		}

		CloseInspection(EJMItemInspectionCloseReason::Replaced);
		// A close delegate may have opened a different session reentrantly.
		if (State != EJMItemInspectionState::Closed) return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionWorldMissing", "A valid world is required to inspect an item."));
		return false;
	}

	State = EJMItemInspectionState::Opening;
	CurrentInspectionData = Request.InspectionData;
	SessionWorld = World;
	SessionSourceActor = Request.SourceActor;
	bSessionHadSourceActor = IsValid(Request.SourceActor);
	const uint64 SessionId = ++SessionSerial;
	PublishModalPresentation(true);
	if (SessionSerial != SessionId || State != EJMItemInspectionState::Opening || !IsSessionWorldValid())
	{
		return true;
	}
	SuppressInteractionPrompt();

	CurrentTransitionSettings = ResolveTransitionSettings(Request);
	bUseSimpleUITransition = Request.bUseSimpleUITransition || !bSessionHadSourceActor;
	if (Request.bDisableExitTransition)
	{
		CurrentTransitionSettings.bEnableExitTransition = false;
	}
	if (IsValid(Request.SourceActor))
	{
		HiddenSourceActor = Request.SourceActor;
		bSourceActorWasHidden = HiddenSourceActor->IsHidden();
		bShouldHideSourceActor = Request.bHideSourceActor
			|| (CurrentTransitionSettings.bEnableEnterTransition && CurrentTransitionSettings.bHideSourceActorDuringInspection);
	}

	if (World)
	{
		bWasGamePausedBeforeOpen = UGameplayStatics::IsGamePaused(World);
		if (Request.bPauseGame && !bWasGamePausedBeforeOpen)
		{
			bAppliedGamePause = UGameplayStatics::SetGamePaused(World, true);
		}
	}

	if (!CreateInspectionWidget(Request))
	{
		CloseInspection(EJMItemInspectionCloseReason::Failed);
		return false;
	}

	const bool bPreviewReady = CreatePreviewResources(Request);
	if (!bUseSimpleUITransition && bPreviewReady && TryStartEnterTransition(Request))
	{
		return true;
	}
	if (bUseSimpleUITransition && TryStartSimpleEnterTransition())
	{
		return true;
	}

	HideSourceActorIfNeeded();
	State = EJMItemInspectionState::Inspecting;
	OnInspectionOpened.Broadcast(CurrentInspectionData);
	if (SessionSerial == SessionId
		&& State == EJMItemInspectionState::Inspecting
		&& CurrentInspectionData == Request.InspectionData
		&& IsValid(CurrentWidget)
		&& IsSessionWorldValid())
	{
		CurrentWidget->OnInspectionOpened(CurrentInspectionData);
	}

	return true;
}

void UJMItemInspectionSubsystem::CloseInspection(EJMItemInspectionCloseReason Reason)
{
	if (State == EJMItemInspectionState::Closed)
	{
		return;
	}

	if (State == EJMItemInspectionState::TransitioningOut)
	{
		if (Reason == EJMItemInspectionCloseReason::WorldTearDown
			|| Reason == EJMItemInspectionCloseReason::SourceInvalid
			|| Reason == EJMItemInspectionCloseReason::Failed
			|| Reason == EJMItemInspectionCloseReason::Replaced)
		{
			State = EJMItemInspectionState::Closing;
			FinalizeCloseInspection(Reason);
		}
		return;
	}

	const bool bAllowAnimatedClose = State == EJMItemInspectionState::Inspecting
		&& (Reason == EJMItemInspectionCloseReason::User
			|| Reason == EJMItemInspectionCloseReason::CloseButton
			|| Reason == EJMItemInspectionCloseReason::ExternalRequest);
	if (bAllowAnimatedClose && TryStartExitTransition(Reason))
	{
		return;
	}

	State = EJMItemInspectionState::Closing;
	FinalizeCloseInspection(Reason);
}

void UJMItemInspectionSubsystem::FinalizeCloseInspection(EJMItemInspectionCloseReason Reason)
{
	if (State == EJMItemInspectionState::Closed || bCloseInProgress)
	{
		return;
	}
	bCloseInProgress = true;
	++SessionSerial;
	CancelEnterTransition();
	CancelExitTransition();

	if (IsValid(CurrentWidget))
	{
		CurrentWidget->OnCloseRequested.RemoveDynamic(this, &UJMItemInspectionSubsystem::HandleWidgetCloseRequested);
		CurrentWidget->OnPreviewDragged.RemoveDynamic(this, &UJMItemInspectionSubsystem::HandlePreviewDragged);
		CurrentWidget->OnPreviewZoomed.RemoveDynamic(this, &UJMItemInspectionSubsystem::HandlePreviewZoomed);
		CurrentWidget->OnInspectionClosed(Reason);
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	DestroyPreviewResources();

	if (ULocalPlayer* LocalPlayer = bAppliedCursorState ? GetLocalPlayer() : nullptr)
	{
		if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(SessionWorld.Get()))
		{
			if (bPreviousMouseCursor)
			{
				FInputModeGameAndUI InputMode;
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PlayerController->SetInputMode(InputMode);
			}
			else
			{
				FInputModeGameOnly InputMode;
				PlayerController->SetInputMode(InputMode);
			}

			PlayerController->bShowMouseCursor = bPreviousMouseCursor;
		}
	}

	RestorePlayerInputBlock();
	bAppliedCursorState = false;
	RestoreInteractionPrompt();
	RestoreSourceActor();
	RestorePauseState();
	PublishModalPresentation(false);

	CurrentInspectionData = nullptr;
	bUseSimpleUITransition = false;
	SessionWorld.Reset();
	SessionSourceActor.Reset();
	bSessionHadSourceActor = false;
	State = EJMItemInspectionState::Closed;
	bCloseInProgress = false;
	OnInspectionClosed.Broadcast(Reason);
}

bool UJMItemInspectionSubsystem::IsInspectionOpen() const
{
	return State == EJMItemInspectionState::Opening
		|| State == EJMItemInspectionState::TransitioningIn
		|| State == EJMItemInspectionState::Inspecting
		|| State == EJMItemInspectionState::TransitioningOut
		|| State == EJMItemInspectionState::Loading;
}

void UJMItemInspectionSubsystem::PublishModalPresentation(bool bIsOpen)
{
	if (bIsOpen == bModalPresentationPublished)
	{
		return;
	}

	bModalPresentationPublished = bIsOpen;
	UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	UJMGameplayEventSubsystem* EventSubsystem = GameInstance
		? GameInstance->GetSubsystem<UJMGameplayEventSubsystem>()
		: nullptr;
	if (!EventSubsystem)
	{
		return;
	}

	FJMGameplayEventMessage Message;
	Message.EventTag = bIsOpen
		? JMGameplayUIEventTags::ModalOpened
		: JMGameplayUIEventTags::ModalClosed;
	Message.Source = this;
	Message.Target = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	EventSubsystem->PublishEvent(Message);
}

UJMItemInspectionData* UJMItemInspectionSubsystem::GetCurrentInspectionData() const
{
	return CurrentInspectionData;
}

EJMItemInspectionState UJMItemInspectionSubsystem::GetInspectionState() const
{
	return State;
}

void UJMItemInspectionSubsystem::ResetPreviewRotation()
{
	if (CurrentPreviewActor)
	{
		CurrentPreviewActor->ResetPreviewRotation();
	}
}

bool UJMItemInspectionSubsystem::CreateInspectionWidget(const FJMItemInspectionRequest& Request)
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionLocalPlayerMissing", "Local player is missing."));
		return false;
	}

	APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
	if (!PlayerController)
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionPlayerControllerMissing", "Player controller is missing."));
		return false;
	}

	TSubclassOf<UJMItemInspectionWidgetBase> WidgetClass = UJMItemInspectionWidgetBase::StaticClass();
	if (const UItemInspectorSettings* Settings = UItemInspectorSettings::Get())
	{
		if (!Settings->DefaultInspectorWidgetClass.IsNull())
		{
			if (TSubclassOf<UJMItemInspectionWidgetBase> LoadedWidgetClass = Settings->DefaultInspectorWidgetClass.LoadSynchronous())
			{
				WidgetClass = LoadedWidgetClass;
			}
			else
			{
				UE_LOG(LogJMGameplay, Warning, TEXT("Configured inspector widget class could not be loaded. Falling back to UJMItemInspectionWidgetBase."));
			}
		}
	}

	if (!WidgetClass)
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionWidgetMissing", "Inspector widget class is missing."));
		return false;
	}

	CurrentWidget = CreateWidget<UJMItemInspectionWidgetBase>(PlayerController, WidgetClass);
	if (!CurrentWidget)
	{
		FailOpen(Request, NSLOCTEXT("JMGameplay", "InspectionWidgetCreateFailed", "Failed to create the inspector widget."));
		return false;
	}

	bPreviousMouseCursor = PlayerController->bShowMouseCursor;
	bAppliedCursorState = true;
	PlayerController->bShowMouseCursor = true;
	ApplyPlayerInputBlock(PlayerController, Request.bBlockPlayerInput);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(CurrentWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	CurrentWidget->OnCloseRequested.AddDynamic(this, &UJMItemInspectionSubsystem::HandleWidgetCloseRequested);
	CurrentWidget->AddToViewport(100);
	CurrentWidget->SetInspectionData(Request.InspectionData);
	CurrentWidget->SetPreviewTexture(nullptr);
	CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewLoadingStatus", "Static Mesh 미리보기를 준비하는 중입니다."));
	CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewLoadingStatusReadable", "Preparing static mesh preview."));
	CurrentWidget->OnPreviewDragged.AddDynamic(this, &UJMItemInspectionSubsystem::HandlePreviewDragged);
	CurrentWidget->OnPreviewZoomed.AddDynamic(this, &UJMItemInspectionSubsystem::HandlePreviewZoomed);
	CurrentWidget->SetKeyboardFocus();

	return true;
}

bool UJMItemInspectionSubsystem::CreatePreviewResources(const FJMItemInspectionRequest& Request)
{
	if (!CurrentWidget || !IsValid(Request.InspectionData))
	{
		return false;
	}

	if (Request.InspectionData->PreviewMesh.IsNull())
	{
		CurrentWidget->SetPreviewTexture(nullptr);
		CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewMeshNotAssigned", "Preview Mesh가 지정되지 않았습니다."));
		return false;
	}

	UStaticMesh* PreviewMesh = Request.InspectionData->PreviewMesh.LoadSynchronous();
	if (!IsValid(PreviewMesh))
	{
		CurrentWidget->SetPreviewTexture(nullptr);
		CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewMeshLoadFailed", "Preview Mesh를 로드하지 못했습니다."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		CurrentWidget->SetPreviewTexture(nullptr);
		CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewWorldInvalid", "월드가 종료 중이라 미리보기를 만들 수 없습니다."));
		return false;
	}

	const UItemInspectorSettings* Settings = UItemInspectorSettings::Get();
	const FIntPoint RenderTargetSize = Settings
		? FIntPoint(FMath::Max(Settings->DefaultRenderTargetSize.X, 64), FMath::Max(Settings->DefaultRenderTargetSize.Y, 64))
		: FIntPoint(1024, 1024);

	CurrentPreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("JMItemInspectionPreviewRenderTarget"));
	if (!CurrentPreviewRenderTarget)
	{
		CurrentWidget->SetPreviewTexture(nullptr);
		CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewRenderTargetFailed", "Render Target을 만들지 못했습니다."));
		return false;
	}

	CurrentPreviewRenderTarget->RenderTargetFormat = RTF_RGBA16f;
	CurrentPreviewRenderTarget->ClearColor = Settings ? Settings->DefaultBackgroundColor : FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	CurrentPreviewRenderTarget->InitAutoFormat(RenderTargetSize.X, RenderTargetSize.Y);
	CurrentPreviewRenderTarget->UpdateResourceImmediate(true);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;

	const FVector PreviewLocation(0.0f, 0.0f, -50000.0f);
	CurrentPreviewActor = World->SpawnActor<AJMItemInspectionPreviewActor>(AJMItemInspectionPreviewActor::StaticClass(), PreviewLocation, FRotator::ZeroRotator, SpawnParameters);
	if (!CurrentPreviewActor)
	{
		CurrentWidget->SetPreviewTexture(nullptr);
		CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewActorFailed", "Preview Actor를 만들지 못했습니다."));
		CurrentPreviewRenderTarget = nullptr;
		return false;
	}

	const float PreviewFOV = Settings ? Settings->DefaultPreviewFOV : 45.0f;
	const float ZoomSpeed = Settings ? Settings->DefaultZoomSpeed : 0.1f;
	if (!CurrentPreviewActor->ConfigurePreview(Request.InspectionData, PreviewMesh, CurrentPreviewRenderTarget, PreviewFOV, ZoomSpeed))
	{
		CurrentWidget->SetPreviewTexture(nullptr);
		CurrentWidget->SetPreviewStatusText(NSLOCTEXT("JMGameplay", "PreviewConfigureFailed", "Static Mesh 미리보기를 설정하지 못했습니다."));
		DestroyPreviewResources();
		return false;
	}

	CurrentWidget->SetPreviewTexture(CurrentPreviewRenderTarget);
	CurrentWidget->SetPreviewStatusText(FText::GetEmpty());
	return true;
}

bool UJMItemInspectionSubsystem::TryStartEnterTransition(const FJMItemInspectionRequest& Request)
{
	if (!CurrentTransitionSettings.bEnableEnterTransition || !CurrentWidget || !CurrentPreviewActor || !CurrentPreviewRenderTarget)
	{
		UE_LOG(LogJMGameplay, Verbose, TEXT("Item inspection entrance transition skipped because it is disabled or preview resources are unavailable."));
		return false;
	}

	if (!BuildTransitionSource(Request, CurrentTransitionSource))
	{
		UE_LOG(LogJMGameplay, Display, TEXT("Item inspection entrance transition skipped because the source actor could not be projected to the viewport."));
		return false;
	}

	CurrentWidget->ForceLayoutPrepass();
	bTransitionTargetResolved = CurrentWidget->GetPreviewViewportRect(TransitionTargetCenter, TransitionTargetSize);
	TransitionLayoutWaitTime = 0.0f;

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	if (!PlayerController)
	{
		return false;
	}

	CurrentTransitionWidget = CreateWidget<UJMItemInspectionTransitionWidget>(
		PlayerController,
		UJMItemInspectionTransitionWidget::StaticClass());
	if (!CurrentTransitionWidget)
	{
		return false;
	}

	CurrentTransitionWidget->AddToViewport(101);
	CurrentTransitionWidget->SetTransitionTexture(CurrentPreviewRenderTarget);
	CurrentTransitionWidget->SetTransitionRect(CurrentTransitionSource.ScreenCenter, CurrentTransitionSource.ScreenSize);
	CurrentTransitionWidget->SetTransitionOpacity(1.0f);

	CurrentPreviewActor->BeginEnterTransition(CurrentTransitionSource);
	CurrentWidget->BeginEnterTransition(CurrentTransitionSettings.InspectorFadeStartAlpha);
	TransitionElapsedTime = 0.0f;
	State = EJMItemInspectionState::TransitioningIn;
	TransitionTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UJMItemInspectionSubsystem::TickEnterTransition));
	UE_LOG(
		LogJMGameplay,
		Display,
		TEXT("Item inspection entrance transition started for '%s'%s."),
		*GetNameSafe(Request.SourceActor),
		bTransitionTargetResolved ? TEXT("") : TEXT("; waiting for the inspector layout"));
	return true;
}

bool UJMItemInspectionSubsystem::TryStartSimpleEnterTransition()
{
	if (!CurrentWidget)
	{
		return false;
	}

	CurrentWidget->BeginSimpleUIEnterTransition();
	TransitionElapsedTime = 0.0f;
	State = EJMItemInspectionState::TransitioningIn;
	TransitionTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UJMItemInspectionSubsystem::TickEnterTransition));
	return true;
}

bool UJMItemInspectionSubsystem::BuildTransitionSource(
	const FJMItemInspectionRequest& Request,
	FJMItemInspectionTransitionSource& OutSource) const
{
	OutSource = FJMItemInspectionTransitionSource();
	if (!IsValid(Request.SourceActor))
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	if (!PlayerController)
	{
		return false;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Request.SourceActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	UPrimitiveComponent* PreferredComponent = nullptr;
	const UStaticMesh* PreviewMesh = IsValid(Request.InspectionData) ? Request.InspectionData->PreviewMesh.Get() : nullptr;
	FBox CombinedBox(ForceInit);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent) || !PrimitiveComponent->IsRegistered() || !PrimitiveComponent->IsVisible())
		{
			continue;
		}

		CombinedBox += PrimitiveComponent->Bounds.GetBox();
		if (!PreferredComponent)
		{
			PreferredComponent = PrimitiveComponent;
		}
		if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent))
		{
			if (PreviewMesh && StaticMeshComponent->GetStaticMesh() == PreviewMesh)
			{
				PreferredComponent = PrimitiveComponent;
			}
		}
	}

	if (!CombinedBox.IsValid)
	{
		return false;
	}

	FVector2D ScreenMin(FLT_MAX, FLT_MAX);
	FVector2D ScreenMax(-FLT_MAX, -FLT_MAX);
	const FVector BoxMin = CombinedBox.Min;
	const FVector BoxMax = CombinedBox.Max;
	int32 ProjectedCornerCount = 0;
	for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
	{
		const FVector Corner(
			(CornerIndex & 1) ? BoxMax.X : BoxMin.X,
			(CornerIndex & 2) ? BoxMax.Y : BoxMin.Y,
			(CornerIndex & 4) ? BoxMax.Z : BoxMin.Z);
		FVector2D ProjectedPosition;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, Corner, ProjectedPosition, false))
		{
			continue;
		}

		++ProjectedCornerCount;
		ScreenMin.X = FMath::Min(ScreenMin.X, ProjectedPosition.X);
		ScreenMin.Y = FMath::Min(ScreenMin.Y, ProjectedPosition.Y);
		ScreenMax.X = FMath::Max(ScreenMax.X, ProjectedPosition.X);
		ScreenMax.Y = FMath::Max(ScreenMax.Y, ProjectedPosition.Y);
	}
	if (ProjectedCornerCount == 0)
	{
		return false;
	}

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PlayerController)
		/ FMath::Max(UWidgetLayoutLibrary::GetViewportScale(PlayerController), 0.01f);
	const bool bIntersectsViewport = ScreenMax.X >= 0.0f && ScreenMax.Y >= 0.0f
		&& ScreenMin.X <= ViewportSize.X && ScreenMin.Y <= ViewportSize.Y;
	if (!bIntersectsViewport)
	{
		return false;
	}

	OutSource.WorldBounds = FBoxSphereBounds(CombinedBox);
	OutSource.ScreenCenter = (ScreenMin + ScreenMax) * 0.5f;
	OutSource.ScreenSize = ScreenMax - ScreenMin;
	OutSource.ScreenSize.X = FMath::Clamp(OutSource.ScreenSize.X, 16.0f, FMath::Max(ViewportSize.X * 1.5f, 16.0f));
	OutSource.ScreenSize.Y = FMath::Clamp(OutSource.ScreenSize.Y, 16.0f, FMath::Max(ViewportSize.Y * 1.5f, 16.0f));

	const FQuat SourceRotation = PreferredComponent
		? PreferredComponent->GetComponentQuat()
		: Request.SourceActor->GetActorQuat();
	const FQuat CameraRotation = PlayerController->PlayerCameraManager
		? PlayerController->PlayerCameraManager->GetCameraRotation().Quaternion()
		: PlayerController->GetControlRotation().Quaternion();
	OutSource.PreviewRelativeRotation = (CameraRotation.Inverse() * SourceRotation).GetNormalized();
	OutSource.bIsOnScreen = true;
	OutSource.bIsValid = true;
	return true;
}

FJMItemInspectionTransitionSettings UJMItemInspectionSubsystem::ResolveTransitionSettings(const FJMItemInspectionRequest& Request) const
{
	if (Request.bOverrideTransitionSettings)
	{
		return Request.TransitionSettings;
	}

	if (IsValid(Request.InspectionData) && Request.InspectionData->bOverrideTransitionSettings)
	{
		return Request.InspectionData->TransitionSettings;
	}

	if (const UItemInspectorSettings* Settings = UItemInspectorSettings::Get())
	{
		return Settings->DefaultTransitionSettings;
	}

	return FJMItemInspectionTransitionSettings();
}

bool UJMItemInspectionSubsystem::TickEnterTransition(float DeltaTime)
{
	if (!IsSessionWorldValid())
	{
		State = EJMItemInspectionState::Closing;
		FinalizeCloseInspection(EJMItemInspectionCloseReason::WorldTearDown);
		return false;
	}
	if (State != EJMItemInspectionState::TransitioningIn || !IsValid(CurrentWidget))
	{
		CancelEnterTransition();
		return false;
	}

	if (bUseSimpleUITransition)
	{
		TransitionElapsedTime += FMath::Max(DeltaTime, 0.0f);
		const float Duration = FMath::Max(CurrentTransitionSettings.EnterDuration, 0.01f);
		const float RawAlpha = FMath::Clamp(TransitionElapsedTime / Duration, 0.0f, 1.0f);
		CurrentWidget->SetSimpleUIEnterTransitionProgress(RawAlpha);
		if (RawAlpha >= 1.0f)
		{
			CompleteEnterTransition();
			return false;
		}
		return true;
	}

	if (!IsValid(CurrentPreviewActor)
		|| CurrentPreviewActor->GetWorld() != SessionWorld.Get()
		|| !IsValid(CurrentTransitionWidget))
	{
		CancelEnterTransition();
		return false;
	}

	if (!bTransitionTargetResolved)
	{
		CurrentWidget->ForceLayoutPrepass();
		bTransitionTargetResolved = CurrentWidget->GetPreviewViewportRect(TransitionTargetCenter, TransitionTargetSize);
		if (!bTransitionTargetResolved)
		{
			TransitionLayoutWaitTime += FMath::Max(DeltaTime, 0.0f);
			if (TransitionLayoutWaitTime < 0.25f)
			{
				return true;
			}

			ULocalPlayer* LocalPlayer = GetLocalPlayer();
			APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
			const FVector2D ViewportSize = PlayerController
				? UWidgetLayoutLibrary::GetViewportSize(PlayerController) / FMath::Max(UWidgetLayoutLibrary::GetViewportScale(PlayerController), 0.01f)
				: FVector2D(1920.0f, 1080.0f);
			TransitionTargetCenter = ViewportSize * 0.5f;
			TransitionTargetSize = ViewportSize * 0.55f;
			bTransitionTargetResolved = true;
			UE_LOG(LogJMGameplay, Warning, TEXT("Inspector preview layout was unavailable; entrance transition is using a centered viewport fallback."));
		}
		else
		{
			UE_LOG(LogJMGameplay, Verbose, TEXT("Inspector preview layout resolved after the widget was added to the viewport."));
		}
	}

	TransitionElapsedTime += FMath::Max(DeltaTime, 0.0f);
	const float Duration = FMath::Max(CurrentTransitionSettings.EnterDuration, 0.01f);
	const float RawAlpha = FMath::Clamp(TransitionElapsedTime / Duration, 0.0f, 1.0f);
	const float EasedAlpha = ApplyTransitionEasing(RawAlpha, CurrentTransitionSettings.Easing);

	CurrentTransitionWidget->SetTransitionRect(
		FMath::Lerp(CurrentTransitionSource.ScreenCenter, TransitionTargetCenter, EasedAlpha),
		FMath::Lerp(CurrentTransitionSource.ScreenSize, TransitionTargetSize, EasedAlpha));
	CurrentTransitionWidget->SetTransitionOpacity(1.0f - FMath::GetMappedRangeValueClamped(FVector2D(0.92f, 1.0f), FVector2D(0.0f, 1.0f), RawAlpha));
	CurrentPreviewActor->UpdateEnterTransition(EasedAlpha);
	CurrentWidget->SetEnterTransitionProgress(RawAlpha);

	if (TransitionElapsedTime >= CurrentTransitionSettings.SourceHideDelay)
	{
		HideSourceActorIfNeeded();
	}

	if (RawAlpha >= 1.0f)
	{
		CompleteEnterTransition();
		return false;
	}

	return true;
}

void UJMItemInspectionSubsystem::CompleteEnterTransition()
{
	if (State != EJMItemInspectionState::TransitioningIn)
	{
		return;
	}

	TransitionTickerHandle.Reset();
	HideSourceActorIfNeeded();
	if (IsValid(CurrentPreviewActor) && CurrentPreviewActor->GetWorld() == SessionWorld.Get())
	{
		CurrentPreviewActor->CompleteEnterTransition();
	}
	if (CurrentWidget)
	{
		if (bUseSimpleUITransition)
		{
			CurrentWidget->CompleteSimpleUIEnterTransition();
		}
		else
		{
			CurrentWidget->CompleteEnterTransition();
		}
	}
	if (CurrentTransitionWidget)
	{
		CurrentTransitionWidget->RemoveFromParent();
		CurrentTransitionWidget = nullptr;
	}

	State = EJMItemInspectionState::Inspecting;
	const uint64 SessionId = SessionSerial;
	UJMItemInspectionData* InspectionData = CurrentInspectionData;
	OnInspectionOpened.Broadcast(CurrentInspectionData);
	if (SessionSerial == SessionId
		&& State == EJMItemInspectionState::Inspecting
		&& CurrentInspectionData == InspectionData
		&& IsValid(CurrentWidget)
		&& IsSessionWorldValid())
	{
		CurrentWidget->OnInspectionOpened(CurrentInspectionData);
	}
}

void UJMItemInspectionSubsystem::CancelEnterTransition()
{
	if (TransitionTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TransitionTickerHandle);
		TransitionTickerHandle.Reset();
	}
	if (CurrentTransitionWidget)
	{
		CurrentTransitionWidget->RemoveFromParent();
		CurrentTransitionWidget = nullptr;
	}
	if (CurrentPreviewActor)
	{
		CurrentPreviewActor->CancelTransition();
	}
	if (CurrentWidget && State != EJMItemInspectionState::Closing)
	{
		CurrentWidget->CompleteEnterTransition();
	}
	TransitionElapsedTime = 0.0f;
	TransitionLayoutWaitTime = 0.0f;
	bTransitionTargetResolved = false;
}

bool UJMItemInspectionSubsystem::TryStartExitTransition(EJMItemInspectionCloseReason Reason)
{
	if (bUseSimpleUITransition && CurrentWidget)
	{
		CurrentWidget->BeginSimpleUIExitTransition();
		PendingCloseReason = Reason;
		TransitionElapsedTime = 0.0f;
		State = EJMItemInspectionState::TransitioningOut;
		TransitionTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UJMItemInspectionSubsystem::TickExitTransition));
		return true;
	}

	if (!CurrentTransitionSettings.bEnableExitTransition
		|| !CurrentWidget
		|| !CurrentPreviewActor
		|| !CurrentPreviewRenderTarget
		|| !IsValid(HiddenSourceActor)
		|| !bDidHideSourceActor
		|| bSourceActorWasHidden)
	{
		return false;
	}

	FJMItemInspectionRequest SourceRequest;
	SourceRequest.InspectionData = CurrentInspectionData;
	SourceRequest.SourceActor = HiddenSourceActor;
	if (!BuildTransitionSource(SourceRequest, CurrentTransitionSource))
	{
		UE_LOG(LogJMGameplay, Display, TEXT("Item inspection exit transition skipped because the source actor is no longer projectable."));
		return false;
	}

	CurrentWidget->ForceLayoutPrepass();
	if (!CurrentWidget->GetPreviewViewportRect(TransitionTargetCenter, TransitionTargetSize))
	{
		UE_LOG(LogJMGameplay, Display, TEXT("Item inspection exit transition skipped because the preview panel layout is unavailable."));
		return false;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	if (!PlayerController)
	{
		return false;
	}

	CurrentTransitionWidget = CreateWidget<UJMItemInspectionTransitionWidget>(
		PlayerController,
		UJMItemInspectionTransitionWidget::StaticClass());
	if (!CurrentTransitionWidget)
	{
		return false;
	}

	CurrentTransitionWidget->AddToViewport(101);
	CurrentTransitionWidget->SetTransitionTexture(CurrentPreviewRenderTarget);
	CurrentTransitionWidget->SetTransitionRect(TransitionTargetCenter, TransitionTargetSize);
	CurrentTransitionWidget->SetTransitionOpacity(1.0f);
	CurrentPreviewActor->BeginExitTransition(CurrentTransitionSource);
	CurrentWidget->BeginExitTransition();

	PendingCloseReason = Reason;
	TransitionElapsedTime = 0.0f;
	State = EJMItemInspectionState::TransitioningOut;
	TransitionTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UJMItemInspectionSubsystem::TickExitTransition));
	UE_LOG(LogJMGameplay, Display, TEXT("Item inspection exit transition started for '%s'."), *GetNameSafe(HiddenSourceActor));
	return true;
}

bool UJMItemInspectionSubsystem::TickExitTransition(float DeltaTime)
{
	if (!IsSessionWorldValid())
	{
		TransitionTickerHandle.Reset();
		State = EJMItemInspectionState::Closing;
		FinalizeCloseInspection(EJMItemInspectionCloseReason::WorldTearDown);
		return false;
	}
	if (State != EJMItemInspectionState::TransitioningOut || !IsValid(CurrentWidget))
	{
		TransitionTickerHandle.Reset();
		State = EJMItemInspectionState::Closing;
		FinalizeCloseInspection(PendingCloseReason);
		return false;
	}

	if (bUseSimpleUITransition)
	{
		TransitionElapsedTime += FMath::Max(DeltaTime, 0.0f);
		const float Duration = FMath::Max(CurrentTransitionSettings.ExitDuration, 0.01f);
		const float RawAlpha = FMath::Clamp(TransitionElapsedTime / Duration, 0.0f, 1.0f);
		CurrentWidget->SetSimpleUIExitTransitionProgress(RawAlpha);
		if (RawAlpha >= 1.0f)
		{
			CompleteExitTransition();
			return false;
		}
		return true;
	}

	if (!IsValid(CurrentPreviewActor)
		|| CurrentPreviewActor->GetWorld() != SessionWorld.Get()
		|| !IsValid(CurrentTransitionWidget)
		|| !IsValid(HiddenSourceActor)
		|| HiddenSourceActor->GetWorld() != SessionWorld.Get())
	{
		TransitionTickerHandle.Reset();
		State = EJMItemInspectionState::Closing;
		FinalizeCloseInspection(PendingCloseReason);
		return false;
	}

	TransitionElapsedTime += FMath::Max(DeltaTime, 0.0f);
	const float Duration = FMath::Max(CurrentTransitionSettings.ExitDuration, 0.01f);
	const float RawAlpha = FMath::Clamp(TransitionElapsedTime / Duration, 0.0f, 1.0f);
	const float EasedAlpha = ApplyTransitionEasing(RawAlpha, CurrentTransitionSettings.Easing);

	CurrentTransitionWidget->SetTransitionRect(
		FMath::Lerp(TransitionTargetCenter, CurrentTransitionSource.ScreenCenter, EasedAlpha),
		FMath::Lerp(TransitionTargetSize, CurrentTransitionSource.ScreenSize, EasedAlpha));
	CurrentTransitionWidget->SetTransitionOpacity(
		1.0f - FMath::GetMappedRangeValueClamped(FVector2D(0.92f, 1.0f), FVector2D(0.0f, 1.0f), RawAlpha));
	CurrentPreviewActor->UpdateExitTransition(EasedAlpha);
	CurrentWidget->SetExitTransitionProgress(RawAlpha);

	if (RawAlpha >= 0.92f)
	{
		RevealSourceActorForExitHandoff();
	}

	if (RawAlpha >= 1.0f)
	{
		CompleteExitTransition();
		return false;
	}

	return true;
}

void UJMItemInspectionSubsystem::CompleteExitTransition()
{
	if (State != EJMItemInspectionState::TransitioningOut)
	{
		return;
	}

	TransitionTickerHandle.Reset();
	if (CurrentPreviewActor)
	{
		CurrentPreviewActor->CompleteExitTransition();
	}
	if (CurrentWidget)
	{
		if (bUseSimpleUITransition)
		{
			CurrentWidget->CompleteSimpleUIExitTransition();
		}
		else
		{
			CurrentWidget->CompleteExitTransition();
		}
	}
	RevealSourceActorForExitHandoff();
	if (CurrentTransitionWidget)
	{
		CurrentTransitionWidget->RemoveFromParent();
		CurrentTransitionWidget = nullptr;
	}

	const EJMItemInspectionCloseReason CompletedReason = PendingCloseReason;
	State = EJMItemInspectionState::Closing;
	FinalizeCloseInspection(CompletedReason);
}

void UJMItemInspectionSubsystem::CancelExitTransition()
{
	if (TransitionTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TransitionTickerHandle);
		TransitionTickerHandle.Reset();
	}
	if (CurrentTransitionWidget)
	{
		CurrentTransitionWidget->RemoveFromParent();
		CurrentTransitionWidget = nullptr;
	}
	if (CurrentPreviewActor)
	{
		CurrentPreviewActor->CancelExitTransition();
	}
	TransitionElapsedTime = 0.0f;
}

void UJMItemInspectionSubsystem::RevealSourceActorForExitHandoff()
{
	if (bDidHideSourceActor && IsValid(HiddenSourceActor))
	{
		HiddenSourceActor->SetActorHiddenInGame(bSourceActorWasHidden);
		bDidHideSourceActor = false;
	}
}

void UJMItemInspectionSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World && SessionWorld.Get() == World && State != EJMItemInspectionState::Closed)
	{
		CloseInspection(EJMItemInspectionCloseReason::WorldTearDown);
	}
}

bool UJMItemInspectionSubsystem::IsSessionWorldValid() const
{
	UWorld* World = SessionWorld.Get();
	return World && !World->bIsTearingDown && World == GetWorld();
}

void UJMItemInspectionSubsystem::HideSourceActorIfNeeded()
{
	if (bShouldHideSourceActor && !bDidHideSourceActor && IsValid(HiddenSourceActor))
	{
		HiddenSourceActor->SetActorHiddenInGame(true);
		bDidHideSourceActor = true;
	}
}

float UJMItemInspectionSubsystem::ApplyTransitionEasing(float Alpha, EJMItemInspectionTransitionEasing Easing)
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	switch (Easing)
	{
	case EJMItemInspectionTransitionEasing::Linear:
		return SafeAlpha;
	case EJMItemInspectionTransitionEasing::EaseOutBack:
	{
		constexpr float Overshoot = 1.70158f;
		const float Shifted = SafeAlpha - 1.0f;
		return 1.0f + ((Overshoot + 1.0f) * FMath::Pow(Shifted, 3.0f)) + (Overshoot * FMath::Square(Shifted));
	}
	case EJMItemInspectionTransitionEasing::EaseOutCubic:
	default:
		return 1.0f - FMath::Pow(1.0f - SafeAlpha, 3.0f);
	}
}

void UJMItemInspectionSubsystem::DestroyPreviewResources()
{
	if (IsValid(CurrentPreviewActor))
	{
		CurrentPreviewActor->Destroy();
	}
	CurrentPreviewActor = nullptr;

	if (IsValid(CurrentPreviewRenderTarget))
	{
		CurrentPreviewRenderTarget->ReleaseResource();
	}
	CurrentPreviewRenderTarget = nullptr;
}

void UJMItemInspectionSubsystem::FailOpen(const FJMItemInspectionRequest& Request, const FText& Reason)
{
	if (!IsInspectionOpen())
	{
		State = EJMItemInspectionState::Failed;
	}

	UE_LOG(LogJMGameplay, Warning, TEXT("Item inspection open failed: %s"), *Reason.ToString());
	OnInspectionFailed.Broadcast(Request.InspectionData, Reason);
}

void UJMItemInspectionSubsystem::ApplyPlayerInputBlock(APlayerController* PlayerController, bool bShouldBlockInput)
{
	if (!PlayerController || !bShouldBlockInput)
	{
		return;
	}

	InputBlockedPlayerController = PlayerController;

	bAppliedMoveInputBlock = !PlayerController->IsMoveInputIgnored();
	if (bAppliedMoveInputBlock)
	{
		PlayerController->SetIgnoreMoveInput(true);
	}

	bAppliedLookInputBlock = !PlayerController->IsLookInputIgnored();
	if (bAppliedLookInputBlock)
	{
		PlayerController->SetIgnoreLookInput(true);
	}
}

void UJMItemInspectionSubsystem::RestorePlayerInputBlock()
{
	if (APlayerController* PlayerController = InputBlockedPlayerController.Get())
	{
		if (bAppliedMoveInputBlock)
		{
			PlayerController->SetIgnoreMoveInput(false);
		}

		if (bAppliedLookInputBlock)
		{
			PlayerController->SetIgnoreLookInput(false);
		}
	}

	InputBlockedPlayerController = nullptr;
	bAppliedMoveInputBlock = false;
	bAppliedLookInputBlock = false;
}

void UJMItemInspectionSubsystem::SuppressInteractionPrompt()
{
	RestoreInteractionPrompt();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	UJMInteractionComponent* InteractionComponent = nullptr;
	if (APawn* Pawn = PlayerController->GetPawn())
	{
		InteractionComponent = Pawn->FindComponentByClass<UJMInteractionComponent>();
	}
	if (!InteractionComponent)
	{
		InteractionComponent = PlayerController->FindComponentByClass<UJMInteractionComponent>();
	}
	if (!InteractionComponent)
	{
		return;
	}

	SuppressedInteractionComponent = InteractionComponent;
	bInteractionPromptWasSuppressed = InteractionComponent->IsPromptSuppressed();
	InteractionComponent->SetPromptSuppressed(true);
}

void UJMItemInspectionSubsystem::RestoreInteractionPrompt()
{
	if (UJMInteractionComponent* InteractionComponent = SuppressedInteractionComponent.Get())
	{
		InteractionComponent->SetPromptSuppressed(bInteractionPromptWasSuppressed);
	}

	SuppressedInteractionComponent.Reset();
	bInteractionPromptWasSuppressed = false;
}

void UJMItemInspectionSubsystem::RestoreSourceActor()
{
	if (bDidHideSourceActor && IsValid(HiddenSourceActor))
	{
		HiddenSourceActor->SetActorHiddenInGame(bSourceActorWasHidden);
	}

	HiddenSourceActor = nullptr;
	bSourceActorWasHidden = false;
	bShouldHideSourceActor = false;
	bDidHideSourceActor = false;
}

void UJMItemInspectionSubsystem::RestorePauseState()
{
	if (bAppliedGamePause && !bWasGamePausedBeforeOpen)
	{
		if (UWorld* World = SessionWorld.Get())
		{
			UGameplayStatics::SetGamePaused(World, false);
		}
	}

	bWasGamePausedBeforeOpen = false;
	bAppliedGamePause = false;
}

void UJMItemInspectionSubsystem::HandleWidgetCloseRequested(EJMItemInspectionCloseReason Reason)
{
	CloseInspection(Reason);
}

void UJMItemInspectionSubsystem::HandlePreviewDragged(float ScreenDeltaX, float ScreenDeltaY)
{
	if (State == EJMItemInspectionState::Inspecting && CurrentPreviewActor)
	{
		CurrentPreviewActor->RotatePreview(ScreenDeltaX, ScreenDeltaY);
	}
}

void UJMItemInspectionSubsystem::HandlePreviewZoomed(float WheelDelta)
{
	if (State == EJMItemInspectionState::Inspecting && CurrentPreviewActor)
	{
		CurrentPreviewActor->ZoomPreview(WheelDelta);
	}
}

bool UJMItemInspectionSubsystem::TickSessionHealth(float DeltaTime)
{
	if (!IsInspectionOpen()) return true;
	if (!IsSessionWorldValid())
	{
		CloseInspection(EJMItemInspectionCloseReason::WorldTearDown);
	}
	else if (bSessionHadSourceActor && !SessionSourceActor.IsValid())
	{
		CloseInspection(EJMItemInspectionCloseReason::SourceInvalid);
	}
	else if (CurrentWidget && !CurrentWidget->IsInViewport())
	{
		CloseInspection(EJMItemInspectionCloseReason::Failed);
	}
	return true;
}
