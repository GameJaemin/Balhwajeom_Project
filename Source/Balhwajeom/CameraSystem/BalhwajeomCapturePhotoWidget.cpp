#include "BalhwajeomCapturePhotoWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

namespace
{
	float EvaluateUnitCubicBezier(
		const float Parameter,
		const float FirstControl,
		const float SecondControl)
	{
		const float Inverse = 1.0f - Parameter;
		return
			3.0f * Inverse * Inverse * Parameter * FirstControl +
			3.0f * Inverse * Parameter * Parameter * SecondControl +
			Parameter * Parameter * Parameter;
	}

	float EvaluateAeZeroSpeedTemporalEase(
		const float LinearAlpha,
		const float OutgoingInfluencePercent,
		const float IncomingInfluencePercent)
	{
		const float TimeAlpha = FMath::Clamp(LinearAlpha, 0.0f, 1.0f);
		if (TimeAlpha <= 0.0f || TimeAlpha >= 1.0f)
		{
			return TimeAlpha;
		}

		const float OutgoingControlX = FMath::Clamp(
			OutgoingInfluencePercent * 0.01f, 0.0f, 1.0f);
		const float IncomingControlX = 1.0f - FMath::Clamp(
			IncomingInfluencePercent * 0.01f, 0.0f, 1.0f);
		float LowerParameter = 0.0f;
		float UpperParameter = 1.0f;
		for (int32 Iteration = 0; Iteration < 24; ++Iteration)
		{
			const float Parameter = (LowerParameter + UpperParameter) * 0.5f;
			const float BezierTime = EvaluateUnitCubicBezier(
				Parameter, OutgoingControlX, IncomingControlX);
			if (BezierTime < TimeAlpha)
			{
				LowerParameter = Parameter;
			}
			else
			{
				UpperParameter = Parameter;
			}
		}

		const float Parameter = (LowerParameter + UpperParameter) * 0.5f;
		return EvaluateUnitCubicBezier(Parameter, 0.0f, 1.0f);
	}

	float EvaluateAeEntryEase(const float LinearAlpha)
	{
		// Source AE entry: outgoing speed 0/influence 0.01%,
		// incoming speed 0/influence 100%.
		return EvaluateAeZeroSpeedTemporalEase(LinearAlpha, 0.01f, 100.0f);
	}

	float EvaluateAeExitPositionEase(const float LinearAlpha)
	{
		// Requested AE exit position: outgoing speed 0/influence 88%,
		// incoming speed 0/influence 10%.
		return EvaluateAeZeroSpeedTemporalEase(LinearAlpha, 88.0f, 10.0f);
	}
}

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
			if (KeywordBackgroundTexture)
			{
				Pill->SetBrushFromTexture(KeywordBackgroundTexture);
			}
			else
			{
				Pill->SetBrushColor(KeywordBackgroundColor);
			}
			Pill->SetPadding(FMargin(30.0f, 20.0f));

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
	bPresentationReady =
		CardRoot != nullptr &&
		CardComposite != nullptr &&
		ScreenDimmer != nullptr &&
		(!bHasGrantedKeywords || KeywordList != nullptr);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyPresentationTimeline(0.0f);
	if (CardComposite)
	{
		CardComposite->RequestRender();
	}
}

float UBalhwajeomCapturePhotoWidget::GetAnimationDuration() const
{
	return FMath::Max(AnimationDuration, KINDA_SMALL_NUMBER);
}

