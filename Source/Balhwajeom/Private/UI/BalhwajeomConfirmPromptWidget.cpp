#include "UI/BalhwajeomConfirmPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"

namespace
{
	/** The same faces the tutorial overlay uses, so the two full-screen layers match. */
	const TCHAR* RegularFontPath =
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-4Regular_Font.Freesentation-4Regular_Font");
	const TCHAR* SemiBoldFontPath =
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-6SemiBold_Font.Freesentation-6SemiBold_Font");

	/** Keeps the inherited font when the Korean face is missing rather than losing the size. */
	FSlateFontInfo MakePromptFont(
		const FSlateFontInfo& InheritedFont,
		const int32 Size,
		const TCHAR* FontPath = RegularFontPath)
	{
		FSlateFontInfo Font = InheritedFont;
		Font.Size = Size;
		if (UFont* KoreanFont = LoadObject<UFont>(nullptr, FontPath))
		{
			Font.FontObject = KoreanFont;
		}
		return Font;
	}

	UTextBlock* MakeButtonLabel(UWidgetTree& Tree, const FText& Label, const FName& Name)
	{
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetFont(MakePromptFont(Text->GetFont(), 20, SemiBoldFontPath));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		return Text;
	}
}

void UBalhwajeomConfirmPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ConfirmLabel.IsEmpty())
	{
		ConfirmLabel = NSLOCTEXT("Balhwajeom", "ConfirmYes", "네");
	}
	if (CancelLabel.IsEmpty())
	{
		CancelLabel = NSLOCTEXT("Balhwajeom", "ConfirmNo", "아니오");
	}

	EnsureFallbackLayout();

	if (BTN_Confirm)
	{
		BTN_Confirm->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmClicked);
	}
	if (BTN_Cancel)
	{
		BTN_Cancel->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancelClicked);
	}

	SetRenderOpacity(0.0f);
}

void UBalhwajeomConfirmPromptWidget::EnsureFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		// A Widget Blueprint already supplied a tree; its BindWidgetOptional members win.
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), TEXT("OVL_Root"));
	if (UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(RootOverlay))
	{
		RootSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		RootSlot->SetOffsets(FMargin(0.0f));
	}

	// Blurs whatever the game already drew, exactly as the tutorial overlay does.
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(
		UBackgroundBlur::StaticClass(), TEXT("BG_Blur"));
	Blur->SetBlurStrength(6.0f);
	if (UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(Blur))
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// A Border rather than an Image: an Image with no texture is not a reliable solid fill.
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BRD_Dim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	if (UOverlaySlot* DimSlot = RootOverlay->AddChildToOverlay(Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* ContentBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("SB_Content"));
	ContentBox->SetWidthOverride(800.0f);
	if (UOverlaySlot* ContentSlot = RootOverlay->AddChildToOverlay(ContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("VB_Content"));
	ContentBox->SetContent(Column);

	TXT_Title = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TXT_Title"));
	TXT_Title->SetJustification(ETextJustify::Center);
	TXT_Title->SetFont(MakePromptFont(TXT_Title->GetFont(), 34, SemiBoldFontPath));
	TXT_Title->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TXT_Title))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	TXT_Message = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TXT_Message"));
	TXT_Message->SetJustification(ETextJustify::Center);
	TXT_Message->SetFont(MakePromptFont(TXT_Message->GetFont(), 22));
	TXT_Message->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.88f, 0.88f, 1.0f)));
	TXT_Message->SetAutoWrapText(true);
	if (UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(TXT_Message))
	{
		MessageSlot->SetHorizontalAlignment(HAlign_Center);
		MessageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 40.0f));
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("HB_Buttons"));
	if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(ButtonRow))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	BTN_Confirm = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BTN_Confirm"));
	if (UButtonSlot* ConfirmContentSlot =
		Cast<UButtonSlot>(BTN_Confirm->AddChild(MakeButtonLabel(*WidgetTree, ConfirmLabel, TEXT("TXT_Confirm")))))
	{
		ConfirmContentSlot->SetPadding(FMargin(48.0f, 14.0f));
	}
	if (UHorizontalBoxSlot* ConfirmSlot = ButtonRow->AddChildToHorizontalBox(BTN_Confirm))
	{
		ConfirmSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
	}

	BTN_Cancel = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BTN_Cancel"));
	if (UButtonSlot* CancelContentSlot =
		Cast<UButtonSlot>(BTN_Cancel->AddChild(MakeButtonLabel(*WidgetTree, CancelLabel, TEXT("TXT_Cancel")))))
	{
		CancelContentSlot->SetPadding(FMargin(48.0f, 14.0f));
	}
	ButtonRow->AddChildToHorizontalBox(BTN_Cancel);
}

void UBalhwajeomConfirmPromptWidget::Present(const FText& InTitle, const FText& InMessage)
{
	if (TXT_Title)
	{
		TXT_Title->SetText(InTitle);
		TXT_Title->SetVisibility(
			InTitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (TXT_Message)
	{
		TXT_Message->SetText(InMessage);
		TXT_Message->SetVisibility(
			InMessage.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	bResultSent = false;
	FadeAlpha = 0.0f;
	FadeState = EFadeState::FadingIn;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Visible);
}

bool UBalhwajeomConfirmPromptWidget::CanAcceptInput() const
{
	return FadeState == EFadeState::Shown && !bResultSent;
}

void UBalhwajeomConfirmPromptWidget::BeginFadeOut()
{
	if (FadeState == EFadeState::FadingOut || FadeState == EFadeState::Hidden)
	{
		return;
	}
	FadeState = EFadeState::FadingOut;
}

void UBalhwajeomConfirmPromptWidget::HandleConfirmClicked()
{
	// Guarded rather than trusting the button: a click landing during the fade in would
	// teleport the player before they had seen what they were agreeing to.
	if (!CanAcceptInput())
	{
		return;
	}
	bResultSent = true;
	OnConfirmed.Broadcast();
}

void UBalhwajeomConfirmPromptWidget::HandleCancelClicked()
{
	if (!CanAcceptInput())
	{
		return;
	}
	bResultSent = true;
	OnCancelled.Broadcast();
}

void UBalhwajeomConfirmPromptWidget::NativeTick(
	const FGeometry& MyGeometry,
	const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	switch (FadeState)
	{
	case EFadeState::FadingIn:
		FadeAlpha = FadeInSeconds > 0.0f
			? FMath::Min(FadeAlpha + InDeltaTime / FadeInSeconds, 1.0f)
			: 1.0f;
		SetRenderOpacity(FadeAlpha);
		if (FadeAlpha >= 1.0f)
		{
			FadeState = EFadeState::Shown;
		}
		break;

	case EFadeState::FadingOut:
		FadeAlpha = FadeOutSeconds > 0.0f
			? FMath::Max(FadeAlpha - InDeltaTime / FadeOutSeconds, 0.0f)
			: 0.0f;
		SetRenderOpacity(FadeAlpha);
		if (FadeAlpha <= 0.0f)
		{
			FadeState = EFadeState::Hidden;
			SetVisibility(ESlateVisibility::Collapsed);
			OnFadeOutFinished.Broadcast();
		}
		break;

	default:
		break;
	}
}
