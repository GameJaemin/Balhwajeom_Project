#include "Intro/BalhwajeomIntroFlowActor.h"

#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Sound/SoundBase.h"
#include "Tablet/BalhwajeomTabletComponent.h"
#include "UI/BalhwajeomMainMenuWidget.h"
#include "UI/BalhwajeomScreenFadeWidget.h"
#include "UI/BalhwajeomCinematicVideoWidget.h"

ABalhwajeomIntroFlowActor::ABalhwajeomIntroFlowActor()
{
	PrimaryActorTick.bCanEverTick = false;
	BGMAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BGM"));
	SetRootComponent(BGMAudioComponent);
	BGMAudioComponent->bAutoActivate = false;
	BGMAudioComponent->bIsUISound = true;

	MainMenuWidgetClass = TSoftClassPtr<UBalhwajeomMainMenuWidget>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/UI/Title/WBP_MainMenu.WBP_MainMenu_C")));
	ScreenFadeWidgetClass = TSoftClassPtr<UBalhwajeomScreenFadeWidget>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/UI/Title/WBP_ScreenFade.WBP_ScreenFade_C")));
	CinematicVideoWidgetClass = TSoftClassPtr<UBalhwajeomCinematicVideoWidget>(
		FSoftObjectPath(TEXT("/Game/Balhwajeom/UI/Title/WBP_CinematicVideo.WBP_CinematicVideo_C")));
}

void ABalhwajeomIntroFlowActor::SetIntroMediaAssets(
	UMediaSource* Source, UMediaPlayer* Player, UMediaTexture* Texture)
{
	IntroMediaSource = Source;
	IntroMediaPlayer = Player;
	IntroMediaTexture = Texture;
}

void ABalhwajeomIntroFlowActor::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PC) || !PC->IsLocalController())
	{
		return;
	}

	const TSubclassOf<UBalhwajeomMainMenuWidget> MenuClass = MainMenuWidgetClass.LoadSynchronous();
	const TSubclassOf<UBalhwajeomScreenFadeWidget> FadeClass = ScreenFadeWidgetClass.LoadSynchronous();
	if (!MenuClass || !FadeClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: WBP_MainMenu or WBP_ScreenFade is missing."), *GetName());
		return;
	}

	MainMenuWidget = CreateWidget<UBalhwajeomMainMenuWidget>(PC, MenuClass);
	ScreenFadeWidget = CreateWidget<UBalhwajeomScreenFadeWidget>(PC, FadeClass);
	if (!MainMenuWidget || !ScreenFadeWidget)
	{
		return;
	}

	MainMenuWidget->OnStartRequested.AddDynamic(this, &ThisClass::HandleStartRequested);
	ScreenFadeWidget->OnFadeToBlackFinished.AddDynamic(this, &ThisClass::HandleFadeToBlackFinished);
	ScreenFadeWidget->OnFadeFromBlackFinished.AddDynamic(this, &ThisClass::HandleFadeFromBlackFinished);
	MainMenuWidget->AddToPlayerScreen(1000);
	ScreenFadeWidget->SetBlackImmediately();
	ScreenFadeWidget->AddToPlayerScreen(9999);

	SetGameplayEnabled(false);
	if (BGM)
	{
		BGMAudioComponent->SetSound(BGM);
		BGMAudioComponent->FadeIn(BGMFadeDuration, BGMVolume);
	}
	State = EBalhwajeomIntroState::Title;
	ScreenFadeWidget->FadeFromBlack(InitialFadeDuration);
}

void ABalhwajeomIntroFlowActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SequencePlayer) SequencePlayer->OnFinished.RemoveAll(this);
	if (IntroMediaPlayer)
	{
		IntroMediaPlayer->OnMediaOpened.RemoveAll(this);
		IntroMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		IntroMediaPlayer->OnEndReached.RemoveAll(this);
		IntroMediaPlayer->Close();
	}
	if (CinematicVideoWidget) CinematicVideoWidget->RemoveFromParent();
	if (MainMenuWidget) MainMenuWidget->RemoveFromParent();
	if (ScreenFadeWidget) ScreenFadeWidget->RemoveFromParent();
	Super::EndPlay(EndPlayReason);
}

void ABalhwajeomIntroFlowActor::HandleStartRequested()
{
	if (State != EBalhwajeomIntroState::Title)
	{
		return;
	}
	State = EBalhwajeomIntroState::TransitionToCinematic;
	if (BGMAudioComponent->IsPlaying()) BGMAudioComponent->FadeOut(BGMFadeDuration, 0.0f);
	ScreenFadeWidget->FadeToBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::HandleFadeToBlackFinished()
{
	if (State == EBalhwajeomIntroState::TransitionToCinematic)
	{
		StartCinematic();
	}
	else if (State == EBalhwajeomIntroState::TransitionToGameplay)
	{
		EnterGameplayAtBlack();
	}
}

void ABalhwajeomIntroFlowActor::HandleFadeFromBlackFinished()
{
	if (State == EBalhwajeomIntroState::TransitionToGameplay)
	{
		State = EBalhwajeomIntroState::Gameplay;
	}
}

void ABalhwajeomIntroFlowActor::StartCinematic()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	if (StartMediaCinematic())
	{
		return;
	}
	StartSequenceCinematic();
}

