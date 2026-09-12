#include "ItemInspection/JMItemInspectionWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Internationalization/Text.h"
#include "InputCoreTypes.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/ConstructorHelpers.h"

UJMItemInspectionWidgetBase::UJMItemInspectionWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterialFinder(TEXT("/ItemInspector/ItemInspection/M_JMItemPreviewAlpha.M_JMItemPreviewAlpha"));
	if (PreviewMaterialFinder.Succeeded())
	{
		PreviewMaterialAsset = PreviewMaterialFinder.Object;
	}
}

void UJMItemInspectionWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	BuildDefaultWidgetTreeIfNeeded();
	ResolveSimpleTransitionLayers();

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UJMItemInspectionWidgetBase::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UJMItemInspectionWidgetBase::HandleCloseButtonClicked);
	}

	ApplyInspectionDataToWidgets();
}

void UJMItemInspectionWidgetBase::NativeDestruct()
{
	ReleasePreviewMouseCapture();

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UJMItemInspectionWidgetBase::HandleCloseButtonClicked);
	}

	Super::NativeDestruct();
}

FReply UJMItemInspectionWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RequestClose(EJMItemInspectionCloseReason::User);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UJMItemInspectionWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPreviewInputEnabled && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsPointerOverPreviewArea(InMouseEvent))
	{
		bPreviewDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UJMItemInspectionWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && (bPreviewDragging || HasMouseCapture()))
	{
		bPreviewDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UJMItemInspectionWidgetBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPreviewDragging && (!bPreviewInputEnabled || !IsPointerOverPreviewArea(InMouseEvent)
		|| !InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)))
	{
		bPreviewDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (bPreviewDragging && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		const auto CursorDelta = InMouseEvent.GetCursorDelta();
		OnPreviewDragged.Broadcast(static_cast<float>(CursorDelta.X), static_cast<float>(CursorDelta.Y));
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UJMItemInspectionWidgetBase::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPreviewInputEnabled && IsPointerOverPreviewArea(InMouseEvent))
	{
		OnPreviewZoomed.Broadcast(InMouseEvent.GetWheelDelta());
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void UJMItemInspectionWidgetBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	ReleasePreviewMouseCapture();
	Super::NativeOnMouseLeave(InMouseEvent);
}

void UJMItemInspectionWidgetBase::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bPreviewDragging = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UJMItemInspectionWidgetBase::SetInspectionData(UJMItemInspectionData* InInspectionData)
{
	InspectionData = InInspectionData;
	BuildDefaultWidgetTreeIfNeeded();
	ApplyInspectionDataToWidgets();
}

void UJMItemInspectionWidgetBase::SetItemName(const FText& InItemName)
{
	SetTextBlockOrHide(ItemNameText, InItemName);
}

void UJMItemInspectionWidgetBase::SetCategoryText(const FText& InCategoryText)
{
	SetTextBlockOrHide(CategoryText, InCategoryText);
}

void UJMItemInspectionWidgetBase::SetDescription(const FText& InDescription)
{
	SetTextBlockOrHide(DescriptionText, InDescription);
}

void UJMItemInspectionWidgetBase::SetAdditionalInfo(const FText& InAdditionalInfo)
{
	SetTextBlockOrHide(AdditionalInfoText, InAdditionalInfo);
}

void UJMItemInspectionWidgetBase::SetPreviewTexture(UTexture* InPreviewTexture)
{
	if (PreviewImage)
	{
		if (InPreviewTexture)
		{
			if (!PreviewMaterialInstance)
			{
				if (!PreviewMaterialAsset)
				{
					static const TCHAR* PreviewMaterialPath = TEXT("/ItemInspector/ItemInspection/M_JMItemPreviewAlpha.M_JMItemPreviewAlpha");
					PreviewMaterialAsset = LoadObject<UMaterialInterface>(nullptr, PreviewMaterialPath);
				}

				if (PreviewMaterialAsset)
				{
					PreviewMaterialInstance = UMaterialInstanceDynamic::Create(PreviewMaterialAsset, this);
				}
			}

			if (PreviewMaterialInstance)
			{
				PreviewMaterialInstance->SetTextureParameterValue(TEXT("PreviewTexture"), InPreviewTexture);
				PreviewImage->SetBrushFromMaterial(PreviewMaterialInstance);
			}
			else
			{
				PreviewImage->SetBrushResourceObject(InPreviewTexture);
			}

			if (const UTextureRenderTarget2D* RenderTarget = Cast<UTextureRenderTarget2D>(InPreviewTexture))
			{
				PreviewImage->SetDesiredSizeOverride(FVector2D(RenderTarget->SizeX, RenderTarget->SizeY));
			}
			PreviewImage->SetVisibility(ESlateVisibility::Visible);
			if (PreviewStatusText)
			{
				PreviewStatusText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
			if (PreviewStatusText)
			{
				PreviewStatusText->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void UJMItemInspectionWidgetBase::SetPreviewStatusText(const FText& InStatusText)
{
	SetTextBlockOrHide(PreviewStatusText, InStatusText);
}

void UJMItemInspectionWidgetBase::BeginEnterTransition(float InspectorFadeStartAlpha)
{
	CurrentInspectorFadeStartAlpha = FMath::Clamp(InspectorFadeStartAlpha, 0.0f, 0.99f);
	bPreviewInputEnabled = false;
	bPreviewDragging = false;
	SetRenderOpacity(0.0f);
	OnEnterTransitionStarted();
}

void UJMItemInspectionWidgetBase::SetEnterTransitionProgress(float Alpha)
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	const float FadeAlpha = FMath::GetRangePct(CurrentInspectorFadeStartAlpha, 1.0f, SafeAlpha);
	SetRenderOpacity(FMath::Clamp(FadeAlpha, 0.0f, 1.0f));
	OnEnterTransitionUpdated(SafeAlpha);
}

void UJMItemInspectionWidgetBase::CompleteEnterTransition()
{
	SetRenderOpacity(1.0f);
	bPreviewInputEnabled = true;
	OnEnterTransitionCompleted();
}

void UJMItemInspectionWidgetBase::BeginExitTransition()
{
	bPreviewInputEnabled = false;
	bPreviewDragging = false;
	SetRenderOpacity(1.0f);
	OnExitTransitionStarted();
}

void UJMItemInspectionWidgetBase::SetExitTransitionProgress(float Alpha)
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	SetRenderOpacity(1.0f - SafeAlpha);
	OnExitTransitionUpdated(SafeAlpha);
}

void UJMItemInspectionWidgetBase::CompleteExitTransition()
{
	SetRenderOpacity(0.0f);
	OnExitTransitionCompleted();
}

void UJMItemInspectionWidgetBase::BeginSimpleUIEnterTransition()
{
	bPreviewInputEnabled = false;
	bPreviewDragging = false;
	ResolveSimpleTransitionLayers();
	ApplySimpleUITransitionVisual(0.0f);
	OnEnterTransitionStarted();
}

void UJMItemInspectionWidgetBase::SetSimpleUIEnterTransitionProgress(float Alpha)
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	ApplySimpleUITransitionVisual(SafeAlpha);
	OnEnterTransitionUpdated(SafeAlpha);
}

void UJMItemInspectionWidgetBase::CompleteSimpleUIEnterTransition()
{
	ApplySimpleUITransitionVisual(1.0f);
	bPreviewInputEnabled = true;
	OnEnterTransitionCompleted();
}

void UJMItemInspectionWidgetBase::BeginSimpleUIExitTransition()
{
	ReleasePreviewMouseCapture();
	bPreviewInputEnabled = false;
	ResolveSimpleTransitionLayers();
	ApplySimpleUITransitionVisual(1.0f);
	OnExitTransitionStarted();
}

void UJMItemInspectionWidgetBase::SetSimpleUIExitTransitionProgress(float Alpha)
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	ApplySimpleUITransitionVisual(1.0f - SafeAlpha);
	OnExitTransitionUpdated(SafeAlpha);
}

void UJMItemInspectionWidgetBase::CompleteSimpleUIExitTransition()
{
	ApplySimpleUITransitionVisual(0.0f);
	OnExitTransitionCompleted();
}

void UJMItemInspectionWidgetBase::SetPreviewInputEnabled(bool bEnabled)
{
	bPreviewInputEnabled = bEnabled;
	if (!bEnabled)
	{
		ReleasePreviewMouseCapture();
	}
}

bool UJMItemInspectionWidgetBase::GetPreviewViewportRect(FVector2D& OutCenter, FVector2D& OutSize) const
{
	if (!PreviewPanel)
	{
		return false;
	}

	const FGeometry& Geometry = PreviewPanel->GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
	{
		return false;
	}

	FVector2D PixelTopLeft;
	FVector2D ViewportTopLeft;
	USlateBlueprintLibrary::AbsoluteToViewport(this, Geometry.LocalToAbsolute(FVector2D::ZeroVector), PixelTopLeft, ViewportTopLeft);
	FVector2D PixelBottomRight;
	FVector2D ViewportBottomRight;
	USlateBlueprintLibrary::AbsoluteToViewport(this, Geometry.LocalToAbsolute(LocalSize), PixelBottomRight, ViewportBottomRight);

	OutSize = ViewportBottomRight - ViewportTopLeft;
	OutCenter = ViewportTopLeft + (OutSize * 0.5f);
	return OutSize.X > 1.0f && OutSize.Y > 1.0f;
}

void UJMItemInspectionWidgetBase::RequestClose(EJMItemInspectionCloseReason Reason)
{
	ReleasePreviewMouseCapture();
	OnCloseRequested.Broadcast(Reason);
}

void UJMItemInspectionWidgetBase::BuildDefaultWidgetTreeIfNeeded()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("InspectorRoot"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.92f));
	if (UOverlaySlot* BackdropSlot = RootOverlay->AddChildToOverlay(Backdrop))
	{
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContentRow"));
	if (UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentRow))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
		ContentSlot->SetPadding(FMargin(48.0f));
	}

	USizeBox* PreviewSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewSizeBox"));
	PreviewSizeBox->SetMinDesiredWidth(560.0f);
	if (UHorizontalBoxSlot* PreviewSlot = ContentRow->AddChildToHorizontalBox(PreviewSizeBox))
	{
		PreviewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 32.0f, 0.0f));
	}

	PreviewPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PreviewPanel"));
	PreviewPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	PreviewSizeBox->AddChild(PreviewPanel);

	UOverlay* PreviewOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PreviewOverlay"));
	PreviewPanel->SetContent(PreviewOverlay);

	PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PreviewImage"));
	if (UOverlaySlot* PreviewImageSlot = PreviewOverlay->AddChildToOverlay(PreviewImage))
	{
		PreviewImageSlot->SetHorizontalAlignment(HAlign_Fill);
		PreviewImageSlot->SetVerticalAlignment(VAlign_Fill);
	}

	PreviewStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviewStatusText"));
	PreviewStatusText->SetText(NSLOCTEXT("JMGameplay", "PreviewPendingStatus", "Static Mesh 미리보기는 다음 단계에서 추가됩니다."));
	PreviewStatusText->SetAutoWrapText(true);
	PreviewStatusText->SetJustification(ETextJustify::Center);
	PreviewStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.78f, 0.82f, 1.0f)));
	if (UOverlaySlot* PreviewStatusSlot = PreviewOverlay->AddChildToOverlay(PreviewStatusText))
	{
		PreviewStatusSlot->SetHorizontalAlignment(HAlign_Center);
		PreviewStatusSlot->SetVerticalAlignment(VAlign_Center);
		PreviewStatusSlot->SetPadding(FMargin(32.0f));
	}

	UVerticalBox* InfoPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoPanel"));
	if (UHorizontalBoxSlot* InfoSlot = ContentRow->AddChildToHorizontalBox(InfoPanel))
	{
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	ItemNameText->SetText(NSLOCTEXT("JMGameplay", "UnnamedInspectionItem", "이름 없는 아이템"));
	ItemNameText->SetAutoWrapText(true);
	ItemNameText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFontStyle("Bold", 26)));
	if (UVerticalBoxSlot* NameSlot = InfoPanel->AddChildToVerticalBox(ItemNameText))
	{
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	CategoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CategoryText"));
	CategoryText->SetText(FText::GetEmpty());
	CategoryText->SetAutoWrapText(true);
	CategoryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.75f, 1.0f, 1.0f)));
	if (UVerticalBoxSlot* CategorySlot = InfoPanel->AddChildToVerticalBox(CategoryText))
	{
		CategorySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UScrollBox* DescriptionScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DescriptionScrollBox"));
	if (UVerticalBoxSlot* ScrollSlot = InfoPanel->AddChildToVerticalBox(DescriptionScrollBox))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
	}

	DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
	DescriptionText->SetText(NSLOCTEXT("JMGameplay", "NoInspectionDescription", "이 아이템에는 아직 설명이 작성되지 않았습니다."));
	DescriptionText->SetAutoWrapText(true);
	DescriptionScrollBox->AddChild(DescriptionText);

	AdditionalInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AdditionalInfoText"));
	AdditionalInfoText->SetText(FText::GetEmpty());
	AdditionalInfoText->SetAutoWrapText(true);
	AdditionalInfoText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.84f, 0.88f, 1.0f)));
	if (UVerticalBoxSlot* AdditionalSlot = InfoPanel->AddChildToVerticalBox(AdditionalInfoText))
	{
		AdditionalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		AdditionalSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	ItemIdText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemIdText"));
	ItemIdText->SetText(FText::GetEmpty());
	ItemIdText->SetAutoWrapText(true);
	ItemIdText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.57f, 0.61f, 1.0f)));
	if (UVerticalBoxSlot* ItemIdSlot = InfoPanel->AddChildToVerticalBox(ItemIdText))
	{
		ItemIdSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ItemIdSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	if (UVerticalBoxSlot* ButtonSlot = InfoPanel->AddChildToVerticalBox(CloseButton))
	{
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
	CloseText->SetText(NSLOCTEXT("JMGameplay", "InspectionCloseButton", "닫기 (ESC)"));
	CloseButton->AddChild(CloseText);

	ApplyInspectionDataToWidgets();
	SetPreviewTexture(nullptr);
}

void UJMItemInspectionWidgetBase::ApplyInspectionDataToWidgets()
{
	if (!InspectionData)
	{
		SetItemName(NSLOCTEXT("JMGameplay", "MissingInspectionDataTitle", "조사 데이터 없음"));
		SetCategoryText(FText::GetEmpty());
		SetDescription(NSLOCTEXT("JMGameplay", "MissingInspectionDataDescription", "Inspector에 전달된 조사 데이터가 없습니다."));
		SetAdditionalInfo(FText::GetEmpty());
		SetTextBlockOrHide(ItemIdText, FText::GetEmpty());
		return;
	}

	SetItemName(InspectionData->DisplayName.IsEmpty()
		? NSLOCTEXT("JMGameplay", "UnnamedInspectionDataTitle", "이름 없는 아이템")
		: InspectionData->DisplayName);
	SetCategoryText(InspectionData->DisplayCategory);
	SetDescription(InspectionData->Description.IsEmpty()
		? NSLOCTEXT("JMGameplay", "EmptyInspectionDescription", "이 아이템에는 아직 설명이 작성되지 않았습니다.")
		: InspectionData->Description);
	SetAdditionalInfo(InspectionData->AdditionalInfo);

	const FText ItemIdLabel = InspectionData->ItemId.IsNone()
		? FText::GetEmpty()
		: FText::Format(NSLOCTEXT("JMGameplay", "InspectionItemIdFormat", "ID: {0}"), FText::FromName(InspectionData->ItemId));
	SetTextBlockOrHide(ItemIdText, ItemIdLabel);
}

void UJMItemInspectionWidgetBase::SetTextBlockOrHide(UTextBlock* TextBlock, const FText& Text)
{
	if (!TextBlock)
	{
		return;
	}

	TextBlock->SetText(Text);
	TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

bool UJMItemInspectionWidgetBase::IsPointerOverPreviewArea(const FPointerEvent& InMouseEvent) const
{
	if (!PreviewPanel)
	{
		return false;
	}

	return PreviewPanel->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
}

void UJMItemInspectionWidgetBase::ResolveSimpleTransitionLayers()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!SimpleTransitionBackdrop)
	{
		SimpleTransitionBackdrop = WidgetTree->FindWidget(TEXT("Backdrop"));
	}
	if (!SimpleTransitionContent)
	{
		SimpleTransitionContent = WidgetTree->FindWidget(TEXT("MainRow"));
		if (!SimpleTransitionContent)
		{
			SimpleTransitionContent = WidgetTree->FindWidget(TEXT("ContentRow"));
		}
	}
}

