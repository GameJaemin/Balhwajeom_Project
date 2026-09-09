#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractionTypes.h"
#include "Interaction/WorldInteractable.h"
#include "UObject/SoftObjectPtr.h"
#include "BedMemoryActor.generated.h"

class ACharacter;
class APlayerController;
class UAnimMontage;
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
class UWidgetComponent;
struct FStreamableHandle;

UENUM(BlueprintType)
enum class EBedMemoryState : uint8
{
	Idle,
	AligningPlayer,
	Entering,
	PreparingAudio,
	Listening,
	Exiting,
	Restoring
};

USTRUCT()
struct FBedMemoryVoiceCandidate
{
	GENERATED_BODY()

	UPROPERTY()
	FName PhotoID = NAME_None;

	UPROPERTY()
	TSoftObjectPtr<USoundBase> StoryVoice;

	UPROPERTY()
	FName EmitterID = NAME_None;
};

/**
 * Blueprint-placeable bed interaction that replays StoryVoice assets belonging
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
	int32 GetAvailableVoiceCount() const { return VoiceCandidates.Num(); }

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
	void FinishExiting();

	UFUNCTION()
	void HandleVoiceFinished();

	void BuildVoiceCandidates();
	void BeginPreparingAudio();
	void HandleVoiceAssetsLoaded();
	void BeginListening();
	void PlayNextVoice();
	void RefillShuffleBag();
	void ScheduleNextVoice(bool bInitialDelay);
	void RestorePlayerState();
	bool SetupRestInput(APlayerController* PlayerController);
	void TeardownRestInput();
	void ApplyInspectionDistanceState(EPlayerInspectionDistanceState DistanceState);
	void SetInspectionLabel(const FText& LabelText, bool bVisible);
	void SetWorldEvidenceLabelsSuppressed(bool bSuppressed) const;
	USceneComponent* ResolveEmitter(FName EmitterID) const;
	FName ResolveEmitterID(FName PhotoID) const;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Animation")
	TObjectPtr<UAnimMontage> EnterMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Animation")
	TObjectPtr<UAnimMontage> SeatedIdleMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Animation")
	TObjectPtr<UAnimMontage> ExitMontage;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Timing")
	FVector2D InitialDelayRange = FVector2D(1.0f, 2.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Timing")
	FVector2D NormalGapRange = FVector2D(1.0f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Timing")
	FVector2D LongGapRange = FVector2D(4.0f, 7.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LongGapChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Camera", meta = (ClampMin = "0.0"))
	float CameraBlendInDuration = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Camera", meta = (ClampMin = "0.0"))
	float CameraBlendOutDuration = 0.4f;

	/** Per-room spatial direction. Unmapped photos use VoiceOrigin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bed Memory|Voice")
	TMap<FName, FName> PhotoEmitterMap;

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
	TArray<FBedMemoryVoiceCandidate> VoiceCandidates;

	UPROPERTY(Transient)
	TArray<int32> ShuffleBag;

	TSharedPtr<FStreamableHandle> VoiceLoadHandle;
	FTimerHandle TransitionTimer;
	FTimerHandle VoiceTimer;
	FTransform SavedPlayerTransform;
	FName LastPlayedPhotoID = NAME_None;
	double EarliestExitTimeSeconds = 0.0;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	EPlayerInspectionDistanceState LastInspectionDistanceState = EPlayerInspectionDistanceState::OutOfRange;
	bool bInspectionLabelSuppressed = false;
	bool bSoundMixApplied = false;
	bool bSavedHUDVisible = true;
};
