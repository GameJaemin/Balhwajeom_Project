#include "UI/BalhwajeomScreenFadeWidget.h"

void UBalhwajeomScreenFadeWidget::SetBlackImmediately()
{
	bFading = false;
	SetRenderOpacity(1.0f);
}

void UBalhwajeomScreenFadeWidget::FadeToBlack(float Duration)
{
	BeginFade(1.0f, Duration);
}

void UBalhwajeomScreenFadeWidget::FadeFromBlack(float Duration)
{
	BeginFade(0.0f, Duration);
}

void UBalhwajeomScreenFadeWidget::BeginFade(float TargetOpacity, float Duration)
{
	FadeStartOpacity = GetRenderOpacity();
	FadeTargetOpacity = TargetOpacity;
	FadeDuration = FMath::Max(0.0f, Duration);
	FadeElapsed = 0.0f;
	bFading = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (FadeDuration <= KINDA_SMALL_NUMBER)
	{
		NativeTick(FGeometry(), 0.0f);
	}
}

void UBalhwajeomScreenFadeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bFading)
	{
		return;
	}

	FadeElapsed += InDeltaTime;
	const float Alpha = FadeDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::Clamp(FadeElapsed / FadeDuration, 0.0f, 1.0f);
	SetRenderOpacity(FMath::Lerp(FadeStartOpacity, FadeTargetOpacity, Alpha));
	if (Alpha < 1.0f)
	{
		return;
	}

	bFading = false;
	if (FadeTargetOpacity >= 0.5f)
	{
		OnFadeToBlackFinished.Broadcast();
	}
	else
	{
		OnFadeFromBlackFinished.Broadcast();
	}
}
