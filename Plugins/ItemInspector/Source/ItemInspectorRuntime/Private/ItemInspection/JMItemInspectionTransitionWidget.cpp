#include "ItemInspection/JMItemInspectionTransitionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UJMItemInspectionTransitionWidget::UJMItemInspectionTransitionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterialFinder(TEXT("/ItemInspector/ItemInspection/M_JMItemPreviewAlpha.M_JMItemPreviewAlpha"));
	if (PreviewMaterialFinder.Succeeded())
	{
		PreviewMaterialAsset = PreviewMaterialFinder.Object;
	}
}

void UJMItemInspectionTransitionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTreeIfNeeded();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UJMItemInspectionTransitionWidget::RebuildWidget()
{
	BuildWidgetTreeIfNeeded();
	return Super::RebuildWidget();
}

void UJMItemInspectionTransitionWidget::SetTransitionTexture(UTexture* InTexture)
{
	BuildWidgetTreeIfNeeded();
	if (!TransitionImage)
	{
		return;
	}

	if (!InTexture)
	{
		TransitionImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (!PreviewMaterialInstance && PreviewMaterialAsset)
	{
		PreviewMaterialInstance = UMaterialInstanceDynamic::Create(PreviewMaterialAsset, this);
	}

	if (PreviewMaterialInstance)
	{
		PreviewMaterialInstance->SetTextureParameterValue(TEXT("PreviewTexture"), InTexture);
		TransitionImage->SetBrushFromMaterial(PreviewMaterialInstance);
	}
	else
	{
		TransitionImage->SetBrushResourceObject(InTexture);
	}

	TransitionImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UJMItemInspectionTransitionWidget::SetTransitionRect(const FVector2D& Center, const FVector2D& Size)
{
	BuildWidgetTreeIfNeeded();
	if (UCanvasPanelSlot* CanvasSlot = TransitionImage ? Cast<UCanvasPanelSlot>(TransitionImage->Slot) : nullptr)
	{
		CanvasSlot->SetPosition(Center);
		CanvasSlot->SetSize(FVector2D(FMath::Max(Size.X, 1.0f), FMath::Max(Size.Y, 1.0f)));
	}
}

void UJMItemInspectionTransitionWidget::SetTransitionOpacity(float InOpacity)
{
	if (TransitionImage)
	{
		TransitionImage->SetRenderOpacity(FMath::Clamp(InOpacity, 0.0f, 1.0f));
	}
}

void UJMItemInspectionTransitionWidget::BuildWidgetTreeIfNeeded()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TransitionRoot"));
	RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	TransitionImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TransitionImage"));
	TransitionImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(TransitionImage))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(false);
	}
}