bool ABalhwajeomIntroFlowActor::StartMediaCinematic()
{
	if (!IntroMediaSource || !IntroMediaPlayer || !IntroMediaTexture)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const TSubclassOf<UBalhwajeomCinematicVideoWidget> WidgetClass =
		CinematicVideoWidgetClass.LoadSynchronous();
	if (!PC || !WidgetClass)
	{
		return false;
	}

	CinematicVideoWidget = CreateWidget<UBalhwajeomCinematicVideoWidget>(PC, WidgetClass);
	if (!CinematicVideoWidget)
	{
		return false;
	}

	IntroMediaTexture->SetMediaPlayer(IntroMediaPlayer);
	CinematicVideoWidget->SetMediaTexture(IntroMediaTexture);
	CinematicVideoWidget->AddToPlayerScreen(2000);
	IntroMediaPlayer->OnMediaOpened.RemoveAll(this);
	IntroMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
	IntroMediaPlayer->OnEndReached.RemoveAll(this);
	IntroMediaPlayer->OnMediaOpened.AddDynamic(this, &ThisClass::HandleMediaOpened);
	IntroMediaPlayer->OnMediaOpenFailed.AddDynamic(this, &ThisClass::HandleMediaOpenFailed);
	IntroMediaPlayer->OnEndReached.AddDynamic(this, &ThisClass::HandleMediaEndReached);
	IntroMediaPlayer->SetLooping(false);
	State = EBalhwajeomIntroState::Cinematic;
	if (!IntroMediaPlayer->OpenSource(IntroMediaSource))
	{
		HandleMediaOpenFailed(IntroMediaSource->GetUrl());
		return true;
	}
	return true;
}

void ABalhwajeomIntroFlowActor::HandleMediaOpened(FString OpenedUrl)
{
	(void)OpenedUrl;
	if (State != EBalhwajeomIntroState::Cinematic || !IntroMediaPlayer)
	{
		return;
	}
	IntroMediaPlayer->Play();
	ScreenFadeWidget->FadeFromBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Error, TEXT("%s: Failed to open intro media: %s"), *GetName(), *FailedUrl);
	if (IntroMediaPlayer)
	{
		IntroMediaPlayer->OnMediaOpened.RemoveAll(this);
		IntroMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		IntroMediaPlayer->OnEndReached.RemoveAll(this);
	}
	if (CinematicVideoWidget)
	{
		CinematicVideoWidget->RemoveFromParent();
		CinematicVideoWidget = nullptr;
	}
	StartSequenceCinematic();
}

void ABalhwajeomIntroFlowActor::HandleMediaEndReached()
{
	if (State == EBalhwajeomIntroState::Cinematic)
	{
		BeginGameplayTransition();
	}
}

void ABalhwajeomIntroFlowActor::StartSequenceCinematic()
{
	if (!IntroSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: No playable intro media or sequence; continuing to gameplay."), *GetName());
		BeginGameplayTransition();
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	Settings.LoopCount.Value = 0;
	ALevelSequenceActor* CreatedSequenceActor = nullptr;
	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), IntroSequence, Settings, CreatedSequenceActor);
	SequenceActor = CreatedSequenceActor;
	if (!SequencePlayer)
	{
		BeginGameplayTransition();
		return;
	}
	SequencePlayer->OnFinished.AddDynamic(this, &ThisClass::HandleSequenceFinished);
	State = EBalhwajeomIntroState::Cinematic;
	SequencePlayer->Play();
	ScreenFadeWidget->FadeFromBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::HandleSequenceFinished()
{
	BeginGameplayTransition();
}

void ABalhwajeomIntroFlowActor::BeginGameplayTransition()
{
	State = EBalhwajeomIntroState::TransitionToGameplay;
	ScreenFadeWidget->FadeToBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::EnterGameplayAtBlack()
{
	if (IntroMediaPlayer)
	{
		IntroMediaPlayer->OnMediaOpened.RemoveAll(this);
		IntroMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		IntroMediaPlayer->OnEndReached.RemoveAll(this);
		IntroMediaPlayer->Close();
	}
	if (CinematicVideoWidget)
	{
		CinematicVideoWidget->RemoveFromParent();
		CinematicVideoWidget = nullptr;
	}
	SetGameplayEnabled(true);
	if (BGM)
	{
		BGMAudioComponent->Stop();
		BGMAudioComponent->SetSound(BGM);
		BGMAudioComponent->FadeIn(BGMFadeDuration, BGMVolume);
	}
	ScreenFadeWidget->FadeFromBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::SetGameplayEnabled(bool bEnabled)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PC)) return;

	if (ABalhwajeomCameraPlayerController* CameraPC = Cast<ABalhwajeomCameraPlayerController>(PC))
	{
		CameraPC->SetGameplayPresentationEnabled(bEnabled);
	}
	APawn* Pawn = PC->GetPawn();
	UBalhwajeomTabletComponent* Tablet = Pawn
		? Pawn->FindComponentByClass<UBalhwajeomTabletComponent>()
		: PC->FindComponentByClass<UBalhwajeomTabletComponent>();
	if (Tablet) Tablet->SetTabletInteractionEnabled(bEnabled);

	if (bEnabled)
	{
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(InputMode);
	}
	else
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		FInputModeUIOnly InputMode;
		if (MainMenuWidget) InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}
