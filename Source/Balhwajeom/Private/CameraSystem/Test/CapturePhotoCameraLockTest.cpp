#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCapturePhotoPresentationState.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapturePhotoCameraLockTest,
	"Balhwajeom.Camera.CapturePhotoCameraLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapturePhotoCameraLockTest::RunTest(const FString& Parameters)
{
	// The card has to be readable before it can be acknowledged, so the camera cannot be
	// raised or lowered out from under it.
	TestTrue(TEXT("an entering card pins the camera mode"),
		LocksCameraMode(ECapturePhotoPresentationPhase::Entering));
	TestTrue(TEXT("a card awaiting confirmation pins the camera mode"),
		LocksCameraMode(ECapturePhotoPresentationPhase::AwaitingConfirmation));

	// This is the boundary the auto-exit depends on. Releasing at Exiting is what lets the
	// camera transition run underneath the card as it flies away, so the player lands in
	// the third-person view as the card clears instead of watching the camera lower
	// afterwards. Moving this to Completed brings back that extra step.
	TestFalse(TEXT("a leaving card releases the camera mode"),
		LocksCameraMode(ECapturePhotoPresentationPhase::Exiting));

	TestFalse(TEXT("a completed card releases the camera mode"),
		LocksCameraMode(ECapturePhotoPresentationPhase::Completed));
	TestFalse(TEXT("no card at all leaves the camera mode free"),
		LocksCameraMode(ECapturePhotoPresentationPhase::Inactive));

	// Driving the real state machine, the release has to land exactly on confirmation.
	FCapturePhotoPresentationState State;
	constexpr double StartTime = 4.0;
	constexpr float EntryCompletionTime = 50.0f / 60.0f;
	constexpr float ExitStartTime = 68.0f / 60.0f;
	constexpr float TotalDuration = 93.0f / 60.0f;
	constexpr float PromptFadeDuration = 10.0f / 60.0f;
	State.Start(StartTime, EntryCompletionTime, ExitStartTime, TotalDuration, PromptFadeDuration);

	TestTrue(TEXT("the camera stays pinned while the card enters"),
		LocksCameraMode(State.GetPhase()));

	State.Update(StartTime + EntryCompletionTime);
	TestTrue(TEXT("the camera stays pinned while the card waits"),
		LocksCameraMode(State.GetPhase()));

	const double ConfirmTime = StartTime + EntryCompletionTime + 5.0 / 60.0;
	TestTrue(TEXT("the card accepts the confirmation"), State.TryConfirm(ConfirmTime));
	TestFalse(TEXT("the camera is released on the confirmation frame itself"),
		LocksCameraMode(State.GetPhase()));

	State.Update(ConfirmTime + (TotalDuration - ExitStartTime));
	TestEqual(TEXT("the card completes after its exit"),
		State.GetPhase(), ECapturePhotoPresentationPhase::Completed);
	TestFalse(TEXT("the camera stays released once the card is gone"),
		LocksCameraMode(State.GetPhase()));

	return !HasAnyErrors();
}

#endif
