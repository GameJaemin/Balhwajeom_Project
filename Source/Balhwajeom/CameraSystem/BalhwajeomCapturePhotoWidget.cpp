#include "BalhwajeomCapturePhotoWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

void UBalhwajeomCapturePhotoWidget::PresentCapture(
	UTexture2D* CapturedTexture,
	const FText& SentenceText,
	const TArray<FText>& GrantedKeywords,
	const bool bIsAnalysisSentence)
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

	if (!AnalysisBackgroundTexture)
	{
		AnalysisBackgroundTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/Balhwajeom/UI/Camera/photo_black_bg_v2.photo_black_bg_v2"));
	}
	if (!NaturalBackgroundTexture)
	{
		NaturalBackgroundTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/Balhwajeom/UI/Camera/photo_yellow_bg_v2.photo_yellow_bg_v2"));
	}
	if (CardBackground)
	{
		CardBackground->SetBrushFromTexture(
			bIsAnalysisSentence ? AnalysisBackgroundTexture.Get() : NaturalBackgroundTexture.Get());
		CardBackground->SetBrushColor(FLinearColor::White);
	}

	if (CapturedPhotoImage)
	{
		CapturedPhotoImage->SetBrushFromTexture(CapturedTexture, false);
		CapturedPhotoImage->SetColorAndOpacity(
			bIsAnalysisSentence ? AnalysisPhotoTint : NaturalPhotoTint);
	}
	if (SentenceTextBlock)
	{
		SentenceTextBlock->SetText(SentenceText);
		SentenceTextBlock->SetColorAndOpacity(FSlateColor(
			bIsAnalysisSentence ? FLinearColor::White : FLinearColor::Black));
		FSlateFontInfo SentenceFont = SentenceTextBlock->GetFont();
		SentenceFont.Size = bIsAnalysisSentence
			? AnalysisSentenceFontSize
			: NaturalSentenceFontSize;
		SentenceTextBlock->SetFont(SentenceFont);
		SentenceTextBlock->SetVisibility(
			SentenceText.IsEmpty() || bIsAnalysisSentence
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
	if (SentenceBuilder)
	{
		SentenceBuilder->ClearChildren();
		if (bIsAnalysisSentence && !SentenceText.IsEmpty())
		{
			TArray<FString> Segments;
			SentenceText.ToString().ParseIntoArray(Segments, TEXT("[]"), false);
			bool bForceNextChildToNewLine = false;
			auto AddSentenceChild = [this, &bForceNextChildToNewLine](UWidget* Child)
			{
				if (UWrapBoxSlot* Slot = SentenceBuilder->AddChildToWrapBox(Child))
				{
					Slot->SetNewLine(bForceNextChildToNewLine);
					Slot->SetVerticalAlignment(VAlign_Center);
				}
				bForceNextChildToNewLine = false;
			};
			auto AddEmptyLine = [this, &AddSentenceChild]()
			{
				USpacer* Spacer = NewObject<USpacer>(SentenceBuilder);
				Spacer->SetSize(FVector2D(1.0f, static_cast<float>(AnalysisSentenceFontSize)));
				AddSentenceChild(Spacer);
				if (UWrapBoxSlot* Slot = Cast<UWrapBoxSlot>(Spacer->Slot))
				{
					Slot->SetFillEmptySpace(true);
				}
			};

			for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
			{
				FString NormalizedSegment = Segments[SegmentIndex]
					.Replace(TEXT("\r\n"), TEXT("\n"))
					.Replace(TEXT("\r"), TEXT("\n"));
				TArray<FString> Lines;
				NormalizedSegment.ParseIntoArray(Lines, TEXT("\n"), false);
				for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
				{
					if (LineIndex > 0)
					{
						// A second unconsumed break represents an intentionally blank line.
						if (bForceNextChildToNewLine)
						{
							AddEmptyLine();
						}
						bForceNextChildToNewLine = true;
					}
					if (!Lines[LineIndex].IsEmpty())
					{
						UTextBlock* Segment = NewObject<UTextBlock>(SentenceBuilder);
						Segment->SetText(FText::FromString(Lines[LineIndex]));
						Segment->SetColorAndOpacity(FSlateColor(FLinearColor::White));
						FSlateFontInfo SegmentFont = SentenceTextBlock
							? SentenceTextBlock->GetFont()
							: Segment->GetFont();
						SegmentFont.Size = AnalysisSentenceFontSize;
						Segment->SetFont(SegmentFont);
						AddSentenceChild(Segment);
					}
				}

				if (SegmentIndex < Segments.Num() - 1)
				{
					UBorder* BlankBackground = NewObject<UBorder>(SentenceBuilder);
					BlankBackground->SetBrushColor(FLinearColor::White);
					USizeBox* BlankSize = NewObject<USizeBox>(SentenceBuilder);
					BlankSize->SetWidthOverride(83.0f);
					BlankSize->SetHeightOverride(36.0f);
					BlankSize->SetContent(BlankBackground);
					AddSentenceChild(BlankSize);
					if (UWrapBoxSlot* BlankSlot = Cast<UWrapBoxSlot>(BlankSize->Slot))
					{
						BlankSlot->SetPadding(FMargin(4.0f, 0.0f));
					}
				}
			}
		}
		SentenceBuilder->SetVisibility(
			bIsAnalysisSentence && !SentenceText.IsEmpty()
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
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
