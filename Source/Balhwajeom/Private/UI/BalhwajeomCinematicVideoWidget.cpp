#include "UI/BalhwajeomCinematicVideoWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Widget.h"
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

void UBalhwajeomCinematicVideoWidget::SetBackgroundVisible(bool bVisible)
{
	if (!VideoBackground)
	{
		return;
	}
	// HitTestInvisible rather than Visible when shown: this is a backing plate, never a control.
	VideoBackground->SetVisibility(
		bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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

void UBalhwajeomCinematicVideoWidget::FadeInWhenMediaReady(
	UMediaTexture* MediaTexture, float Duration, float MaxWaitSeconds)
{
	bFadingIn = false;
	SetRenderOpacity(0.0f);
	if (!MediaTexture)
	{
		FadeIn(Duration);
		return;
	}
	// A texture that already has picture on it (e.g. a clip pre-opened and parked on frame 0) needs no
	// wait at all -- reveal it on this frame.
	if (MediaTexture->GetWidth() > 0)
	{
		FadeIn(Duration);
		return;
	}
	PendingRevealTexture = MediaTexture;
	PendingRevealDuration = Duration;
	PendingRevealMaxWait = FMath::Max(0.0f, MaxWaitSeconds);
	PendingRevealWaited = 0.0f;
	bWaitingForMediaFrame = true;
}

void UBalhwajeomCinematicVideoWidget::FadeIn(float Duration)
{
	bWaitingForMediaFrame = false;
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
	if (bWaitingForMediaFrame)
	{
		PendingRevealWaited += InDeltaTime;
		const UMediaTexture* RevealTexture = PendingRevealTexture.Get();
		// GetWidth() stays 0 until the player has handed a real frame to the texture, which is the
		// only reliable "there is picture to show" signal -- OnMediaOpened fires well before it.
		if (!RevealTexture || RevealTexture->GetWidth() > 0 || PendingRevealWaited >= PendingRevealMaxWait)
		{
			PendingRevealTexture.Reset();
			FadeIn(PendingRevealDuration);
		}
		return;
	}
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
