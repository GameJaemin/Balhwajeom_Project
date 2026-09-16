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

/** Self-contained world-locked caption and narration presentation spawned after a photo capture. */
UCLASS(Blueprintable)
class BALHWAJEOM_API APhotoWorldStoryActor : public AActor
{
	GENERATED_BODY()

public:
	APhotoWorldStoryActor();

	/**
	 * Spawns one presentation at SpawnTransform and starts it. Returns nullptr when the photo has
	 * nothing to present, so both the capture flow and evidence interaction share a single spawn path.
	 */
	static APhotoWorldStoryActor* SpawnAndStart(
		UWorld* World,
		TSubclassOf<APhotoWorldStoryActor> StoryClass,
		const FTransform& SpawnTransform,
		const FPhotoDefinition& PhotoDefinition,
		AActor* Owner);

	/** Starts loading and playing one story. Legacy lines are only used while old tables migrate. */
	void StartStory(
		const TArray<FPhotoStoryCue>& InCues,
		const TArray<FText>& LegacyLines,
		const TSoftObjectPtr<USoundBase>& InCueSound,
		float InLastCueDurationSeconds);

	/** Stops narration and fades this presentation out, for example when a new photo replaces it. */
	UFUNCTION(BlueprintCallable, Category = "Photo Story")
	void StopStory();

	/** Keeps the actor and widget scale fixed, then enlarges the actual font for the third-person view. */
	UFUNCTION(BlueprintCallable, Category = "Photo Story")
	void TransitionToThirdPersonScale();

	/** Pure typewriter timing helper exposed for deterministic tests and UI previews. */
	static float CalculateTypingDuration(
		int32 TotalCharacterCount,
		float CharactersPerSecond);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Photo Story")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Photo Story")
	TObjectPtr<UWidgetComponent> StoryWidgetComponent;

	/** Opposite-facing copy so the caption is equally bright and readable from either side. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Photo Story")
	TObjectPtr<UWidgetComponent> BackStoryWidgetComponent;

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

	/** Compatibility fallback for invalid or legacy per-photo final-cue durations. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Photo Story|Timing",
		meta = (ClampMin = "0.1", UIMin = "0.1", Units = "s"))
	float DefaultLastCueDuration = 2.5f;

private:
	void InitializeStoryWidgets();
	void SetStoryWidgetsVisible(bool bVisible);
	void SetStoryWidgetsText(const FText& Text);
	void SetStoryWidgetsOpacity(float Opacity);
	void SetStoryWidgetsFontSize(int32 FontSize);
	void PlayCharacterSound(TCHAR Character);
	void ApplyCue(int32 CueIndex);
	void RevealNextCharacter();
	void ScheduleNextCue();
	void HandleCueTimer();
	void FinishStory();
	void UpdateFadeOut();
	void UpdateScaleTransition();

	TArray<FPhotoStoryCue> StoryCues;
	TSoftObjectPtr<USoundBase> StoryCueSound;
	FTimerHandle CueTimer;
	FTimerHandle TypewriterTimer;
	FTimerHandle FadeTimer;
	FTimerHandle ScaleTimer;
	int32 CurrentCueIndex = INDEX_NONE;
	double StoryStartTimeSeconds = 0.0;
	double FadeStartTimeSeconds = 0.0;
	double ScaleStartTimeSeconds = 0.0;
	int32 FontSizeTransitionStart = 32;
	int32 FontSizeTransitionTarget = 32;
	float ScaleTransitionDuration = 0.0f;
	float LastCueDurationSeconds = 2.5f;
	FString CurrentCueFullString;
	float CurrentCharactersPerSecond = 0.0f;
	int32 VisibleCharacterCount = 0;
	bool bFinishing = false;
	bool bThirdPersonScaleRequested = false;
};
