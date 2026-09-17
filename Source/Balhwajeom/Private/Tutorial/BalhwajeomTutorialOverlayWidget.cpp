#include "Tutorial/BalhwajeomTutorialOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CameraSystem/BalhwajeomCapturePhotoDismissInput.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "Tutorial/BalhwajeomTutorialOverlayLayout.h"
#include "Tutorial/TutorialOverlayDefinitions.h"


UBalhwajeomTutorialOverlayWidget::UBalhwajeomTutorialOverlayWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}


float UBalhwajeomTutorialOverlayWidget::ComputeBlinkOpacity(
	const float Elapsed,
	const float Period,
	const float MinOpacity,
	const float MaxOpacity)
{
	if (Period <= KINDA_SMALL_NUMBER)
	{
		return MaxOpacity;
	}

	// Elapsed measured in half-periods, folded into 0..2 and mirrored, gives 0->1->0 linearly.
	const float HalfPeriods = FMath::Fmod(FMath::Max(Elapsed, 0.0f) / (Period * 0.5f), 2.0f);
	const float Alpha = 1.0f - FMath::Abs(HalfPeriods - 1.0f);
	return FMath::Lerp(MinOpacity, MaxOpacity, Alpha);
}


FVector2D UBalhwajeomTutorialOverlayWidget::ComputeUniformImageSize(
	const FIntPoint& SourceSize,
	const float TargetHeight)
{
	if (SourceSize.X <= 0 || SourceSize.Y <= 0 || TargetHeight <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}

	const float Width =
		static_cast<float>(SourceSize.X) * TargetHeight / static_cast<float>(SourceSize.Y);
	return FVector2D(Width, TargetHeight);
}


void UBalhwajeomTutorialOverlayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureFallbackLayout();
}


void UBalhwajeomTutorialOverlayWidget::EnsureFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	BalhwajeomTutorialOverlayLayout::FBoundWidgets Widgets;
	if (!BalhwajeomTutorialOverlayLayout::Build(*WidgetTree, Widgets))
	{
		return;
	}

	SCB_Images = Widgets.ImageScale;
	HB_Images = Widgets.ImageRow;
	TXT_Title = Widgets.Title;
	SB_Description = Widgets.DescriptionBox;
	TXT_Description = Widgets.Description;
	TXT_ContinuePrompt = Widgets.ContinuePrompt;
}


void UBalhwajeomTutorialOverlayWidget::Present(const FTutorialOverlayDefinition& Definition)
{
	EnsureFallbackLayout();

	CompletionTag = Definition.CompletionTag;
	RebuildImageRow(Definition.Images);
	ApplyTitle(Definition.OverlayTitle);
	ApplyDescription(Definition.OverlayText);
	ApplyContinuePrompt();

	VisibleSeconds = 0.0f;
	BlinkElapsed = 0.0f;
	SetKeyboardFocus();
}


void UBalhwajeomTutorialOverlayWidget::BeginFadeIn()
{
	DelayRemaining = FMath::Max(ShowDelaySeconds, 0.0f);
	FadeAlpha = 0.0f;
	PromptAlpha = 0.0f;
	VisibleSeconds = 0.0f;
	BlinkElapsed = 0.0f;
	SetRenderOpacity(0.0f);
	if (TXT_ContinuePrompt)
	{
		TXT_ContinuePrompt->SetRenderOpacity(0.0f);
	}

	if (DelayRemaining > 0.0f)
	{
		FadeState = EFadeState::Delayed;
		return;
	}

	FadeState = FadeInSeconds > 0.0f ? EFadeState::FadingIn : EFadeState::Shown;
	if (FadeState == EFadeState::Shown)
	{
		FadeAlpha = 1.0f;
		SetRenderOpacity(1.0f);
	}
}


void UBalhwajeomTutorialOverlayWidget::BeginFadeOut()
{
	if (FadeState == EFadeState::FadingOut)
	{
		return;
	}

	if (FadeOutSeconds <= 0.0f)
	{
		FadeState = EFadeState::Hidden;
		FadeAlpha = 0.0f;
		SetRenderOpacity(0.0f);
		OnFadeOutFinished.Broadcast();
		return;
	}

	FadeState = EFadeState::FadingOut;
}


