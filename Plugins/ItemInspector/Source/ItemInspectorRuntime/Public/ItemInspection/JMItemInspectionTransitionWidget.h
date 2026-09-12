#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JMItemInspectionTransitionWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture;

/** Full-viewport, hit-test-invisible image used while a world item flies into the inspector. */
UCLASS()
class ITEMINSPECTORRUNTIME_API UJMItemInspectionTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UJMItemInspectionTransitionWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	void SetTransitionTexture(UTexture* InTexture);
	void SetTransitionRect(const FVector2D& Center, const FVector2D& Size);
	void SetTransitionOpacity(float InOpacity);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	void BuildWidgetTreeIfNeeded();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TransitionImage = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewMaterialAsset = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance = nullptr;
};
