#include "UI/BalhwajeomMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "Sound/SoundBase.h"

UBalhwajeomMainMenuWidget::UBalhwajeomMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ButtonClickSound = TSoftObjectPtr<USoundBase>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/Audio/SFX/Button.Button")));
}

void UBalhwajeomMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bStartAccepted = false;
	if (BTN_Start)
	{
		BTN_Start->OnClicked.RemoveAll(this);
		BTN_Start->OnClicked.AddDynamic(this, &ThisClass::HandleStartClicked);
		BTN_Start->SetIsEnabled(true);
	}
	PlayBackgroundLoop();
}

void UBalhwajeomMainMenuWidget::NativeDestruct()
{
	if (BTN_Start)
	{
		BTN_Start->OnClicked.RemoveAll(this);
	}
	// Matches ABalhwajeomIntroFlowActor's own MediaPlayer::Close() calls on every transition away
	// from the state that owns the playback -- this widget is torn down (RemoveFromParent then
	// dropped to null) the moment the intro flow leaves Title, so the loop must stop here rather
	// than relying on GC to eventually release the player.
	if (BackgroundMediaPlayer)
	{
		BackgroundMediaPlayer->Close();
	}
	Super::NativeDestruct();
}

void UBalhwajeomMainMenuWidget::SetStartButtonEnabled(bool bEnabled)
{
	bStartAccepted = !bEnabled;
	if (BTN_Start)
	{
		BTN_Start->SetIsEnabled(bEnabled);
	}
}

void UBalhwajeomMainMenuWidget::HandleStartClicked()
{
	if (bStartAccepted)
	{
		return;
	}
	bStartAccepted = true;
	if (BTN_Start)
	{
		BTN_Start->SetIsEnabled(false);
	}
	if (USoundBase* Sound = ButtonClickSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
	OnStartRequested.Broadcast();
}

void UBalhwajeomMainMenuWidget::SetBackgroundMediaAssets(
	UMediaSource* Source,
	UMediaPlayer* Player,
	UMediaTexture* Texture)
{
	BackgroundMediaSource = Source;
	BackgroundMediaPlayer = Player;
	BackgroundMediaTexture = Texture;
	PlayBackgroundLoop();
}

void UBalhwajeomMainMenuWidget::PlayBackgroundLoop()
{
	if (!BackgroundMediaSource || !BackgroundMediaPlayer || !BackgroundMediaTexture)
	{
		return;
	}

	BackgroundMediaTexture->SetMediaPlayer(BackgroundMediaPlayer);
	if (IMG_Background)
	{
		FSlateBrush Brush = IMG_Background->GetBrush();
		Brush.SetResourceObject(BackgroundMediaTexture);
		Brush.ImageSize = FVector2D(1920.0f, 1080.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		IMG_Background->SetBrush(Brush);
	}

	// The title loop is the one media playback in this flow that should never stop on its own --
	// contrast ABalhwajeomIntroFlowActor's IntroMediaPlayer/EndingMediaPlayer, which both set
	// SetLooping(false) because HandleMediaEndReached is what advances their state machine.
	BackgroundMediaPlayer->SetLooping(true);
	BackgroundMediaPlayer->OpenSource(BackgroundMediaSource);
}