void UBalhwajeomTutorialOverlayWidget::SetInputLockSeconds(const float Seconds)
{
	MinimumVisibleSeconds = FMath::Max(Seconds, 0.0f);
}


void UBalhwajeomTutorialOverlayWidget::RebuildImageRow(
	const TArray<TSoftObjectPtr<UTexture2D>>& Images)
{
	if (!HB_Images || !WidgetTree)
	{
		return;
	}

	HB_Images->ClearChildren();

	int32 AddedCount = 0;
	for (const TSoftObjectPtr<UTexture2D>& SoftTexture : Images)
	{
		if (SoftTexture.IsNull())
		{
			continue;
		}

		UTexture2D* Texture = SoftTexture.LoadSynchronous();
		if (!Texture)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Tutorial overlay image failed to load and was skipped: %s."),
				*SoftTexture.ToString());
			continue;
		}

		const FIntPoint SourceSize(Texture->GetSizeX(), Texture->GetSizeY());
		const FVector2D DrawSize = ComputeUniformImageSize(SourceSize, ImageHeight);
		if (DrawSize.IsNearlyZero())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Tutorial overlay image has no usable dimensions and was skipped: %s."),
				*Texture->GetPathName());
			continue;
		}

		// The SizeBox is what actually guarantees the shared height; the brush size only
		// supplies the matching width so the aspect ratio survives.
		USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Frame->SetHeightOverride(ImageHeight);

		UImage* Picture = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Picture->SetBrushFromTexture(Texture);
		FSlateBrush Brush = Picture->GetBrush();
		Brush.ImageSize = DrawSize;
		Picture->SetBrush(Brush);
		Frame->SetContent(Picture);

		if (UHorizontalBoxSlot* FrameSlot = HB_Images->AddChildToHorizontalBox(Frame))
		{
			// Auto size, never Fill: filling would divide the row evenly and squash the
			// aspect ratio the brush size just established.
			FrameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			FrameSlot->SetPadding(FMargin(ImageSpacing * 0.5f, 0.0f));
			FrameSlot->SetHorizontalAlignment(HAlign_Center);
			FrameSlot->SetVerticalAlignment(VAlign_Center);
		}
		++AddedCount;
	}

	const ESlateVisibility RowVisibility = AddedCount > 0
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	if (SCB_Images)
	{
		SCB_Images->SetVisibility(RowVisibility);
	}
	else
	{
		HB_Images->SetVisibility(RowVisibility);
	}
}


void UBalhwajeomTutorialOverlayWidget::ApplyTitle(const FText& OverlayTitle)
{
	if (!TXT_Title)
	{
		return;
	}

	TXT_Title->SetText(OverlayTitle);
	TXT_Title->SetVisibility(OverlayTitle.IsEmptyOrWhitespace()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible);
}


void UBalhwajeomTutorialOverlayWidget::ApplyDescription(const FText& OverlayText)
{
	if (!TXT_Description)
	{
		return;
	}

	TXT_Description->SetText(OverlayText);

	// Restated here so an edit in the Widget Blueprint cannot quietly centre the prose or
	// turn wrapping off. The block stays left aligned; SB_Description centres the block.
	TXT_Description->SetJustification(ETextJustify::Left);
	TXT_Description->SetAutoWrapText(true);

	const ESlateVisibility TextVisibility = OverlayText.IsEmptyOrWhitespace()
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	if (SB_Description)
	{
		SB_Description->SetVisibility(TextVisibility);
	}
	else
	{
		TXT_Description->SetVisibility(TextVisibility);
	}
}


void UBalhwajeomTutorialOverlayWidget::ApplyContinuePrompt()
{
	if (!TXT_ContinuePrompt)
	{
		return;
	}

	if (!ContinuePromptText.IsEmpty())
	{
		TXT_ContinuePrompt->SetText(ContinuePromptText);
	}
	TXT_ContinuePrompt->SetRenderOpacity(0.0f);
}


