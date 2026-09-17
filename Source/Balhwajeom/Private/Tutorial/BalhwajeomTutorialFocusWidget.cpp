#include "Tutorial/BalhwajeomTutorialFocusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Tutorial/BalhwajeomTutorialDirector.h"


UBalhwajeomTutorialFocusWidget::UBalhwajeomTutorialFocusWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The layer is decoration only. Hit testing would let the dim swallow mouse input.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// Current WB_HUID names first, the original generated names as a fallback.
	PhotoCameraIconNames = { TEXT("Image_Camera"), TEXT("Image_0") };
	TabletIconNames = { TEXT("Image_TAB"), TEXT("Image_1") };
}


void UBalhwajeomTutorialFocusWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
}


void UBalhwajeomTutorialFocusWidget::BuildWidgetTree()
{
	// A Blueprint subclass may supply its own tree; only build one when there is none.
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("TutorialFocusCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	DimImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Dim"));
	DimImage->SetColorAndOpacity(FLinearColor::Black);
	DimImage->SetRenderOpacity(0.0f);
	DimImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* DimSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChild(DimImage)))
	{
		// Full screen, so the dim covers the world and every layer below ZOrder 5.
		DimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		DimSlot->SetOffsets(FMargin(0.0f));
	}

	// Added after the dim, so the highlights draw on top of it.
	PhotoCameraHighlight = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("Img_PhotoCameraHighlight"));
	PhotoCameraHighlight->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChild(PhotoCameraHighlight);

	TabletHighlight = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("Img_TabletHighlight"));
	TabletHighlight->SetVisibility(ESlateVisibility::Collapsed);
	RootCanvas->AddChild(TabletHighlight);

	// Added last, so it draws above both highlights.
	HintMessageText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("Txt_HintMessage"));
	HintMessageText->SetVisibility(ESlateVisibility::Collapsed);
	HintMessageText->SetJustification(ETextJustify::Center);
	HintMessageText->SetAutoWrapText(true);
	HintMessageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	HintMessageText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	HintMessageText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FSlateFontInfo HintFont = HintMessageText->GetFont();
	HintFont.Size = 14;
	HintMessageText->SetFont(HintFont);
	if (UCanvasPanelSlot* HintSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChild(HintMessageText)))
	{
		HintSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		HintSlot->SetAutoSize(false);
	}
}


UUserWidget* UBalhwajeomTutorialFocusWidget::GetPlayerHUD() const
{
	const ABalhwajeomCameraPlayerController* PlayerController =
		Cast<ABalhwajeomCameraPlayerController>(GetOwningPlayer());

	return PlayerController ? PlayerController->GetPlayerHUD() : nullptr;
}


UImage* UBalhwajeomTutorialFocusWidget::FindHudIcon(
	const TArray<FName>& SourceIconNames) const
{
	UUserWidget* PlayerHUD = GetPlayerHUD();
	if (!PlayerHUD)
	{
		return nullptr;
	}

	for (const FName& IconName : SourceIconNames)
	{
		if (UImage* Icon = Cast<UImage>(PlayerHUD->GetWidgetFromName(IconName)))
		{
			return Icon;
		}
	}

	return nullptr;
}


