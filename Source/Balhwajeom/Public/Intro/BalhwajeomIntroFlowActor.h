#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BalhwajeomIntroFlowActor.generated.h"

class ABalhwajeomGateDoorActor;
class ALevelSequenceActor;
class UAudioComponent;
class UBalhwajeomMainMenuWidget;
class UBalhwajeomScreenFadeWidget;
class ULevelSequence;
class ULevelSequencePlayer;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class USoundBase;
class UBalhwajeomCinematicVideoWidget;
struct FIntroFlowActorTestAccessor;

UENUM(BlueprintType)
enum class EBalhwajeomIntroState : uint8
{
	Boot,
	Title,
	TitleStart,
	TransitionToCinematic,
	Cinematic,
	TransitionToGameplay,
	Gameplay,
	TransitionToEnding,
	Ending,
	TransitionToTitle
};

/** Coordinates title, fades, BGM, optional Level Sequence, and gameplay input. */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomIntroFlowActor : public AActor
{
	GENERATED_BODY()

public:
	ABalhwajeomIntroFlowActor();

	UFUNCTION(BlueprintPure, Category = "Intro")
	EBalhwajeomIntroState GetIntroState() const { return State; }

	/** Assigns a pre-rendered movie setup. When valid, this takes priority over IntroSequence. */
	UFUNCTION(BlueprintCallable, Category = "Intro|Media")
	void SetIntroMediaAssets(UMediaSource* Source, UMediaPlayer* Player, UMediaTexture* Texture);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TSoftClassPtr<UBalhwajeomMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TSoftClassPtr<UBalhwajeomScreenFadeWidget> ScreenFadeWidgetClass;

	/** Plays once gameplay actually starts (EnterGameplayAtBlack) and while no BGMTriggerDoor override
	 * is active yet. Assign your imported Sound Wave, Sound Cue, or MetaSound here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<USoundBase> BGM;

	/** Plays only while the title screen (WBP_MainMenu) is shown. Falls back to BGM when unset, so
	 * existing levels that only fill in BGM keep working unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<USoundBase> Title_BGM;

	/** Replaces BGM the moment BGMTriggerDoor finishes opening. Leave BGMTriggerDoor unset to disable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<USoundBase> BGM_Sound;

	/** Level-placed door whose OnDoorFullyOpened switches the BGM track to BGM_Sound. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<ABalhwajeomGateDoorActor> BGMTriggerDoor;

	/** Assign a Level Sequence containing a Camera Cuts track. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<ULevelSequence> IntroSequence;

	/** Optional clip played once, right after Start is clicked and before the intro cinematic. Plays
	 * to completion with no skip control. Leave any of the three unset to skip this step entirely
	 * and go straight to the existing intro cinematic, unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TObjectPtr<UMediaSource> TitleStartMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TObjectPtr<UMediaPlayer> TitleStartMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TObjectPtr<UMediaTexture> TitleStartMediaTexture;

	/** Optional full-screen MP4 path. Takes priority over IntroSequence when all three assets are valid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TObjectPtr<UMediaSource> IntroMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TObjectPtr<UMediaPlayer> IntroMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TObjectPtr<UMediaTexture> IntroMediaTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Media")
	TSoftClassPtr<UBalhwajeomCinematicVideoWidget> CinematicVideoWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float InitialFadeDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float TransitionFadeDuration = 0.8f;

	/** How long the TitleStart clip takes to fade from invisible to fully opaque once its media
	 * actually opens. The widget is added at 0 opacity so the still-open/buffering media source
	 * never shows a blank gap over the title screen; this softens the reveal once real frames
	 * are ready instead of popping straight to full opacity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float TitleStartRevealFadeDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float BGMFadeDuration = 0.8f;

	/** Once every character's statement is solved, how long to hold on the tablet's success
	 * animation (with input locked) before automatically starting the ending -- the player no
	 * longer has to close the tablet themselves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float EndingAutoTriggerDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Audio", meta = (ClampMin = "0.0"))
	float BGMVolume = 1.0f;

	/** Opens the tablet directly on the default person's statement when the intro enters gameplay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Gameplay")
	bool bOpenStatementAfterIntro = true;

	/** Starts a new run without photographs left by a previous play session. */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Intro|Gameplay",
		meta = (DisplayName = "시작 시 촬영 사진 초기화"))
	bool bResetInvestigationPhotosOnStart = true;

	/** Optional ending MP4 setup. All three media fields must be assigned; otherwise EndingSequence is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Ending")
	TObjectPtr<UMediaSource> EndingMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Ending")
	TObjectPtr<UMediaPlayer> EndingMediaPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Ending")
	TObjectPtr<UMediaTexture> EndingMediaTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Ending")
	TObjectPtr<ULevelSequence> EndingSequence;

private:
	friend struct FIntroFlowActorTestAccessor;

	UFUNCTION()
	void HandleStartRequested();

	UFUNCTION()
	void HandleFadeToBlackFinished();

	UFUNCTION()
	void HandleFadeFromBlackFinished();

	UFUNCTION()
	void HandleFadeProgress(float Opacity);

	UFUNCTION()
	void HandleSequenceFinished();

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleMediaEndReached();

	UFUNCTION()
	void HandleSkipRequested();

	/** Bound to BGMTriggerDoor's OnDoorFullyOpened; switches the currently playing track to BGM_Sound. */
	UFUNCTION()
	void HandleBGMTriggerDoorOpened();

	/** Scales the movie audio without touching the picture, so a skip can mute while the fade runs. */
	void SetCinematicAudioVolume(float Volume);

	void SetGameplayEnabled(bool bEnabled);
	void ResetInvestigationPhotosIfRequested();
	void StartTitleStart();
	void StartCinematic();
	bool StartMediaCinematic();
	void StartSequenceCinematic();
	void BeginGameplayTransition();
	void EnterGameplayAtBlack();
	void HandleTabletClosed();
	void HandleStatementSolved();
	bool AreAllStatementsSolved() const;
	void TriggerEndingSequence();
	void StartEndingCinematic();
	bool StartEndingMediaCinematic();
	void StartEndingSequenceCinematic();
	void BeginTitleTransition();
	void ReturnToTitle();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAudioComponent> BGMAudioComponent;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomMainMenuWidget> MainMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomScreenFadeWidget> ScreenFadeWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomCinematicVideoWidget> CinematicVideoWidget;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	EBalhwajeomIntroState State = EBalhwajeomIntroState::Boot;
	bool bEndingTriggered = false;

	/** While set, movie audio is ducked in step with the fade overlay's opacity. */
	bool bFadeCinematicAudioWithScreen = false;

	/** Runs TriggerEndingSequence() after EndingAutoTriggerDelay once every statement is solved. */
	FTimerHandle EndingAutoTriggerTimerHandle;
};
