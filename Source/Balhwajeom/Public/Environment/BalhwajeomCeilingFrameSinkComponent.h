// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BalhwajeomCeilingFrameSinkComponent.generated.h"

class UAudioComponent;
class UCurveFloat;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;

UENUM(BlueprintType)
enum class ECeilingFrameEffectPlacement : uint8
{
	AttachedToFrame UMETA(DisplayName = "Attached To Moving Frame"),
	WorldAtStart UMETA(DisplayName = "World Space At Start"),
	WorldAtDestination UMETA(DisplayName = "World Space At Destination")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCeilingFrameSinkFinished);

/**
 * Optional removal presentation for a ceiling-frame evidence actor.
 *
 * Add this component only to the ceiling-frame BP_TestAll1 child Blueprint. The evidence actor's
 * normal progression removal discovers it automatically, waits for the sink, then performs the
 * usual hide/collision/story-state completion. Other BP_TestAll1 instances keep instant removal.
 */
UCLASS(ClassGroup = (Balhwajeom), BlueprintType, Blueprintable,
	meta = (BlueprintSpawnableComponent, DisplayName = "Ceiling Frame Sink"))
class BALHWAJEOM_API UBalhwajeomCeilingFrameSinkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBalhwajeomCeilingFrameSinkComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Starts the transition once. False means configuration is incomplete or already used. */
	UFUNCTION(BlueprintCallable, Category = "Ceiling Frame Sink")
	bool StartSink();

	UFUNCTION(BlueprintPure, Category = "Ceiling Frame Sink")
	bool IsSinking() const { return bSinkStarted && !bSinkFinished; }

	UFUNCTION(BlueprintPure, Category = "Ceiling Frame Sink")
	bool IsSinkFinished() const { return bSinkFinished; }

	UPROPERTY(BlueprintAssignable, Category = "Ceiling Frame Sink")
	FOnCeilingFrameSinkFinished OnSinkFinished;

protected:
	/** Place an Empty Actor or Target Point at the exact final frame transform. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Ceiling Frame Sink|Target")
	TObjectPtr<AActor> DestinationActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Timing",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float StartDelay = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Timing",
		meta = (ClampMin = "0.01", UIMin = "0.01", Units = "s"))
	float SinkDuration = 1.0f;

	/** Optional normalized 0-1 curve. Ease-in/out is used when this is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Timing")
	TObjectPtr<UCurveFloat> SinkCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Transform")
	bool bApplyDestinationRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Transform")
	bool bApplyDestinationScale = false;

	/** Optional. Leaving this empty skips VFX without affecting movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Niagara")
	ECeilingFrameEffectPlacement NiagaraPlacement =
		ECeilingFrameEffectPlacement::AttachedToFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Niagara")
	FVector NiagaraOffset = FVector::ZeroVector;

	/** Optional. Starts on the same frame as Niagara and the downward movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Sound")
	TObjectPtr<USoundBase> SinkSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Sound",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Sound",
		meta = (ClampMin = "0.01", UIMin = "0.01"))
	float SoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Sound")
	FVector SoundOffset = FVector::ZeroVector;

	/** Useful for a looping rumble; one-shot sounds can leave this disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Sound")
	bool bStopSoundOnFinish = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling Frame Sink|Completion")
	bool bDisableCollisionOnStart = true;

private:
	void BeginSinkMovement();
	void FinishSink();
	void SpawnEffects();
	void StopEffects();
	UNiagaraComponent* SpawnNiagara() const;
	UAudioComponent* SpawnSound() const;

	bool bSinkStarted = false;
	bool bMovementStarted = false;
	bool bSinkFinished = false;
	float ElapsedTime = 0.0f;
	FTransform StartTransform = FTransform::Identity;
	FTransform DestinationTransform = FTransform::Identity;
	TWeakObjectPtr<UNiagaraComponent> ActiveNiagara;
	TWeakObjectPtr<UAudioComponent> ActiveSound;
};

