#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCapturePhotoMovementLock.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapturePhotoMovementLockTest,
	"Balhwajeom.Camera.CapturePhotoMovementLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapturePhotoMovementLockTest::RunTest(const FString& Parameters)
{
	FCapturePhotoMovementLock Lock;
	TestFalse(TEXT("a fresh lock holds nothing"), Lock.IsLocked());

	TestTrue(TEXT("the first acquire asks the caller to ignore move input"), Lock.Acquire());
	TestTrue(TEXT("the lock is held after acquiring"), Lock.IsLocked());

	// SetIgnoreMoveInput() is a counter. A second acquire would push it to two and leave
	// the player unable to walk even after a matching release.
	TestFalse(TEXT("a repeated acquire does not raise the counter again"), Lock.Acquire());
	TestTrue(TEXT("the lock is still held"), Lock.IsLocked());

	TestTrue(TEXT("the first release hands movement back"), Lock.Release());
	TestFalse(TEXT("the lock is clear after releasing"), Lock.IsLocked());

	// The card is released from several places: the frame it starts leaving, its
	// teardown, and the malformed-layout timeout. Only the first may decrement, or the
	// extra release would steal movement from whatever else was holding it.
	TestFalse(TEXT("a repeated release does not lower the counter again"), Lock.Release());
	TestFalse(TEXT("the lock stays clear"), Lock.IsLocked());

	// A release with nothing held must never decrement, which is what keeps an
	// interaction modal's own lock intact when a capture never happened.
	FCapturePhotoMovementLock Untouched;
	TestFalse(TEXT("releasing a lock that was never acquired does nothing"),
		Untouched.Release());

	// Successive captures have to keep balancing.
	TestTrue(TEXT("the next capture can acquire again"), Lock.Acquire());
	TestTrue(TEXT("and release again"), Lock.Release());
	TestFalse(TEXT("ending balanced"), Lock.IsLocked());

	return !HasAnyErrors();
}

#endif
