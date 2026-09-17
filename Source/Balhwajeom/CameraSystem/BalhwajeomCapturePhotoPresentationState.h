#pragma once

#include "CoreMinimal.h"

enum class ECapturePhotoPresentationPhase : uint8
{
	Inactive,
	Entering,
	AwaitingConfirmation,
	Exiting,
	Completed
};

/**
 * Whether the camera mode is pinned while the capture card is in this phase.
 *
 * The card has to be readable before it can be acknowledged, so raising or lowering the
 * camera is refused until the player confirms. From Exiting onward the exit is free,
 * which is what lets the camera transition run underneath the card as it flies away
 * instead of starting after it is gone.
 */
BALHWAJEOM_API bool LocksCameraMode(ECapturePhotoPresentationPhase Phase);


/** Time-only state for the capture result presentation. */
class BALHWAJEOM_API FCapturePhotoPresentationState
{
public:
	void Start(
		double Now,
		float InEntryCompletionTime,
		float InExitStartTime,
		float InTotalDuration,
		float InPromptFadeDuration);
	void Update(double Now);
	bool TryConfirm(double Now);
	void Reset();

	bool IsActive() const { return Phase != ECapturePhotoPresentationPhase::Inactive; }
	ECapturePhotoPresentationPhase GetPhase() const { return Phase; }
	float GetTimelineTime(double Now) const;
	float GetPromptOpacity(double Now) const;

private:
	ECapturePhotoPresentationPhase Phase = ECapturePhotoPresentationPhase::Inactive;
	double PresentationStartTime = 0.0;
	double PhaseStartTime = 0.0;
	float EntryCompletionTime = 0.0f;
	float ExitStartTime = 0.0f;
	float TotalDuration = 0.0f;
	float PromptFadeDuration = 0.0f;
	float PromptFadeOutStartOpacity = 0.0f;
};