void UJMItemInspectionWidgetBase::ApplySimpleUITransitionVisual(float Alpha)
{
	const float SafeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	if (!SimpleTransitionBackdrop || !SimpleTransitionContent)
	{
		SetRenderOpacity(SafeAlpha);
		SetRenderScale(FVector2D(FMath::Lerp(0.985f, 1.0f, SafeAlpha)));
		SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(8.0f, 0.0f, SafeAlpha)));
		return;
	}

	SetRenderOpacity(1.0f);
	SetRenderScale(FVector2D::UnitVector);
	SetRenderTranslation(FVector2D::ZeroVector);
	SimpleTransitionBackdrop->SetRenderOpacity(1.0f);
	SimpleTransitionContent->SetRenderOpacity(SafeAlpha);
	SimpleTransitionContent->SetRenderScale(FVector2D(FMath::Lerp(0.985f, 1.0f, SafeAlpha)));
	SimpleTransitionContent->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(8.0f, 0.0f, SafeAlpha)));
}

void UJMItemInspectionWidgetBase::ReleasePreviewMouseCapture()
{
	bPreviewDragging = false;
	if (HasMouseCapture() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture();
	}
}

void UJMItemInspectionWidgetBase::HandleCloseButtonClicked()
{
	RequestClose(EJMItemInspectionCloseReason::CloseButton);
}
