#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GateDoorLockedFeedback.generated.h"


/** One ordered piece of guidance shown while a gate door remains locked. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FGateDoorLockedFeedbackStage
{
	GENERATED_BODY()

	/** This stage is complete only when every tag in this container is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback")
	FGameplayTagContainer CompleteWhenAllTags;

	/** Guidance shown while this is the first incomplete stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Door|Locked Feedback",
		meta = (MultiLine = "true"))
	FText IncompleteMessage;
};


namespace GateDoorLockedFeedback
{
	/** Returns the message for the first incomplete stage, or empty when all are done. */
	BALHWAJEOM_API FText ResolveFirstIncompleteMessage(
		const TArray<FGateDoorLockedFeedbackStage>& Stages,
		const FGameplayTagContainer& CurrentStateTags);

	/**
	 * Render opacity of a message that fades in, holds fully opaque, then fades out.
	 * A zero-length fade is applied instantly, and any elapsed time at or past the total
	 * duration returns 0, which is what tells the caller the message is finished.
	 */
	BALHWAJEOM_API float ResolveFadeOpacity(
		float ElapsedSeconds,
		float FadeInDuration,
		float HoldDuration,
		float FadeOutDuration);
}