bool UBalhwajeomTutorialOverlayWidget::CanAcceptDismissInput() const
{
	// Already on the way out: a second press must not queue a second close.
	return FadeState != EFadeState::FadingOut && VisibleSeconds >= MinimumVisibleSeconds;
}


void UBalhwajeomTutorialOverlayWidget::NativeTick(
	const FGeometry& MyGeometry,
	const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickFade(InDeltaTime);

	// The lock runs from the start of the fade in, not from the blank beat before it, so
	// the 2 seconds the player waits are 2 seconds of actually being able to read.
	if (FadeState != EFadeState::Delayed)
	{
		VisibleSeconds += InDeltaTime;
	}
	TickPrompt(InDeltaTime);
}


void UBalhwajeomTutorialOverlayWidget::TickFade(const float InDeltaTime)
{
	if (FadeState == EFadeState::Delayed)
	{
		DelayRemaining -= InDeltaTime;
		if (DelayRemaining > 0.0f)
		{
			return;
		}

		DelayRemaining = 0.0f;
		FadeState = FadeInSeconds > 0.0f ? EFadeState::FadingIn : EFadeState::Shown;
		if (FadeState == EFadeState::Shown)
		{
			FadeAlpha = 1.0f;
			SetRenderOpacity(1.0f);
		}
		return;
	}

	if (FadeState == EFadeState::FadingIn)
	{
		FadeAlpha = FMath::Clamp(FadeAlpha + InDeltaTime / FMath::Max(FadeInSeconds, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		SetRenderOpacity(FadeAlpha);
		if (FadeAlpha >= 1.0f)
		{
			FadeState = EFadeState::Shown;
		}
		return;
	}

	if (FadeState == EFadeState::FadingOut)
	{
		FadeAlpha = FMath::Clamp(FadeAlpha - InDeltaTime / FMath::Max(FadeOutSeconds, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		SetRenderOpacity(FadeAlpha);
		if (FadeAlpha <= 0.0f)
		{
			FadeState = EFadeState::Hidden;
			OnFadeOutFinished.Broadcast();
		}
	}
}


void UBalhwajeomTutorialOverlayWidget::TickPrompt(const float InDeltaTime)
{
	if (!TXT_ContinuePrompt)
	{
		return;
	}

	// "Press any key" must not be on screen while any key is being swallowed, so the
	// prompt only appears once the lock has expired.
	if (!CanAcceptDismissInput())
	{
		PromptAlpha = 0.0f;
		TXT_ContinuePrompt->SetRenderOpacity(0.0f);
		return;
	}

	PromptAlpha = FMath::Clamp(
		PromptAlpha + InDeltaTime / FMath::Max(PromptRevealSeconds, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f);
	BlinkElapsed += InDeltaTime;
	TXT_ContinuePrompt->SetRenderOpacity(PromptAlpha * ComputeBlinkOpacity(
		BlinkElapsed, PromptBlinkPeriod, PromptMinOpacity, PromptMaxOpacity));
}


FReply UBalhwajeomTutorialOverlayWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	// Same policy as the capture result card, so "any key" means the same thing in both
	// places: no repeats, no analog axes, no modifiers, no function or console keys.
	if (CanAcceptDismissInput() &&
		BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey(
			InKeyEvent.GetKey(), InKeyEvent.IsRepeat()))
	{
		RequestClose();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}


FReply UBalhwajeomTutorialOverlayWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	// Previewed so a click anywhere counts even when it lands on an image, which would
	// otherwise swallow the press and steal keyboard focus from the overlay.
	if (CanAcceptDismissInput() &&
		BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey(
			InMouseEvent.GetEffectingButton(), false))
	{
		RequestClose();
		return FReply::Handled();
	}

	SetKeyboardFocus();
	return FReply::Handled();
}


void UBalhwajeomTutorialOverlayWidget::RequestClose()
{
	OnCloseRequested.Broadcast();
}
