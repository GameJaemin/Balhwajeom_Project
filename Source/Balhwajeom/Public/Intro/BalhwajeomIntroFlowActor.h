#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BalhwajeomIntroFlowActor.generated.h"

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

UENUM(BlueprintType)
enum class EBalhwajeomIntroState : uint8
{
	Boot,
	Title,
	TransitionToCinematic,
	Cinematic,
	TransitionToGameplay,
	Gameplay
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

	/** Assign your imported Sound Wave, Sound Cue, or MetaSound here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<USoundBase> BGM;

	/** Assign a Level Sequence containing a Camera Cuts track. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Assets")
	TObjectPtr<ULevelSequence> IntroSequence;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", Units = "s"))
	float BGMFadeDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro|Audio", meta = (ClampMin = "0.0"))
	float BGMVolume = 1.0f;

private:
	UFUNCTION()
	void HandleStartRequested();

	UFUNCTION()
	void HandleFadeToBlackFinished();

	UFUNCTION()
	void HandleFadeFromBlackFinished();

	UFUNCTION()
	void HandleSequenceFinished();

	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleMediaEndReached();

	void SetGameplayEnabled(bool bEnabled);
	void StartCinematic();
	bool StartMediaCinematic();
	void StartSequenceCinematic();
	void BeginGameplayTransition();
	void EnterGameplayAtBlack();

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
};