void UBalhwajeomCapturePhotoWidget::ApplyPresentationTimeline(const float LinearAlpha)
{
	if (!bPresentationReady)
	{
		return;
	}

	const float Duration = GetAnimationDuration();
	const float Elapsed = FMath::Clamp(LinearAlpha, 0.0f, 1.0f) * Duration;
	const float CardEntryAlpha = FMath::Clamp(
		Elapsed / FMath::Max(CardEntryDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	ApplyEntryTransform(CardComposite, CardEntryAlpha);
	if (Elapsed < ExitStartTime)
	{
		UpdateResolvedExitDistance();
	}

	const float ExitAlpha = FMath::Clamp(
		(Elapsed - ExitStartTime) /
		FMath::Max(Duration - ExitStartTime, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f);
	if (CardRoot)
	{
		const float EasedExitAlpha = EvaluateAeExitPositionEase(ExitAlpha);
		FWidgetTransform RootTransform;
		RootTransform.Translation =
			FVector2D(ExitOffset.X, ResolvedExitDistanceY) * EasedExitAlpha;
		RootTransform.Scale = FVector2D(1.0f, 1.0f);
		CardRoot->SetRenderTransform(RootTransform);
	}

	const float FadeOutAlpha = FMath::Clamp(
		(Elapsed - FadeOutStartTime) /
		FMath::Max(Duration - FadeOutStartTime, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f);
	const float DimmerOpacity = 1.0f - FadeOutAlpha;
	if (CardComposite)
	{
		CardComposite->SetRenderOpacity(1.0f);
	}
	if (ScreenDimmer)
	{
		// The dimmer keeps its authored position and retains the final fade independently.
		ScreenDimmer->SetRenderOpacity(DimmerOpacity);
	}

	if (KeywordList && bHasGrantedKeywords)
	{
		for (int32 KeywordIndex = 0; KeywordIndex < KeywordList->GetChildrenCount(); ++KeywordIndex)
		{
			UWidget* Keyword = KeywordList->GetChildAt(KeywordIndex);
			const float EntryStart = FirstKeywordDelay + KeywordStagger * KeywordIndex;
			const float KeywordEntryAlpha = FMath::Clamp(
				(Elapsed - EntryStart) /
				FMath::Max(KeywordEntryDuration, KINDA_SMALL_NUMBER),
				0.0f,
				1.0f);
			ApplyEntryTransform(Keyword, KeywordEntryAlpha);

			const float FadeInAlpha = FMath::Clamp(
				(Elapsed - EntryStart) /
				FMath::Max(KeywordFadeInDuration, KINDA_SMALL_NUMBER),
				0.0f,
				1.0f);
			Keyword->SetRenderOpacity(FadeInAlpha);
		}
	}
}

float UBalhwajeomCapturePhotoWidget::CalculateExitDistance(
	const float ViewportHeight,
	const float ContentTop,
	const float MinimumDistance,
	const float SafetyMargin)
{
	const float DistancePastBottom =
		ViewportHeight - ContentTop + FMath::Max(SafetyMargin, 0.0f);
	return FMath::Max(FMath::Max(MinimumDistance, 0.0f), DistancePastBottom);
}

void UBalhwajeomCapturePhotoWidget::UpdateResolvedExitDistance()
{
	const FGeometry& ViewportGeometry = GetCachedGeometry();
	const float ViewportHeight = ViewportGeometry.GetLocalSize().Y;
	if (ViewportHeight <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	float ContentTop = TNumericLimits<float>::Max();
	auto AccumulateContentTop = [&ViewportGeometry, &ContentTop](const UWidget* Content)
	{
		if (!Content || !Content->IsVisible())
		{
			return;
		}

		const FGeometry& ContentGeometry = Content->GetCachedGeometry();
		if (ContentGeometry.GetLocalSize().IsNearlyZero())
		{
			return;
		}

		const float LocalTop = ViewportGeometry.AbsoluteToLocal(
			ContentGeometry.GetAbsolutePosition()).Y;
		if (FMath::IsFinite(LocalTop))
		{
			ContentTop = FMath::Min(ContentTop, LocalTop);
		}
	};

	AccumulateContentTop(CardComposite);
	if (bHasGrantedKeywords)
	{
		AccumulateContentTop(KeywordList);
	}
	if (ContentTop < TNumericLimits<float>::Max())
	{
		ResolvedExitDistanceY = CalculateExitDistance(
			ViewportHeight,
			ContentTop,
			ExitOffset.Y,
			ExitSafetyMargin);
	}
}

void UBalhwajeomCapturePhotoWidget::ApplyEntryTransform(UWidget* Widget, const float EntryAlpha)
{
	if (!Widget)
	{
		return;
	}

	const float EasedAlpha = EvaluateAeEntryEase(EntryAlpha);
	Widget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FWidgetTransform Transform;
	Transform.Translation = EntryOffset * (1.0f - EasedAlpha);
	Transform.Scale = FVector2D(1.0f, 1.0f);
	Transform.Angle = EntryRotation * (1.0f - EasedAlpha);
	Widget->SetRenderTransform(Transform);
}

void UBalhwajeomCapturePhotoWidget::ResetPresentation()
{
	ResolvedExitDistanceY = FMath::Max(ExitOffset.Y, 0.0f);
	if (CardRoot)
	{
		CardRoot->SetRenderTransform(FWidgetTransform());
		CardRoot->SetRenderOpacity(1.0f);
	}
	if (CardComposite)
	{
		CardComposite->SetRenderTransform(FWidgetTransform());
		CardComposite->SetRenderOpacity(1.0f);
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
		for (int32 KeywordIndex = 0; KeywordIndex < KeywordList->GetChildrenCount(); ++KeywordIndex)
		{
			if (UWidget* Keyword = KeywordList->GetChildAt(KeywordIndex))
			{
				Keyword->SetRenderTransform(FWidgetTransform());
				Keyword->SetRenderOpacity(1.0f);
			}
		}
	}
	if (ScreenDimmer)
	{
		ScreenDimmer->SetRenderTransform(FWidgetTransform());
		ScreenDimmer->SetRenderOpacity(1.0f);
	}
	bPresentationReady = false;
}
