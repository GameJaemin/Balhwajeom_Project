#include "MultiShadowText.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"


UMultiShadowTextWidget::UMultiShadowTextWidget()
{
	Text = FText::FromString(TEXT("Multi Shadow Text"));

	Font.Size = 24;

	TextColor = FLinearColor::White;

	Justification = ETextJustify::Left;

	bAutoWrapText = false;

	WrapTextAt = 0.0f;


	// 기본 그림자 레이어 하나
	FMultiShadowLayer DefaultShadow;

	DefaultShadow.bEnabled = true;

	DefaultShadow.Offset = FVector2D::ZeroVector;

	DefaultShadow.Blur = 4.0f;

	DefaultShadow.Spread = 0.0f;

	DefaultShadow.Color = FLinearColor::Black;

	DefaultShadow.Opacity = 0.62f;

	ShadowLayers.Add(DefaultShadow);
}


TSharedRef<SWidget> UMultiShadowTextWidget::RebuildWidget()
{
	MyOverlay = SNew(SOverlay);

	RebuildShadowWidgets();

	return MyOverlay.ToSharedRef();
}


void UMultiShadowTextWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	RebuildShadowWidgets();
}


void UMultiShadowTextWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyOverlay.Reset();
}


void UMultiShadowTextWidget::RebuildShadowWidgets()
{
	if (!MyOverlay.IsValid())
	{
		return;
	}


	MyOverlay->ClearChildren();


	// ============================================================
	// SHADOW LAYERS
	// ============================================================

	for (const FMultiShadowLayer& Layer : ShadowLayers)
	{
		if (!Layer.bEnabled)
		{
			continue;
		}


		const float BlurRadius =
			FMath::Max(0.0f, Layer.Blur);


		const float SpreadRadius =
			FMath::Max(0.0f, Layer.Spread);


		// ========================================================
		// BLUR = 0
		// 일반적인 선명한 그림자
		// ========================================================

		if (BlurRadius <= KINDA_SMALL_NUMBER)
		{
			FLinearColor ShadowColor = Layer.Color;

			ShadowColor.A *= Layer.Opacity;


			MyOverlay->AddSlot()
			[
				SNew(STextBlock)

				.Text(Text)

				.Font(Font)

				.ColorAndOpacity(
					FSlateColor(ShadowColor)
				)

				.Justification(
					Justification.GetValue()
				)

				.AutoWrapText(
					bAutoWrapText
				)

				.WrapTextAt(
					WrapTextAt
				)

				.RenderTransform(
					FSlateRenderTransform(
						FVector2f(
							static_cast<float>(Layer.Offset.X),
							static_cast<float>(Layer.Offset.Y)
						)
					)
				)
			];


			continue;
		}


		// ========================================================
		// BLUR
		// ========================================================

		const int32 RingCount =
			FMath::Clamp(
				FMath::CeilToInt(
					BlurRadius / 1.5f
				),
				2,
				5
			);


		constexpr int32 SamplesPerRing = 12;


		// --------------------------------------------------------
		// 중심부
		// --------------------------------------------------------

		{
			FLinearColor CenterColor = Layer.Color;

			CenterColor.A *=
				Layer.Opacity * 0.18f;


			MyOverlay->AddSlot()
			[
				SNew(STextBlock)

				.Text(Text)

				.Font(Font)

				.ColorAndOpacity(
					FSlateColor(CenterColor)
				)

				.Justification(
					Justification.GetValue()
				)

				.AutoWrapText(
					bAutoWrapText
				)

				.WrapTextAt(
					WrapTextAt
				)

				.RenderTransform(
					FSlateRenderTransform(
						FVector2f(
							static_cast<float>(Layer.Offset.X),
							static_cast<float>(Layer.Offset.Y)
						)
					)
				)
			];
		}


		// --------------------------------------------------------
		// 여러 개의 원형 샘플
		// --------------------------------------------------------

		for (
			int32 RingIndex = 0;
			RingIndex < RingCount;
			++RingIndex
		)
		{
			const float RingT =
				static_cast<float>(RingIndex + 1) /
				static_cast<float>(RingCount);


			const float Radius =
				SpreadRadius +
				(BlurRadius * RingT);


			const float Falloff =
				FMath::Pow(
					1.0f - RingT,
					1.35f
				);


			const float RingOpacity =
				FMath::Max(
					Falloff,
					0.08f
				);


			for (
				int32 SampleIndex = 0;
				SampleIndex < SamplesPerRing;
				++SampleIndex
			)
			{
				const float Angle =
					2.0f *
					PI *
					(
						static_cast<float>(SampleIndex) /
						static_cast<float>(SamplesPerRing)
					);


				const float X =
					FMath::Cos(Angle) *
					Radius;


				const float Y =
					FMath::Sin(Angle) *
					Radius;


				const FVector2D FinalOffset =
					Layer.Offset +
					FVector2D(X, Y);


				FLinearColor ShadowColor =
					Layer.Color;


				ShadowColor.A *=
					Layer.Opacity *
					RingOpacity *
					(
						1.0f /
						static_cast<float>(SamplesPerRing)
					);


				MyOverlay->AddSlot()
				[
					SNew(STextBlock)

					.Text(Text)

					.Font(Font)

					.ColorAndOpacity(
						FSlateColor(ShadowColor)
					)

					.Justification(
						Justification.GetValue()
					)

					.AutoWrapText(
						bAutoWrapText
					)

					.WrapTextAt(
						WrapTextAt
					)

					.RenderTransform(
						FSlateRenderTransform(
							FVector2f(
								static_cast<float>(FinalOffset.X),
								static_cast<float>(FinalOffset.Y)
							)
						)
					)
				];
			}
		}
	}


	// ============================================================
	// MAIN TEXT
	// ============================================================

	MyOverlay->AddSlot()
	[
		SNew(STextBlock)

		.Text(Text)

		.Font(Font)

		.ColorAndOpacity(
			FSlateColor(TextColor)
		)

		.Justification(
			Justification.GetValue()
		)

		.AutoWrapText(
			bAutoWrapText
		)

		.WrapTextAt(
			WrapTextAt
		)
	];
}


#if WITH_EDITOR

const FText UMultiShadowTextWidget::GetPaletteCategory()
{
	return FText::FromString(
		TEXT("Custom UI")
	);
}

#endif