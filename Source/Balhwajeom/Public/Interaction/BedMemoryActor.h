#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "Interaction/WorldInteractable.h"
#include "Investigation/PhotoDefinitions.h"
#include "UObject/SoftObjectPtr.h"
#include "BedMemoryActor.generated.h"

class ACharacter;
class APlayerController;
class APhotoWorldStoryActor;
class UAnimationAsset;
class UAnimInstance;
class UAnimMontage;
class UAnimSequenceBase;
class UArrowComponent;
class UAudioComponent;
class UBoxComponent;
class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInspectionComponent;
class USceneComponent;
class USoundBase;
class USoundMix;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EBedMemoryState : uint8
{
	Idle,
	AligningPlayer,
	Entering,
	Seated,
	PreparingAudio,
	Listening,
	Exiting,
	Restoring
};

USTRUCT()
struct FBedMemoryStoryCandidate
{
	GENERATED_BODY()

	UPROPERTY()
	FName PhotoID = NAME_None;

	UPROPERTY()
	FPhotoDefinition PhotoDefinition;
};

/**
 * Blueprint-placeable bed interaction that presents WorldStoryCues belonging
 * to photos currently captured in UBalhwajeomInvestigationSubsystem.
 */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABedMemoryActor : public AActor, public IWorldInteractable
{
	GENERATED_BODY()

public:
	ABedMemoryActor();

	virtual bool CanInteract_Implementation(APawn* InteractingPawn) const override;
	virtual bool RequestInteraction_Implementation(APawn* InteractingPawn) override;

	UFUNCTION(BlueprintCallable, Category = "Bed Memory")
	bool BeginRest(APawn* InteractingPawn);

	UFUNCTION(BlueprintCallable, Category = "Bed Memory")
	void EndRest();

	/** Collider-owned F input calls this for both entry and exit. */
	UFUNCTION(BlueprintCallable, Category = "Bed Memory")
	bool ToggleRest(APawn* InteractingPawn);

	UFUNCTION(BlueprintPure, Category = "Bed Memory")
	bool IsPawnWithinInteractionZone(APawn* Pawn) const;

	UFUNCTION(BlueprintPure, Category = "Bed Memory")
	EBedMemoryState GetBedMemoryState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Bed Memory")
	int32 GetAvailableStoryCount() const { return StoryCandidates.Num(); }

	UFUNCTION(BlueprintPure, Category = "Bed Memory",
		meta = (DeprecatedFunction, DeprecationMessage = "Use GetAvailableStoryCount."))
	int32 GetAvailableVoiceCount() const { return GetAvailableStoryCount(); }

	UFUNCTION(BlueprintCallable, Category = "Bed Memory|UI")
	void SetInspectionLabelSuppressed(bool bSuppressed);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandlePlayerDistanceStateChanged(EPlayerInspectionDistanceState NewState);

	UFUNCTION()
	void HandleToggleInput();

	UFUNCTION()
	void HandleInteractionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleInteractionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void FinishEntering();

	UFUNCTION()
	void UpdatePlayerTurn();

	UFUNCTION()
	void FinishExiting();

	UFUNCTION()
	void HandleMemoryStoryDestroyed(AActor* DestroyedActor);

	void BuildStoryCandidates();
	void BeginPlayerTurn();
	void FinishPlayerTurn();
	void BeginEntering();
	void BeginPreparingAudio();
	void BeginListening();
	void PlayNextStory();
	void RefillShuffleBag();
	void ScheduleNextStory(bool bInitialDelay);
	void SavePlayerAnimationState();
	float PlayBedAnimation(UAnimationAsset* Animation, bool bLooping, float PlayRate, float StartPosition);
	void RestorePlayerAnimationState();
	void RestorePlayerState();
	bool SetupRestInput(APlayerController* PlayerController);
	void TeardownRestInput();
	void ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState);
	void SetInspectionLabel(const FText& LabelText, bool bVisible);
	void SetWorldEvidenceLabelsSuppressed(bool bSuppressed) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Intentionally has no default mesh. Assign the bed mesh in BP_BedMemory. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UStaticMeshComponent> BedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UBoxComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UInspectionComponent> InspectionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UWidgetComponent> InteractionWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> PlayerAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> ExitAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UCameraComponent> SeatedCamera;

	/** Move and rotate this component to author where collected-photo memories appear. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> StoryAnchor;

#if WITH_EDITORONLY_DATA
	/** Editor-only +X reading-direction indicator for StoryAnchor. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> StoryAnchorArrow;

	/** Editor-only sample caption used to preview the anchor position and reading angle. */
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> StoryAnchorPreviewText;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> VoiceOrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> VoiceDesk;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> VoiceDoor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> VoiceBed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> VoiceHall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<USceneComponent> VoicePhoto;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UAudioComponent> BGMPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bed Memory|Components")
	TObjectPtr<UAudioComponent> VoicePlayer;

	/** Animation Sequence or Montage played forward to sit and backward to stand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Animation",
		meta = (DisplayName = "Sit Animation"))
	TObjectPtr<UAnimSequenceBase> SitAnimation;

	/** Optional looping seated idle. Leave empty to hold the final Sit Animation pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Animation")
	TObjectPtr<UAnimMontage> SeatedIdleMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Audio")
	TObjectPtr<USoundBase> BedBGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Audio")
	TObjectPtr<USoundMix> BedSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Audio", meta = (ClampMin = "0.0"))
	float BGMFadeInDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Audio", meta = (ClampMin = "0.0"))
	float BGMFadeOutDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Audio", meta = (ClampMin = "0.0"))
	float BGMVolume = 0.7f;

	/** Random delay after sitting before the first collected-photo story appears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Story Timing",
		meta = (DisplayName = "Initial Story Delay Range", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	FVector2D InitialStoryDelayRange = FVector2D::ZeroVector;

	/** Random delay between ordinary collected-photo stories. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Story Timing",
		meta = (DisplayName = "Story Interval Range", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	FVector2D NormalGapRange = FVector2D(1.0f, 4.0f);

	/** Random silence used when a long pause is selected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Story Timing",
		meta = (DisplayName = "Long Silence Range", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	FVector2D LongGapRange = FVector2D(4.0f, 7.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Story Timing",
		meta = (DisplayName = "Long Silence Chance", ClampMin = "0.0", ClampMax = "1.0",
			UIMin = "0.0", UIMax = "1.0"))
	float LongGapChance = 0.2f;

	/** Presentation actor spawned at StoryAnchor for each collected photo. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bed Memory|Story")
	TSubclassOf<APhotoWorldStoryActor> StoryActorClass;

	/** Seconds used to blend from the exploration camera to SeatedCamera. Zero cuts immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Camera",
		meta = (DisplayName = "Camera Transition Duration", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float CameraBlendInDuration = 0.6f;

	/** Seconds used to rotate the player 180 degrees away from the bed. Zero rotates immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Animation",
		meta = (DisplayName = "Player Rotation Duration", ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float PlayerRotationDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Input")
	TObjectPtr<UInputAction> ExitAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Input")
	int32 RestInputPriority = 2000;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FBedMemoryTestAccessor;
#endif

	UPROPERTY(Transient)
	EBedMemoryState State = EBedMemoryState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> RestingCharacter;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> RestingPlayerController;

	UPROPERTY(Transient)
	TObjectPtr<APawn> OverlappingPawn;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> RestInputComponent;

	UPROPERTY(Transient)
	TArray<FBedMemoryStoryCandidate> StoryCandidates;

	UPROPERTY(Transient)
	TArray<int32> ShuffleBag;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> SavedAnimationAsset;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> SavedAnimInstanceClass;

	FTimerHandle TransitionTimer;
	FTimerHandle PlayerTurnTimer;
	FTimerHandle StoryTimer;

	UPROPERTY(Transient)
	TWeakObjectPtr<APhotoWorldStoryActor> ActiveMemoryStory;
	FTransform SavedPlayerTransform;
	FRotator SavedControlRotation = FRotator::ZeroRotator;
	FRotator PlayerTurnStartRotation = FRotator::ZeroRotator;
	FRotator PlayerTurnTargetRotation = FRotator::ZeroRotator;
	FName LastPlayedPhotoID = NAME_None;
	double EarliestExitTimeSeconds = 0.0;
	double PlayerTurnStartedAtSeconds = 0.0;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	uint8 SavedAnimationMode = 0;
	float SavedAnimationPosition = 0.0f;
	float SavedAnimationPlayRate = 1.0f;
	EPlayerInspectionDistanceState LastInspectionDistanceState = EPlayerInspectionDistanceState::OutOfRange;
	bool bInspectionLabelSuppressed = false;
	bool bSoundMixApplied = false;
	bool bSavedHUDVisible = true;
	bool bHasSavedControlRotation = false;
	bool bHasSavedAnimationState = false;
	bool bSavedAnimationLooping = false;
	bool bSavedAnimationPlaying = false;
};
