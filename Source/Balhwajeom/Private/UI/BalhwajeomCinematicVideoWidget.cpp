#include "UI/BalhwajeomCinematicVideoWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "MediaTexture.h"

void UBalhwajeomCinematicVideoWidget::SetMediaTexture(UMediaTexture* MediaTexture)
{
	if (!IMG_Video || !MediaTexture)
	{
		return;
	}

	FSlateBrush Brush = IMG_Video->GetBrush();
	Brush.SetResourceObject(MediaTexture);
	Brush.ImageSize = FVector2D(1920.0f, 1080.0f);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	IMG_Video->SetBrush(Brush);
}

void UBalhwajeomCinematicVideoWidget::SetSkipEnabled(bool bEnabled)
{
	if (!BTN_Skip)
	{
		return;
	}

	BTN_Skip->SetIsEnabled(bEnabled);
	BTN_Skip->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UBalhwajeomCinematicVideoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BTN_Skip)
	{
		BTN_Skip->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSkipClicked);
	}
}

void UBalhwajeomCinematicVideoWidget::HandleSkipClicked()
{
	OnSkipRequested.Broadcast();
}

void UBalhwajeomCinematicVideoWidget::FadeIn(float Duration)
{
	FadeInStartOpacity = GetRenderOpacity();
	FadeInDuration = FMath::Max(0.0f, Duration);
	FadeInElapsed = 0.0f;
	bFadingIn = true;
	if (FadeInDuration <= KINDA_SMALL_NUMBER)
	{
		SetRenderOpacity(1.0f);
		bFadingIn = false;
	}
}

void UBalhwajeomCinematicVideoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bFadingIn)
	{
		return;
	}

	FadeInElapsed += InDeltaTime;
	const float Alpha = FadeInDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::Clamp(FadeInElapsed / FadeInDuration, 0.0f, 1.0f);
	SetRenderOpacity(FMath::Lerp(FadeInStartOpacity, 1.0f, Alpha));
	if (Alpha >= 1.0f)
	{
		bFadingIn = false;
	}
}