void UBalhwajeomTutorialFocusWidget::MirrorHudIcon(
	const FGeometry& MyGeometry,
	const TArray<FName>& SourceIconNames,
	UImage* Highlight,
	bool bShouldShow,
	float PulseOpacity,
	float FadeAlpha)
{
	if (!Highlight)
	{
		return;
	}

	// Still mirrored while fading out, so the highlight stays on its icon on the way down.
	const bool bNeedsIcon = bShouldShow || FadeAlpha > KINDA_SMALL_NUMBER;
	UImage* SourceIcon = bNeedsIcon ? FindHudIcon(SourceIconNames) : nullptr;

	// A HUD icon that is hidden or has never been laid out has no meaningful rectangle,
	// so there is nothing to raise above the dim.
	if (!SourceIcon || !SourceIcon->IsVisible())
	{
		Highlight->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FGeometry& SourceGeometry = SourceIcon->GetCachedGeometry();
	const FVector2D SourceSize = SourceGeometry.GetLocalSize();
	if (SourceSize.IsNearlyZero() || FMath::IsNearlyZero(MyGeometry.Scale))
	{
		Highlight->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Copying the brush keeps the highlight matching whatever art the HUD uses, so the
	// icon never has to be duplicated by hand or kept in sync.
	Highlight->SetBrush(SourceIcon->GetBrush());
	Highlight->SetColorAndOpacity(FLinearColor::White);

	// Fading this copy in and out is what makes the HUD icon read as bright, then dark,
	// then bright again: at zero the dimmed original shows through instead. FadeAlpha
	// scales the whole blink so the highlight can also leave gradually.
	Highlight->SetRenderOpacity(PulseOpacity * FadeAlpha);

	UCanvasPanelSlot* HighlightSlot = Cast<UCanvasPanelSlot>(Highlight->Slot);
	if (!HighlightSlot)
	{
		Highlight->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector2D LocalPosition =
		MyGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D LocalSize = SourceGeometry.GetAbsoluteSize() / MyGeometry.Scale;

	HighlightSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	HighlightSlot->SetAlignment(FVector2D::ZeroVector);
	HighlightSlot->SetAutoSize(false);
	HighlightSlot->SetPosition(LocalPosition);
	HighlightSlot->SetSize(LocalSize);

	Highlight->SetVisibility(ESlateVisibility::HitTestInvisible);
}


void UBalhwajeomTutorialFocusWidget::NativeTick(
	const FGeometry& MyGeometry,
	float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Every decision -- current step, player mode, [F] prompt alpha -- is already made by
	// the director, so this layer only interpolates and positions.
	const float TargetOpacity =
		ABalhwajeomTutorialDirector::GetTutorialDimOpacity(this);
	CurrentDimOpacity = FMath::FInterpTo(
		CurrentDimOpacity, TargetOpacity, InDeltaTime, DimInterpolationSpeed);

	if (DimImage)
	{
		DimImage->SetRenderOpacity(CurrentDimOpacity);
		DimImage->SetVisibility(
			CurrentDimOpacity > KINDA_SMALL_NUMBER
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	const EBalhwajeomTutorialHintTarget HintTarget =
		ABalhwajeomTutorialDirector::GetTutorialHintTarget(this);

	// Timed by the director, so this icon and the [F] prompt blink on the same beat.
	const float PulseOpacity = ABalhwajeomTutorialDirector::GetTutorialHighlightPulse(this);

	const bool bWantsPhotoCamera =
		HintTarget == EBalhwajeomTutorialHintTarget::PhotoCameraIcon;
	const bool bWantsTablet = HintTarget == EBalhwajeomTutorialHintTarget::TabletIcon;
	PhotoCameraHighlightAlpha = FMath::FInterpTo(
		PhotoCameraHighlightAlpha,
		bWantsPhotoCamera ? 1.0f : 0.0f,
		InDeltaTime,
		HighlightInterpolationSpeed);
	TabletHighlightAlpha = FMath::FInterpTo(
		TabletHighlightAlpha,
		bWantsTablet ? 1.0f : 0.0f,
		InDeltaTime,
		HighlightInterpolationSpeed);

	MirrorHudIcon(
		MyGeometry,
		PhotoCameraIconNames,
		PhotoCameraHighlight,
		bWantsPhotoCamera,
		PulseOpacity,
		PhotoCameraHighlightAlpha);
	MirrorHudIcon(
		MyGeometry,
		TabletIconNames,
		TabletHighlight,
		bWantsTablet,
		PulseOpacity,
		TabletHighlightAlpha);

	UpdateHintMessage(HintTarget);
}


void UBalhwajeomTutorialFocusWidget::UpdateHintMessage(
	const EBalhwajeomTutorialHintTarget HintTarget)
{
	if (!HintMessageText)
	{
		return;
	}

	const FText Message = ABalhwajeomTutorialDirector::GetTutorialHintMessage(this);
	UImage* AnchorHighlight = nullptr;
	if (HintTarget == EBalhwajeomTutorialHintTarget::PhotoCameraIcon)
	{
		AnchorHighlight = PhotoCameraHighlight;
	}
	else if (HintTarget == EBalhwajeomTutorialHintTarget::TabletIcon)
	{
		AnchorHighlight = TabletHighlight;
	}

	// No message, or its icon is not actually shown right now (hidden HUD, other mode
	// owns the screen): nothing to anchor the text to.
	if (Message.IsEmpty() || !AnchorHighlight ||
		AnchorHighlight->GetVisibility() == ESlateVisibility::Collapsed)
	{
		HintMessageText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UCanvasPanelSlot* AnchorSlot = Cast<UCanvasPanelSlot>(AnchorHighlight->Slot);
	UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(HintMessageText->Slot);
	if (!AnchorSlot || !TextSlot)
	{
		HintMessageText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	HintMessageText->SetText(Message);
	HintMessageText->SetVisibility(ESlateVisibility::HitTestInvisible);

	// Above the highlighted icon, resting a small gap off its top edge and shifted right
	// of center so it does not sit directly over the icon itself.
	const FVector2D IconPosition = AnchorSlot->GetPosition();
	const FVector2D IconSize = AnchorSlot->GetSize();
	const FVector2D TextSize(260.0f, 30.0f);
	constexpr float RightShift = 60.0f;
	constexpr float TopGap = 60.0f;
	TextSlot->SetPosition(FVector2D(
		IconPosition.X + IconSize.X * 0.5f - TextSize.X * 0.5f + RightShift,
		IconPosition.Y - TextSize.Y - TopGap));
	TextSlot->SetSize(TextSize);
}
