#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ItemInspection/JMItemInspectionTypes.h"
#include "JMItemInspectionSubsystem.generated.h"

class UJMItemInspectionData;
class UJMItemInspectionWidgetBase;
class UJMItemInspectionTransitionWidget;
class UTextureRenderTarget2D;
class AJMItemInspectionPreviewActor;
class APlayerController;
class UJMInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMItemInspectionOpenedSignature, UJMItemInspectionData*, InspectionData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMItemInspectionClosedSignature, EJMItemInspectionCloseReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJMItemInspectionFailedSignature, UJMItemInspectionData*, InspectionData, FText, Reason);

UCLASS()
class ITEMINSPECTORRUNTIME_API UJMItemInspectionSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	friend struct FJMInspectionTestAccessor;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionOpenedSignature OnInspectionOpened;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionClosedSignature OnInspectionClosed;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionFailedSignature OnInspectionFailed;

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	bool OpenInspection(UJMItemInspectionData* InspectionData);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	bool OpenInspectionFromRequest(const FJMItemInspectionRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	void CloseInspection(EJMItemInspectionCloseReason Reason = EJMItemInspectionCloseReason::ExternalRequest);

	UFUNCTION(BlueprintPure, Category = "JM Gameplay|Item Inspection")
	bool IsInspectionOpen() const;

	UFUNCTION(BlueprintPure, Category = "JM Gameplay|Item Inspection")
	UJMItemInspectionData* GetCurrentInspectionData() const;

	UFUNCTION(BlueprintPure, Category = "JM Gameplay|Item Inspection")
	EJMItemInspectionState GetInspectionState() const;

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	void ResetPreviewRotation();

protected:
	bool TickSessionHealth(float DeltaTime);
	bool CreateInspectionWidget(const FJMItemInspectionRequest& Request);
	bool CreatePreviewResources(const FJMItemInspectionRequest& Request);
	void DestroyPreviewResources();
	bool TryStartEnterTransition(const FJMItemInspectionRequest& Request);
	bool TryStartSimpleEnterTransition();
	bool BuildTransitionSource(const FJMItemInspectionRequest& Request, FJMItemInspectionTransitionSource& OutSource) const;
	FJMItemInspectionTransitionSettings ResolveTransitionSettings(const FJMItemInspectionRequest& Request) const;
	bool TickEnterTransition(float DeltaTime);
	void CompleteEnterTransition();
	void CancelEnterTransition();
	bool TryStartExitTransition(EJMItemInspectionCloseReason Reason);
	bool TickExitTransition(float DeltaTime);
	void CompleteExitTransition();
	void CancelExitTransition();
	void FinalizeCloseInspection(EJMItemInspectionCloseReason Reason);
	void RevealSourceActorForExitHandoff();
	void HideSourceActorIfNeeded();
	static float ApplyTransitionEasing(float Alpha, EJMItemInspectionTransitionEasing Easing);
	void FailOpen(const FJMItemInspectionRequest& Request, const FText& Reason);
	void ApplyPlayerInputBlock(APlayerController* PlayerController, bool bShouldBlockInput);
	void RestorePlayerInputBlock();
	void RestoreSourceActor();
	void RestorePauseState();
	void SuppressInteractionPrompt();
	void RestoreInteractionPrompt();
	void PublishModalPresentation(bool bIsOpen);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	bool IsSessionWorldValid() const;

	UFUNCTION()
	void HandleWidgetCloseRequested(EJMItemInspectionCloseReason Reason);

	UFUNCTION()
	void HandlePreviewDragged(float ScreenDeltaX, float ScreenDeltaY);

	UFUNCTION()
	void HandlePreviewZoomed(float WheelDelta);

	UPROPERTY(Transient)
	EJMItemInspectionState State = EJMItemInspectionState::Closed;

	UPROPERTY(Transient)
	TObjectPtr<UJMItemInspectionData> CurrentInspectionData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UJMItemInspectionWidgetBase> CurrentWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UJMItemInspectionTransitionWidget> CurrentTransitionWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> HiddenSourceActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> CurrentPreviewRenderTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AJMItemInspectionPreviewActor> CurrentPreviewActor = nullptr;

	TWeakObjectPtr<APlayerController> InputBlockedPlayerController;
	TWeakObjectPtr<UJMInteractionComponent> SuppressedInteractionComponent;
	FTSTicker::FDelegateHandle TransitionTickerHandle;
	FTSTicker::FDelegateHandle SessionHealthTickerHandle;
	TWeakObjectPtr<AActor> SessionSourceActor;
	bool bSessionHadSourceActor = false;
	bool bAppliedCursorState = false;
	FJMItemInspectionTransitionSource CurrentTransitionSource;
	FJMItemInspectionTransitionSettings CurrentTransitionSettings;
	FVector2D TransitionTargetCenter = FVector2D::ZeroVector;
	FVector2D TransitionTargetSize = FVector2D::ZeroVector;
	float TransitionElapsedTime = 0.0f;
	float TransitionLayoutWaitTime = 0.0f;
	bool bTransitionTargetResolved = false;
	bool bSourceActorWasHidden = false;
	bool bShouldHideSourceActor = false;
	bool bDidHideSourceActor = false;
	EJMItemInspectionCloseReason PendingCloseReason = EJMItemInspectionCloseReason::ExternalRequest;
	bool bWasGamePausedBeforeOpen = false;
	bool bAppliedGamePause = false;
	bool bAppliedMoveInputBlock = false;
	bool bAppliedLookInputBlock = false;
	bool bPreviousMouseCursor = false;
	bool bInteractionPromptWasSuppressed = false;
	bool bModalPresentationPublished = false;
	bool bUseSimpleUITransition = false;
	TWeakObjectPtr<UWorld> SessionWorld;
	uint64 SessionSerial = 0;
	bool bCloseInProgress = false;
	bool bDeinitializing = false;
};
