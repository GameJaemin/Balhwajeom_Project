#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemInspection/JMItemInspectionTypes.h"
#include "JMItemInspectionWidgetBase.generated.h"

class UButton;
class UBorder;
class UImage;
class UJMItemInspectionData;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextBlock;
class UTexture;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMItemInspectionCloseRequestedSignature, EJMItemInspectionCloseReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJMItemInspectionPreviewDraggedSignature, float, ScreenDeltaX, float, ScreenDeltaY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMItemInspectionPreviewZoomedSignature, float, WheelDelta);

UCLASS(BlueprintType, Blueprintable)
class ITEMINSPECTORRUNTIME_API UJMItemInspectionWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UJMItemInspectionWidgetBase(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionCloseRequestedSignature OnCloseRequested;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionPreviewDraggedSignature OnPreviewDragged;

	UPROPERTY(BlueprintAssignable, Category = "JM Gameplay|Item Inspection")
	FJMItemInspectionPreviewZoomedSignature OnPreviewZoomed;

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetInspectionData(UJMItemInspectionData* InInspectionData);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetItemName(const FText& InItemName);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetCategoryText(const FText& InCategoryText);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetDescription(const FText& InDescription);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetAdditionalInfo(const FText& InAdditionalInfo);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetPreviewTexture(UTexture* InPreviewTexture);

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	virtual void SetPreviewStatusText(const FText& InStatusText);

	void BeginEnterTransition(float InspectorFadeStartAlpha);
	void SetEnterTransitionProgress(float Alpha);
	void CompleteEnterTransition();
	void BeginExitTransition();
	void SetExitTransitionProgress(float Alpha);
	void CompleteExitTransition();
	void BeginSimpleUIEnterTransition();
	void SetSimpleUIEnterTransitionProgress(float Alpha);
	void CompleteSimpleUIEnterTransition();
	void BeginSimpleUIExitTransition();
	void SetSimpleUIExitTransitionProgress(float Alpha);
	void CompleteSimpleUIExitTransition();
	void SetPreviewInputEnabled(bool bEnabled);
	bool GetPreviewViewportRect(FVector2D& OutCenter, FVector2D& OutSize) const;

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection")
	void RequestClose(EJMItemInspectionCloseReason Reason = EJMItemInspectionCloseReason::User);

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection")
	void OnInspectionOpened(UJMItemInspectionData* InInspectionData);

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection")
	void OnInspectionClosed(EJMItemInspectionCloseReason Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Transition")
	void OnEnterTransitionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Transition")
	void OnEnterTransitionUpdated(float Alpha);

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Transition")
	void OnEnterTransitionCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Transition")
	void OnExitTransitionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Transition")
	void OnExitTransitionUpdated(float Alpha);

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Transition")
	void OnExitTransitionCompleted();

protected:
	void BuildDefaultWidgetTreeIfNeeded();
	void ApplyInspectionDataToWidgets();
	void SetTextBlockOrHide(UTextBlock* TextBlock, const FText& Text);
	bool IsPointerOverPreviewArea(const FPointerEvent& InMouseEvent) const;
	void ReleasePreviewMouseCapture();
	void ResolveSimpleTransitionLayers();
	void ApplySimpleUITransitionVisual(float Alpha);

	UFUNCTION()
	void HandleCloseButtonClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UBorder> PreviewPanel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UImage> PreviewImage = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UTextBlock> ItemNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UTextBlock> CategoryText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UTextBlock> AdditionalInfoText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UTextBlock> ItemIdText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UTextBlock> PreviewStatusText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection")
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UJMItemInspectionData> InspectionData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewMaterialAsset = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> SimpleTransitionBackdrop = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> SimpleTransitionContent = nullptr;

	bool bPreviewDragging = false;
	bool bPreviewInputEnabled = true;
	float CurrentInspectorFadeStartAlpha = 0.55f;
};
