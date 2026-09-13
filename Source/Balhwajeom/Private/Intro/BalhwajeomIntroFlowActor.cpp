#include "Intro/BalhwajeomIntroFlowActor.h"

#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "Components/AudioComponent.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/EvidenceDefinitions.h"
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
	if (EndingMediaPlayer)
	{
		EndingMediaPlayer->OnMediaOpened.RemoveAll(this);
		EndingMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		EndingMediaPlayer->OnEndReached.RemoveAll(this);
		EndingMediaPlayer->Close();
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (UBalhwajeomTabletComponent* Tablet = Pawn
		? Pawn->FindComponentByClass<UBalhwajeomTabletComponent>()
		: (PC ? PC->FindComponentByClass<UBalhwajeomTabletComponent>() : nullptr))
	{
		Tablet->OnTabletClosed.RemoveAll(this);
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
	else if (State == EBalhwajeomIntroState::TransitionToEnding)
	{
		StartEndingCinematic();
	}
	else if (State == EBalhwajeomIntroState::TransitionToTitle)
	{
		ReturnToTitle();
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
	UMediaPlayer* ActiveMediaPlayer = State == EBalhwajeomIntroState::Ending
		? EndingMediaPlayer.Get() : IntroMediaPlayer.Get();
	if ((State != EBalhwajeomIntroState::Cinematic && State != EBalhwajeomIntroState::Ending) ||
		!ActiveMediaPlayer)
	{
		return;
	}
	ActiveMediaPlayer->Play();
	ScreenFadeWidget->FadeFromBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Error, TEXT("%s: Failed to open intro media: %s"), *GetName(), *FailedUrl);
	UMediaPlayer* FailedPlayer = State == EBalhwajeomIntroState::Ending
		? EndingMediaPlayer.Get() : IntroMediaPlayer.Get();
	if (FailedPlayer)
	{
		FailedPlayer->OnMediaOpened.RemoveAll(this);
		FailedPlayer->OnMediaOpenFailed.RemoveAll(this);
		FailedPlayer->OnEndReached.RemoveAll(this);
	}
	if (CinematicVideoWidget)
	{
		CinematicVideoWidget->RemoveFromParent();
		CinematicVideoWidget = nullptr;
	}
	if (State == EBalhwajeomIntroState::Ending)
	{
		StartEndingSequenceCinematic();
	}
	else
	{
		StartSequenceCinematic();
	}
}

void ABalhwajeomIntroFlowActor::HandleMediaEndReached()
{
	if (State == EBalhwajeomIntroState::Ending)
	{
		BeginTitleTransition();
	}
	else if (State == EBalhwajeomIntroState::Cinematic)
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
	if (State == EBalhwajeomIntroState::Ending)
	{
		BeginTitleTransition();
	}
	else
	{
		BeginGameplayTransition();
	}
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
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UBalhwajeomTabletComponent* Tablet = Pawn
		? Pawn->FindComponentByClass<UBalhwajeomTabletComponent>()
		: (PC ? PC->FindComponentByClass<UBalhwajeomTabletComponent>() : nullptr);
	if (Tablet)
	{
		Tablet->OnTabletClosed.RemoveAll(this);
		Tablet->OnTabletClosed.AddUObject(this, &ThisClass::HandleTabletClosed);
		if (bOpenStatementAfterIntro)
		{
			Tablet->RequestOpenTabletToStatement();
		}
	}
	if (BGM)
	{
		BGMAudioComponent->Stop();
		BGMAudioComponent->SetSound(BGM);
		BGMAudioComponent->FadeIn(BGMFadeDuration, BGMVolume);
	}
	ScreenFadeWidget->FadeFromBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::HandleTabletClosed()
{
	if (State != EBalhwajeomIntroState::Gameplay || bEndingTriggered)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UBalhwajeomInvestigationSubsystem* Investigation = GameInstance
		? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
	if (!Investigation)
	{
		return;
	}

	bool bFoundStatement = false;
	TArray<FCharacterDefinition> Characters;
	Investigation->GetAllCharacterDefinitions(Characters);
	for (const FCharacterDefinition& Character : Characters)
	{
		TArray<FSentenceDefinition> Statements;
		Investigation->GetStatementSentencesForCharacter(Character.CharacterID, Statements);
		for (const FSentenceDefinition& Statement : Statements)
		{
			bFoundStatement = true;
			if (!Investigation->IsSentenceSolved(Statement.SentenceID))
			{
				return;
			}
		}
	}
	if (!bFoundStatement)
	{
		return;
	}

	bEndingTriggered = true;
	State = EBalhwajeomIntroState::TransitionToEnding;
	if (BGMAudioComponent && BGMAudioComponent->IsPlaying())
	{
		BGMAudioComponent->FadeOut(BGMFadeDuration, 0.0f);
	}
	SetGameplayEnabled(false);
	ScreenFadeWidget->FadeToBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::StartEndingCinematic()
{
	if (!StartEndingMediaCinematic())
	{
		StartEndingSequenceCinematic();
	}
}

bool ABalhwajeomIntroFlowActor::StartEndingMediaCinematic()
{
	if (!EndingMediaSource || !EndingMediaPlayer || !EndingMediaTexture)
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
	EndingMediaTexture->SetMediaPlayer(EndingMediaPlayer);
	CinematicVideoWidget->SetMediaTexture(EndingMediaTexture);
	CinematicVideoWidget->AddToPlayerScreen(2000);
	EndingMediaPlayer->OnMediaOpened.RemoveAll(this);
	EndingMediaPlayer->OnMediaOpenFailed.RemoveAll(this);
	EndingMediaPlayer->OnEndReached.RemoveAll(this);
	EndingMediaPlayer->OnMediaOpened.AddDynamic(this, &ThisClass::HandleMediaOpened);
	EndingMediaPlayer->OnMediaOpenFailed.AddDynamic(this, &ThisClass::HandleMediaOpenFailed);
	EndingMediaPlayer->OnEndReached.AddDynamic(this, &ThisClass::HandleMediaEndReached);
	EndingMediaPlayer->SetLooping(false);
	State = EBalhwajeomIntroState::Ending;
	if (!EndingMediaPlayer->OpenSource(EndingMediaSource))
	{
		HandleMediaOpenFailed(EndingMediaSource->GetUrl());
	}
	return true;
}

void ABalhwajeomIntroFlowActor::StartEndingSequenceCinematic()
{
	if (!EndingSequence)
	{
		ReturnToTitle();
		return;
	}
	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	Settings.LoopCount.Value = 0;
	ALevelSequenceActor* CreatedSequenceActor = nullptr;
	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), EndingSequence, Settings, CreatedSequenceActor);
	SequenceActor = CreatedSequenceActor;
	if (!SequencePlayer)
	{
		ReturnToTitle();
		return;
	}
	SequencePlayer->OnFinished.AddDynamic(this, &ThisClass::HandleSequenceFinished);
	State = EBalhwajeomIntroState::Ending;
	SequencePlayer->Play();
	ScreenFadeWidget->FadeFromBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::BeginTitleTransition()
{
	State = EBalhwajeomIntroState::TransitionToTitle;
	ScreenFadeWidget->FadeToBlack(TransitionFadeDuration);
}

void ABalhwajeomIntroFlowActor::ReturnToTitle()
{
	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevel));
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
