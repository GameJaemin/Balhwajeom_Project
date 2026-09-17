#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "Tutorial/BalhwajeomTutorialFlow.h"
#include "BalhwajeomTutorialDirector.generated.h"


class UStoryStateSubsystem;
struct FBalhwajeomTutorialDirectorTestAccessor;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnBalhwajeomTutorialStepChanged,
	const FBalhwajeomTutorialStep&,
	NewStep
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBalhwajeomTutorialFlowFinished);


/**
 * Runs one UBalhwajeomTutorialFlow for the level it is placed in.
 *
 * The director owns nothing but story state tags: it applies the flow's locks, advances
 * through the steps as their FGameplayTagQuery conditions become true, and exposes the
 * current step's presentation intent. Gameplay systems never learn about it -- the photo
 * camera, tablet and doors only read tags.
 *
 * Place one per level and assign a Flow. Everything else is data.
 */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomTutorialDirector : public AActor
{
	GENERATED_BODY()

public:
	ABalhwajeomTutorialDirector();

	/** The first director found in the world, or null. Safe to call from widgets. */
	UFUNCTION(BlueprintPure, Category = "Tutorial",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Get Tutorial Director"))
	static ABalhwajeomTutorialDirector* GetTutorialDirector(const UObject* WorldContextObject);

	/**
	 * Swaps the flow before it starts, so one placed director can run a different script
	 * per chapter or difficulty. Ignored once a flow is running.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetFlow(UBalhwajeomTutorialFlow* InFlow);

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	UBalhwajeomTutorialFlow* GetFlow() const { return Flow; }

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartFlow();

	/** Skips to the next step regardless of the current step's condition. Debug and cutscene use. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AdvanceStep();

	/**
	 * Ends the flow immediately and releases every lock the flow applied.
	 * Use it to skip the tutorial, and as the recovery path if a step ever fails to complete.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AbortFlow();

	/** Applies or clears one lock tag directly, independent of the flow's steps. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetLockActive(FGameplayTag LockTag, bool bActive);

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsFlowActive() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	FName GetCurrentStepID() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetCurrentStepIndex() const { return CurrentStepIndex; }

	/** Returns false when no step is active, which is also the case before StartFlow. */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool GetCurrentStep(FBalhwajeomTutorialStep& OutStep) const;

	/** Fires on every step entry, including the first. */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnBalhwajeomTutorialStepChanged OnStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnBalhwajeomTutorialFlowFinished OnFlowFinished;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	TObjectPtr<UBalhwajeomTutorialFlow> Flow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	bool bAutoStartOnBeginPlay = true;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FBalhwajeomTutorialDirectorTestAccessor;
#endif

	UFUNCTION()
	void HandleStateTagChanged(FGameplayTag StateTag);

	void SubscribeToStoryState();
	void UnsubscribeFromStoryState();

	/**
	 * Applies the step's tag changes and presentation, then evaluates it once so a step whose
	 * condition already holds is skipped. That fast-forward is what lets a flow be started
	 * mid-progress -- after a reload, a debug restart, or a level entered out of order.
	 */
	void EnterStep(int32 StepIndex);
	void EvaluateCurrentStep();
	void FinishFlow();

	/** Started in EnterStep when the step's AutoAdvanceAfterSeconds is greater than zero. */
	void HandleAutoAdvanceTimer();
	void ClearAutoAdvanceTimer();

	UStoryStateSubsystem* GetStoryState() const;
	const FBalhwajeomTutorialStep* GetCurrentStepPtr() const;

	/** True when every non-empty condition on the step is satisfied. */
	bool IsStepSatisfied(const FBalhwajeomTutorialStep& Step) const;

	int32 CurrentStepIndex = INDEX_NONE;

	/** World time the current step began, so its blink starts bright. */
	double StepEnteredTimeSeconds = 0.0;
	bool bFlowStarted = false;
	bool bSubscribed = false;

	/**
	 * Entering a step edits tags, which re-enters evaluation through the story state delegate.
	 * The guard defers that until the entry finishes, so a chain of already-satisfied steps
	 * resolves as a loop instead of unbounded recursion.
	 */
	bool bIsEnteringStep = false;
	bool bPendingEvaluation = false;

	FTimerHandle AutoAdvanceTimerHandle;
};
