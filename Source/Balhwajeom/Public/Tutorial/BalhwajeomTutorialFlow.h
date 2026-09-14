#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BalhwajeomTutorialFlow.generated.h"


/** HUD element a tutorial step asks the tutorial focus layer to raise above the dim. */
UENUM(BlueprintType)
enum class EBalhwajeomTutorialHintTarget : uint8
{
	/** Nothing pulses. The [F] prompt still stays readable because of its ZOrder. */
	None,
	PhotoCameraIcon,
	TabletIcon,

	/** The [F] interaction prompt itself, which only appears while looking at a target. */
	InteractPrompt
};


/** How a tutorial step darkens the screen. */
UENUM(BlueprintType)
enum class EBalhwajeomTutorialDimMode : uint8
{
	/** No dim. */
	Off,

	/** Dim for the whole step. Use this to point at a HUD icon. */
	Always,

	/**
	 * Dim only while the [F] interaction prompt is faded in, and exactly as much.
	 * The prompt appears only when the player is looking at a close, interactable
	 * target, so this is the "highlight what I am looking at" mode.
	 */
	FollowInteractPrompt
};


USTRUCT(BlueprintType)
struct BALHWAJEOM_API FBalhwajeomTutorialStep
{
	GENERATED_BODY()

	/** Identifies the step in logs and in AdvanceStep debugging. Not shown to players. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step")
	FName StepID = NAME_None;

	/**
	 * Optional exclusive stage tag set on entry, under the flow's StageRootTag.
	 * Other systems (conditional inspection text, Blueprints) can read it.
	 * Leave empty when the step needs no externally visible stage.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step")
	FGameplayTag StageTag;

	/** Story state tags added when this step begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step")
	FGameplayTagContainer GrantOnEnter;

	/** Story state tags removed when this step begins. Put Runtime.Lock.* here to unlock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step")
	FGameplayTagContainer RemoveOnEnter;

	/** The step ends once the story state holds every one of these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step")
	FGameplayTagContainer CompleteWhenAllTags;

	/** The step ends once the story state holds at least one of these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step")
	FGameplayTagContainer CompleteWhenAnyTags;

	/**
	 * Escape hatch for conditions the two containers cannot express, such as a NOT.
	 * All three conditions are combined with AND; each empty one is simply skipped.
	 *
	 * A step with no condition at all only ends through AdvanceStep, so the last step
	 * of a flow normally leaves every one of them empty.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step", AdvancedDisplay)
	FGameplayTagQuery CompleteWhen;

	bool HasCompletionCondition() const
	{
		return !CompleteWhenAllTags.IsEmpty() ||
			!CompleteWhenAnyTags.IsEmpty() ||
			!CompleteWhen.IsEmpty();
	}

	/**
	 * When greater than zero, the step advances on its own after this many seconds,
	 * regardless of the tag conditions above -- the same escape hatch AdvanceStep() gives
	 * a designer, just timed instead of manual. A step can combine this with a tag
	 * condition; whichever is satisfied first wins. Zero (the default) means no timer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step",
		meta = (ClampMin = "0.0", Units = "s"))
	float AutoAdvanceAfterSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step|Presentation")
	EBalhwajeomTutorialDimMode DimMode = EBalhwajeomTutorialDimMode::Off;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step|Presentation")
	EBalhwajeomTutorialHintTarget HintTarget = EBalhwajeomTutorialHintTarget::None;

	/**
	 * Optional line shown next to HintTarget's highlighted icon, for example explaining a
	 * key. Empty shows nothing, matching every existing step (icon-only, no text).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step|Presentation")
	FText HintMessage;

	/** Dim strength at full opacity. FollowInteractPrompt scales this by the prompt's alpha. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Step|Presentation",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DimOpacity = 0.65f;
};


/**
 * An ordered tutorial or staged-progression script for one level.
 *
 * Everything a level needs is authored here, so reusing this system in another map
 * means creating another one of these assets and placing one ABalhwajeomTutorialDirector.
 * No C++ or widget changes are required.
 */
UCLASS(BlueprintType)
class BALHWAJEOM_API UBalhwajeomTutorialFlow : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Locks applied when the flow starts, for example Runtime.Lock.PhotoCamera.
	 * A lock tag being present is what blocks an ability, so abilities left out of this
	 * list stay available and levels without a director are entirely unaffected.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Flow")
	FGameplayTagContainer LocksOnStart;

	/**
	 * Exclusive group the steps' StageTag values belong to, for example Tutorial.Stage.
	 * Only needed when at least one step sets a StageTag.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Flow")
	FGameplayTag StageRootTag;

	/**
	 * Removes any LocksOnStart tag still active once the flow finishes.
	 * On by default: a lock surviving a finished flow leaves the player permanently
	 * unable to use an ability, which is far worse than an unlock arriving early.
	 * Turn it off only for a lock that is meant to outlive this flow.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Flow")
	bool bReleaseLocksOnFinish = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Flow")
	TArray<FBalhwajeomTutorialStep> Steps;
};
