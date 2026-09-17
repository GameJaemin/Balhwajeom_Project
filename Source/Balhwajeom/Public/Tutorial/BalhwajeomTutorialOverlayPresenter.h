#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BalhwajeomTutorialOverlayPresenter.generated.h"

class FTutorialOverlayInputProcessor;
class UBalhwajeomTutorialOverlayWidget;
class UDataTable;


/**
 * Shows DT_TutorialOverlay rows when their moment arrives.
 *
 * Lives on the player controller because it needs a viewport, an input mode and the
 * local player's story state. The controller only has to create it; everything about
 * which screen is due, and when, stays here.
 *
 * Rows become due on a story state tag being ADDED, never by polling, so a moment that
 * comes and goes -- the tablet being open, for instance -- can be spent without the
 * overlay firing later out of context.
 */
UCLASS(ClassGroup = (Balhwajeom), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomTutorialOverlayPresenter : public UActorComponent
{
	GENERATED_BODY()

public:
	UBalhwajeomTutorialOverlayPresenter();

	/** True while an overlay is on screen, including both fades. */
	UFUNCTION(BlueprintPure, Category = "Tutorial Overlay")
	bool IsOverlayOnScreen() const;

	/** Starts the fade out, if the input lock has already expired. */
	void RequestDismiss();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay")
	TSoftObjectPtr<UDataTable> OverlayTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay")
	TSoftClassPtr<UBalhwajeomTutorialOverlayWidget> OverlayWidgetClass;

	/** Above every other layer in the project; the interaction modal is the next at 1300. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay")
	int32 ViewportZOrder = 2000;

	/** Seconds of forced reading before any key closes the screen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial Overlay",
		meta = (ClampMin = "0.0"))
	float InputLockSeconds = 2.0f;

private:
	UFUNCTION()
	void HandleStateTagAdded(FGameplayTag StateTag);

	/** Adds the gameplay-started trigger when the level has no intro actor to add it. */
	void StartWithoutIntroIfNeeded();

	void CollectDueRows();
	void ShowNextQueuedRow();
	void HandleFadeOutFinished();
	void FinishCurrentOverlay();
	void ApplyInputBlock(bool bBlocked);

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomTutorialOverlayWidget> OverlayWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedOverlayTable;

	TSharedPtr<FTutorialOverlayInputProcessor> InputProcessor;

	/** Due rows waiting their turn. One tag can complete two rows at once. */
	TArray<FName> QueuedRowNames;

	FGameplayTag ActiveCompletionTag;
	FName ActiveRowName;

	bool bInputBlockApplied = false;
	bool bRestoreMoveInput = false;
	bool bRestoreLookInput = false;
	bool bCheckedForIntroActor = false;
};
