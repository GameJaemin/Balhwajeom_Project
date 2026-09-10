#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Investigation/PhotoDefinitions.h"
#include "PhotoWorldStoryActor.generated.h"

class UAudioComponent;
class UPhotoWorldStoryWidget;
class USceneComponent;
class USoundBase;
class UUserWidget;
class UWidgetComponent;
struct FStreamableHandle;

/** Self-contained world-locked caption and narration presentation spawned after a photo capture. */
UCLASS(Blueprintable)
class BALHWAJEOM_API APhotoWorldStoryActor : public AActor
{
	GENERATED_BODY()

public:
	APhotoWorldStoryActor();

	/** Starts loading and playing one story. Legacy lines are only used while old tables migrate. */
	void StartStory(
		const TArray<FPhotoStoryCue>& InCues,
		const TArray<FText>& LegacyLines,
		const TSoftObjectPtr<USoundBase>& InVoice);

	/** Stops narration and fades this presentation out, for example when a new photo replaces it. */
	UFUNCTION(BlueprintCallable, Category = "Photo Story")
	void StopStory();

	/** Keeps the actor and widget scale fixed, then enlarges the actual font for the third-person view. */
	UFUNCTION(BlueprintCallable, Category = "Photo Story")
	void TransitionToThirdPersonScale();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Photo Story")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Photo Story")
	TObjectPtr<UWidgetComponent> StoryWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Photo Story")
	TObjectPtr<UAudioComponent> StoryAudioComponent;

	/** Optional styled Widget Blueprint. The native widget is used when this is empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual")
	TSubclassOf<UPhotoWorldStoryWidget> StoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Visual",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float FadeOutDuration = 0.35f;

	/** Used only for legacy WorldStoryLines that do not have authored cue times. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Compatibility",
		meta = (ClampMin = "0.1", UIMin = "0.1", Units = "s"))
	float LegacySecondsPerLine = 2.5f;

	/** How long the final cue remains visible when this story has no StoryVoice. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Timing",
		meta = (ClampMin = "0.1", UIMin = "0.1", Units = "s"))
	float NoVoiceLastCueDuration = 2.5f;

private:
	void HandleVoiceLoaded();
	void PlayLoadedVoice();
	void ApplyCue(int32 CueIndex);
	void ScheduleNextCue();
	void HandleCueTimer();
	void FinishStory();
	void UpdateFadeOut();
	void UpdateScaleTransition();

	UFUNCTION()
	void HandleAudioFinished();

	TArray<FPhotoStoryCue> StoryCues;
	TSoftObjectPtr<USoundBase> StoryVoice;
	TSharedPtr<FStreamableHandle> VoiceLoadHandle;
	FTimerHandle CueTimer;
	FTimerHandle FadeTimer;
	FTimerHandle ScaleTimer;
	int32 CurrentCueIndex = INDEX_NONE;
	double StoryStartTimeSeconds = 0.0;
	double FadeStartTimeSeconds = 0.0;
	double ScaleStartTimeSeconds = 0.0;
	int32 FontSizeTransitionStart = 32;
	int32 FontSizeTransitionTarget = 32;
	float ScaleTransitionDuration = 0.0f;
	bool bFinishing = false;
	bool bThirdPersonScaleRequested = false;
	bool bPlayingWithoutVoice = false;
};
