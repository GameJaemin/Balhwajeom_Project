#include "BalhwajeomCapturePhotoPresentationState.h"


bool LocksCameraMode(const ECapturePhotoPresentationPhase Phase)
{
	return Phase == ECapturePhotoPresentationPhase::Entering ||
		Phase == ECapturePhotoPresentationPhase::AwaitingConfirmation;
}

void FCapturePhotoPresentationState::Start(
	const double Now,
	const float InEntryCompletionTime,
	const float InExitStartTime,
	const float InTotalDuration,
	const float InPromptFadeDuration)
{
	TotalDuration = FMath::Max(InTotalDuration, KINDA_SMALL_NUMBER);
	EntryCompletionTime = FMath::Clamp(InEntryCompletionTime, 0.0f, TotalDuration);
	ExitStartTime = FMath::Clamp(InExitStartTime, EntryCompletionTime, TotalDuration);
	PromptFadeDuration = FMath::Max(InPromptFadeDuration, KINDA_SMALL_NUMBER);
	PresentationStartTime = Now;
	PhaseStartTime = Now;
	PromptFadeOutStartOpacity = 0.0f;
	Phase = ECapturePhotoPresentationPhase::Entering;
}

void FCapturePhotoPresentationState::Update(const double Now)
{
	if (Phase == ECapturePhotoPresentationPhase::Entering &&
		Now - PresentationStartTime >= EntryCompletionTime)
	{
		Phase = ECapturePhotoPresentationPhase::AwaitingConfirmation;
		PhaseStartTime = PresentationStartTime + EntryCompletionTime;
	}
	else if (Phase == ECapturePhotoPresentationPhase::Exiting &&
		Now - PhaseStartTime >= TotalDuration - ExitStartTime)
	{
		Phase = ECapturePhotoPresentationPhase::Completed;
	}
}

bool FCapturePhotoPresentationState::TryConfirm(const double Now)
{
	Update(Now);
	if (Phase != ECapturePhotoPresentationPhase::AwaitingConfirmation)
	{
		return false;
	}

	PromptFadeOutStartOpacity = GetPromptOpacity(Now);
	Phase = ECapturePhotoPresentationPhase::Exiting;
	PhaseStartTime = Now;
	return true;
}

void FCapturePhotoPresentationState::Reset()
{
	Phase = ECapturePhotoPresentationPhase::Inactive;
	PresentationStartTime = 0.0;
	PhaseStartTime = 0.0;
	EntryCompletionTime = 0.0f;
	ExitStartTime = 0.0f;
	TotalDuration = 0.0f;
	PromptFadeDuration = 0.0f;
	PromptFadeOutStartOpacity = 0.0f;
}

float FCapturePhotoPresentationState::GetTimelineTime(const double Now) const
{
	switch (Phase)
	{
	case ECapturePhotoPresentationPhase::Entering:
		return FMath::Clamp(
			static_cast<float>(Now - PresentationStartTime),
			0.0f,
			EntryCompletionTime);
	case ECapturePhotoPresentationPhase::AwaitingConfirmation:
		return EntryCompletionTime;
	case ECapturePhotoPresentationPhase::Exiting:
		return FMath::Clamp(
			ExitStartTime + static_cast<float>(Now - PhaseStartTime),
			ExitStartTime,
			TotalDuration);
	case ECapturePhotoPresentationPhase::Completed:
		return TotalDuration;
	case ECapturePhotoPresentationPhase::Inactive:
	default:
		return 0.0f;
	}
}

float FCapturePhotoPresentationState::GetPromptOpacity(const double Now) const
{
	switch (Phase)
	{
	case ECapturePhotoPresentationPhase::AwaitingConfirmation:
		return FMath::Clamp(
			static_cast<float>(Now - PhaseStartTime) / PromptFadeDuration,
			0.0f,
			1.0f);
	case ECapturePhotoPresentationPhase::Exiting:
		return PromptFadeOutStartOpacity *
			(1.0f - FMath::Clamp(
				static_cast<float>(Now - PhaseStartTime) / PromptFadeDuration,
				0.0f,
				1.0f));
	case ECapturePhotoPresentationPhase::Inactive:
	case ECapturePhotoPresentationPhase::Entering:
	case ECapturePhotoPresentationPhase::Completed:
	default:
		return 0.0f;
	}
}
