#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomPhotoCaptureAvailability.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhotoCaptureAvailabilityTest,
	"Balhwajeom.Camera.CaptureAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhotoCaptureAvailabilityTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomPhotoCaptureAvailability;

	// Argument order: resolved, within the search radius, state allows capture, names a
	// photo, already captured.
	TestTrue(TEXT("a resolved, nearby, capturable, unshot target is available"),
		IsCapturableNow(true, true, true, true, false));

	// The scan decides whether to lower the camera, so every one of these has to read as
	// unavailable or the camera would stay up with nothing to shoot.
	TestFalse(TEXT("an unresolved target is not available"),
		IsCapturableNow(false, true, true, true, false));
	TestFalse(TEXT("a state that does not allow capture is not available"),
		IsCapturableNow(true, true, false, true, false));
	TestFalse(TEXT("a state with no photo to record is not available"),
		IsCapturableNow(true, true, true, false, false));
	TestFalse(TEXT("an already photographed target is not available"),
		IsCapturableNow(true, true, true, true, true));

	// Beyond the radius there is nothing nearby worth keeping the camera up for; the
	// player has to walk before it matters again.
	TestFalse(TEXT("a target beyond the search radius is not available"),
		IsCapturableNow(true, false, true, true, false));

	// Being nearby is not on its own enough; the state still has to be shootable.
	TestFalse(TEXT("proximity alone does not make a target available"),
		IsCapturableNow(true, true, false, false, false));

	// This combination is real data, not a hypothetical: the pre-interaction states of
	// the diary and the four notes each carry a PhotoID while bCanCapture is still off.
	TestFalse(TEXT("a PhotoID alone does not make a state capturable"),
		IsCapturableNow(true, true, false, true, false));

	// Progression-locked evidence arrives here as an unresolved target, which is what
	// makes the camera lower once the current phase is cleared.
	TestFalse(TEXT("a progression-locked target is not available"),
		IsCapturableNow(false, true, true, true, false));

	return !HasAnyErrors();
}

#endif
