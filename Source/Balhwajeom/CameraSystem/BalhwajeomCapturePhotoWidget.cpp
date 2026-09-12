#include "BalhwajeomCapturePhotoWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

void UBalhwajeomCapturePhotoWidget::PresentCapture(
	UTexture2D* CapturedTexture,
	const FText& SentenceText,
	const TArray<FText>& GrantedKeywords)
{
	if (!CardBackground)
	{
		CardBackground = Cast<UBorder>(GetWidgetFromName(TEXT("CardBackground")));
	}
	if (!SentenceBackground)
	{
		SentenceBackground = Cast<UBorder>(GetWidgetFromName(TEXT("SentenceBackground")));
	}
	TabFlyTarget = GetWidgetFromName(TEXT("TabFlyTarget"));
	ResetPresentation();

	if (CapturedPhotoImage)
	{
		CapturedPhotoImage->SetBrushFromTexture(CapturedTexture, false);
	}
	if (SentenceTextBlock)
	{
		SentenceTextBlock->SetText(SentenceText);
		SentenceTextBlock->SetVisibility(
			SentenceText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (KeywordList)
	{
		KeywordList->ClearChildren();
		for (const FText& Keyword : GrantedKeywords)
		{
			UBorder* Pill = NewObject<UBorder>(KeywordList);
			Pill->SetBrushColor(KeywordBackgroundColor);
			Pill->SetPadding(FMargin(20.0f, 10.0f));

			UTextBlock* Label = NewObject<UTextBlock>(Pill);
			Label->SetText(Keyword);
			Label->SetColorAndOpacity(FSlateColor(KeywordTextColor));
			Label->SetJustification(ETextJustify::Center);
			FSlateFontInfo KeywordFont = Label->GetFont();
			if (KeywordFontAsset)
			{
				KeywordFont.FontObject = KeywordFontAsset.Get();
			}
			KeywordFont.Size = KeywordFontSize;
			Label->SetFont(KeywordFont);
			Pill->SetContent(Label);

			if (UVerticalBoxSlot* KeywordSlot = KeywordList->AddChildToVerticalBox(Pill))
			{
				KeywordSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
				KeywordSlot->SetHorizontalAlignment(HAlign_Fill);
			}
		}
		KeywordList->SetVisibility(
			GrantedKeywords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	bHasGrantedKeywords = !GrantedKeywords.IsEmpty();
	bAnimationOriginsCached = false;

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

float UBalhwajeomCapturePhotoWidget::GetFlyDuration() const
{
	const float PhotoDuration = FMath::Max(FlyDuration, KINDA_SMALL_NUMBER);
	return bHasGrantedKeywords
		? PhotoDuration + FMath::Max(KeywordFollowDelay, 0.0f) +
			FMath::Max(KeywordFlyDuration, KINDA_SMALL_NUMBER)
		: PhotoDuration;
}

void UBalhwajeomCapturePhotoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bAnimationOriginsCached || !TabFlyTarget || !CapturedPhotoImage)
	{
		return;
	}
	// SetRenderTransform resets do not update cached geometry synchronously.
	// Allow Slate to arrange/paint the reset widgets, including rebuilt keywords.
	if (LayoutTicksRemaining-- > 0)
	{
		return;
	}
	const FGeometry TargetGeometry = TabFlyTarget->GetCachedGeometry();
	const FGeometry PhotoGeometry = CapturedPhotoImage->GetCachedGeometry();
	if (TabFlyTarget->GetVisibility() == ESlateVisibility::Collapsed ||
		TargetGeometry.GetLocalSize().IsNearlyZero() ||
		PhotoGeometry.GetLocalSize().IsNearlyZero())
	{
		return;
	}
	const FVector2D AbsoluteTarget = TargetGeometry.LocalToAbsolute(
		TargetGeometry.GetLocalSize() * 0.5f);
	// Render translations are expressed in each animated widget's LOCAL units.
	// AbsoluteToLocal includes DPI, ScaleBoxes and all ancestor transforms.
	PhotoLocalTravel = PhotoGeometry.AbsoluteToLocal(AbsoluteTarget) -
		PhotoGeometry.GetLocalSize() * 0.5f;
	if (bHasGrantedKeywords)
	{
		if (!KeywordList || KeywordList->GetCachedGeometry().GetLocalSize().IsNearlyZero())
		{
			return;
		}
		const FGeometry KeywordGeometry = KeywordList->GetCachedGeometry();
		KeywordLocalTravel = KeywordGeometry.AbsoluteToLocal(AbsoluteTarget) -
			KeywordGeometry.GetLocalSize() * 0.5f;
	}
	bAnimationOriginsCached = true;
}

void UBalhwajeomCapturePhotoWidget::ApplyFlyToTab(const float LinearAlpha)
{
	if (!bAnimationOriginsCached)
	{
		return;
	}

	const float TotalFlyDuration = GetFlyDuration();
	const float Elapsed = FMath::Clamp(LinearAlpha, 0.0f, 1.0f) * TotalFlyDuration;
	const float PhotoAlpha = FMath::Clamp(
		Elapsed / FMath::Max(FlyDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	ApplyWidgetFly(CapturedPhotoImage, PhotoLocalTravel, PhotoAlpha);

	if (SentenceBackground)
	{
		SentenceBackground->SetRenderOpacity(1.0f - PhotoAlpha);
	}
	if (CardBackground)
	{
		CardBackground->SetRenderOpacity(1.0f - PhotoAlpha);
	}

	if (ScreenDimmer)
	{
		ScreenDimmer->SetRenderOpacity(1.0f - PhotoAlpha);
	}

	if (KeywordList && bHasGrantedKeywords)
	{
		const float KeywordStartTime =
			FMath::Max(FlyDuration, KINDA_SMALL_NUMBER) + FMath::Max(KeywordFollowDelay, 0.0f);
		const float KeywordAlpha = FMath::Clamp(
			(Elapsed - KeywordStartTime) /
			FMath::Max(KeywordFlyDuration, KINDA_SMALL_NUMBER),
			0.0f,
			1.0f);
		ApplyWidgetFly(KeywordList, KeywordLocalTravel, KeywordAlpha);
	}
}

void UBalhwajeomCapturePhotoWidget::ApplyWidgetFly(
	UWidget* Widget,
	const FVector2D& LocalTravel,
	const float Alpha)
{
	if (!Widget)
	{
		return;
	}

	const float EasedAlpha = FMath::InterpEaseInOut(
		0.0f, 1.0f, FMath::Clamp(Alpha, 0.0f, 1.0f), 2.0f);
	Widget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform Transform;
	Transform.Translation = LocalTravel * EasedAlpha;
	Transform.Scale = FVector2D(FMath::Lerp(1.0f, TabTargetScale, EasedAlpha));
	Widget->SetRenderTransform(Transform);
	Widget->SetRenderOpacity(
		1.0f - FMath::Clamp((EasedAlpha - 0.82f) / 0.18f, 0.0f, 1.0f));
}

void UBalhwajeomCapturePhotoWidget::ResetPresentation()
{
	if (CardRoot)
	{
		CardRoot->SetRenderTransform(FWidgetTransform());
		CardRoot->SetRenderOpacity(1.0f);
	}
	if (CardBackground)
	{
		CardBackground->SetRenderTransform(FWidgetTransform());
		CardBackground->SetRenderOpacity(1.0f);
	}
	if (CapturedPhotoImage)
	{
		CapturedPhotoImage->SetRenderTransform(FWidgetTransform());
		CapturedPhotoImage->SetRenderOpacity(1.0f);
	}
	if (SentenceBackground)
	{
		SentenceBackground->SetRenderTransform(FWidgetTransform());
		SentenceBackground->SetRenderOpacity(1.0f);
	}
	if (KeywordList)
	{
		KeywordList->SetRenderTransform(FWidgetTransform());
		KeywordList->SetRenderOpacity(1.0f);
	}
	if (ScreenDimmer)
	{
		ScreenDimmer->SetRenderOpacity(1.0f);
	}
	bAnimationOriginsCached = false;
	LayoutTicksRemaining = 2;
}
