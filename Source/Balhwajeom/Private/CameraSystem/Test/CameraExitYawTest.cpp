#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCameraExitYaw.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCameraExitExplorationYawTest,
	"Balhwajeom.Camera.CameraExitYaw.ExplorationYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraExitExplorationYawTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomCameraExitYaw;

	constexpr float Tolerance = KINDA_SMALL_NUMBER;

	// The regression this whole subtraction exists for. Entry turned the view by
	// AlignmentDelta to keep the screen-centre object framed; if the player never looked
	// around, the exit has to give the pre-entry yaw back exactly, or repeated RMB presses
	// walk the third-person view around the room.
	{
		constexpr float SavedYaw = 30.0f;
		constexpr float AlignmentDelta = 5.0f;
		TestEqual(TEXT("an untouched camera trip restores the entry yaw exactly"),
			ResolveExplorationYaw(SavedYaw + AlignmentDelta, AlignmentDelta),
			SavedYaw, Tolerance);
	}

	// Ten round trips in a row, the way a player actually presses RMB. Any leak per trip
	// would compound here.
	{
		constexpr float AlignmentDelta = -7.5f;
		float Yaw = 120.0f;
		for (int32 Trip = 0; Trip < 10; ++Trip)
		{
			Yaw = ResolveExplorationYaw(Yaw + AlignmentDelta, AlignmentDelta);
		}
		TestEqual(TEXT("ten round trips leave the exploration yaw where it started"),
			Yaw, 120.0f, Tolerance);
	}

	// What the player looked around by is theirs and comes back out with them. Without
	// this the walk direction changes at the exact moment the camera comes down.
	{
		constexpr float SavedYaw = 30.0f;
		constexpr float AlignmentDelta = 5.0f;
		constexpr float LookedAroundBy = 40.0f;
		TestEqual(TEXT("look input survives the exit"),
			ResolveExplorationYaw(SavedYaw + AlignmentDelta + LookedAroundBy, AlignmentDelta),
			SavedYaw + LookedAroundBy, Tolerance);
	}

	// An unaligned entry (nothing under the screen centre to trace against) records a zero
	// delta, so the exit is a pass-through.
	TestEqual(TEXT("a zero alignment delta passes the yaw straight through"),
		ResolveExplorationYaw(64.0f, 0.0f), 64.0f, Tolerance);

	// Crossing the wrap point must not produce a 350-degree control rotation.
	TestEqual(TEXT("the result is unwound across the 180 degree seam"),
		ResolveExplorationYaw(170.0f, -30.0f), -160.0f, Tolerance);

	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCameraExitYawSettleTest,
	"Balhwajeom.Camera.CameraExitYaw.Settle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraExitYawSettleTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomCameraExitYaw;

	constexpr float Tolerance = KINDA_SMALL_NUMBER;
	constexpr float Ease = 0.6f;

	// Landing exactly is the whole contract. A remainder left here is a turn the movement
	// component finishes after the fade clears, which is the swing being removed.
	TestEqual(TEXT("the final step lands on the target"),
		StepSettleYaw(0.0f, 90.0f, 1.0f / 60.0f, 1.0f / 60.0f, Ease), 90.0f, Tolerance);
	TestEqual(TEXT("an overrun step lands on the target"),
		StepSettleYaw(0.0f, 90.0f, 0.5f, 0.01f, Ease), 90.0f, Tolerance);

	// A spent or degenerate schedule snaps instead of dividing by it.
	TestEqual(TEXT("no remaining time snaps to the target"),
		StepSettleYaw(0.0f, 90.0f, 1.0f / 60.0f, 0.0f, Ease), 90.0f, Tolerance);
	TestEqual(TEXT("negative remaining time snaps to the target"),
		StepSettleYaw(0.0f, 90.0f, 1.0f / 60.0f, -1.0f, Ease), 90.0f, Tolerance);

	// Front-loading is what hides the turn: the same frame covers more ground with the
	// eased exponent than at constant speed, and the fade is darkest at the start.
	{
		const float Eased = StepSettleYaw(0.0f, 90.0f, 1.0f / 60.0f, 0.25f, Ease);
		const float Linear = StepSettleYaw(0.0f, 90.0f, 1.0f / 60.0f, 0.25f, 1.0f);
		TestTrue(TEXT("the eased first step outruns the constant-speed one"), Eased > Linear);
		TestTrue(TEXT("the eased first step still stops short of the target"), Eased < 90.0f);
	}

	// Driving the real schedule: half a default CameraTransitionDuration at 60Hz.
	{
		constexpr float DeltaSeconds = 1.0f / 60.0f;
		constexpr float TargetYaw = 150.0f;
		float Yaw = -30.0f;
		float Remaining = 0.25f;
		float PreviousYaw = Yaw;

		for (int32 Step = 0; Step < 15; ++Step)
		{
			Yaw = StepSettleYaw(Yaw, TargetYaw, DeltaSeconds, Remaining, Ease);
			Remaining -= DeltaSeconds;

			TestTrue(TEXT("the settle advances toward the target every step"),
				Yaw >= PreviousYaw - Tolerance);
			TestTrue(TEXT("the settle never overshoots the target"),
				Yaw <= TargetYaw + Tolerance);
			PreviousYaw = Yaw;
		}

		TestEqual(TEXT("the settle has arrived once the fade-in is spent"),
			Yaw, TargetYaw, Tolerance);
	}

	// The short way around. A naive lerp would take this 340 degrees the wrong way and
	// spin the character a full turn in front of the player.
	{
		const float Stepped = StepSettleYaw(350.0f, 10.0f, 1.0f / 60.0f, 0.25f, Ease);
		const float Travelled = FMath::UnwindDegrees(Stepped - 350.0f);
		TestTrue(TEXT("a seam-crossing settle turns the short way"), Travelled > 0.0f);
		TestTrue(TEXT("a seam-crossing settle stays within the 20 degree gap"),
			Travelled < 20.0f + Tolerance);
	}

	TestEqual(TEXT("a seam-crossing settle finishes on the target"),
		StepSettleYaw(350.0f, 10.0f, 0.25f, 0.25f, Ease), 10.0f, Tolerance);

	// Standing still is handled by the caller refusing to step at all, but a zero-length
	// step must still be inert rather than drifting.
	TestEqual(TEXT("a zero-length step holds the current yaw"),
		StepSettleYaw(45.0f, 90.0f, 0.0f, 0.25f, Ease), 45.0f, Tolerance);

	return !HasAnyErrors();
}

#endif
