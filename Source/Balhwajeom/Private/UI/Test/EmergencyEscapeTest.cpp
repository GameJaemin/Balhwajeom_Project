#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/BalhwajeomEmergencyEscape.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmergencyEscapeDoorChoiceTest,
	"Balhwajeom.UI.EmergencyEscape.DoorChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyEscapeDoorChoiceTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomEmergencyEscape;

	// A level with no gate door (the intro map) must fall through to the caller's own
	// fallback rather than producing a destination at the origin.
	TestEqual(TEXT("no doors leaves no choice"),
		ResolveNearestDoorIndex(FVector::ZeroVector, {}), INDEX_NONE);

	TestEqual(TEXT("a single door is always the choice"),
		ResolveNearestDoorIndex(FVector(5000.0f, 5000.0f, 0.0f), {FVector(1.0f, 2.0f, 3.0f)}), 0);

	{
		const TArray<FVector> Doors = {
			FVector(1000.0f, 0.0f, 0.0f),
			FVector(100.0f, 0.0f, 0.0f),
			FVector(500.0f, 0.0f, 0.0f)
		};
		TestEqual(TEXT("the nearest door wins regardless of order"),
			ResolveNearestDoorIndex(FVector(120.0f, 0.0f, 0.0f), Doors), 1);
		TestEqual(TEXT("moving the player moves the choice"),
			ResolveNearestDoorIndex(FVector(980.0f, 0.0f, 0.0f), Doors), 0);
	}

	// Actor iteration order is not guaranteed, so an exact tie has to resolve the same way
	// every run or the escape would send the player somewhere different each time.
	{
		const TArray<FVector> Tied = {
			FVector(-200.0f, 0.0f, 0.0f),
			FVector(200.0f, 0.0f, 0.0f)
		};
		TestEqual(TEXT("a tie takes the lower index"),
			ResolveNearestDoorIndex(FVector::ZeroVector, Tied), 0);
	}

	// Height matters: a door directly overhead is not a way out of being stuck here.
	{
		const TArray<FVector> Doors = {
			FVector(0.0f, 0.0f, 900.0f),
			FVector(300.0f, 0.0f, 0.0f)
		};
		TestEqual(TEXT("distance is measured in three dimensions"),
			ResolveNearestDoorIndex(FVector::ZeroVector, Doors), 1);
	}

	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmergencyEscapeTransformTest,
	"Balhwajeom.UI.EmergencyEscape.Transform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmergencyEscapeTransformTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomEmergencyEscape;

	constexpr float Tolerance = 0.01f;
	const FVector Offset(150.0f, 0.0f, 0.0f);

	// An unrotated door: straight out along its own +X, facing along it.
	{
		const FTransform Door(FRotator::ZeroRotator, FVector(1000.0f, 500.0f, 0.0f));
		const FTransform Escape = ResolveEscapeTransform(Door, Offset, 0.0f);
		TestTrue(TEXT("the spot sits one offset from the door"),
			Escape.GetLocation().Equals(FVector(1150.0f, 500.0f, 0.0f), Tolerance));
		TestEqual(TEXT("a zero local yaw looks along the door's own facing"),
			static_cast<float>(FMath::UnwindDegrees(Escape.Rotator().Yaw)), 0.0f, Tolerance);
	}

	// The offset is in the door's own space, so a rotated door carries it around with it.
	{
		const FTransform Door(FRotator(0.0f, 90.0f, 0.0f), FVector::ZeroVector);
		const FTransform Escape = ResolveEscapeTransform(Door, Offset, 0.0f);
		TestTrue(TEXT("a rotated door rotates its offset"),
			Escape.GetLocation().Equals(FVector(0.0f, 150.0f, 0.0f), Tolerance));
		TestEqual(TEXT("the facing follows the rotated door"),
			static_cast<float>(FMath::UnwindDegrees(Escape.Rotator().Yaw)), 90.0f, Tolerance);
	}

	// The local yaw is added to the door's, and wraps rather than running past 180.
	{
		const FTransform Door(FRotator(0.0f, 90.0f, 0.0f), FVector::ZeroVector);
		const FTransform Escape = ResolveEscapeTransform(Door, Offset, 180.0f);
		TestEqual(TEXT("a local yaw turns the arrival around"),
			static_cast<float>(FMath::UnwindDegrees(Escape.Rotator().Yaw)), -90.0f, Tolerance);
	}

	// Pitch and roll must never reach the capsule, however the door was placed.
	{
		const FTransform Door(FRotator(35.0f, 20.0f, -15.0f), FVector(10.0f, 20.0f, 30.0f));
		const FTransform Escape = ResolveEscapeTransform(Door, Offset, 0.0f);
		TestEqual(TEXT("the escape rotation has no pitch"),
			static_cast<float>(Escape.Rotator().Pitch), 0.0f, Tolerance);
		TestEqual(TEXT("the escape rotation has no roll"),
			static_cast<float>(Escape.Rotator().Roll), 0.0f, Tolerance);
	}

	// room3's own numbers, so the shipped default is checked rather than an invented case.
	// GateDoor_Exit sits on its hinge at (-553.6, -1771.3, 2.8) with no rotation; its panel
	// centres 70cm away in Y and the room is on the panel's -X side.
	{
		const FTransform GateDoorExit(FRotator::ZeroRotator, FVector(-553.6f, -1771.3f, 2.8f));
		const FTransform Escape =
			ResolveEscapeTransform(GateDoorExit, FVector(-150.0f, 70.0f, 100.0f), 0.0f);

		TestTrue(TEXT("the default lands in front of the doorway, centred on it"),
			Escape.GetLocation().Equals(FVector(-703.6f, -1701.3f, 102.8f), Tolerance));

		// Facing +X is facing the panel at X = -549.6 from X = -703.6.
		TestEqual(TEXT("the default arrival looks at the door"),
			static_cast<float>(FMath::UnwindDegrees(Escape.Rotator().Yaw)), 0.0f, Tolerance);

		// Standing height, not sunk into the floor: the level's PlayerStart sits at Z 102.
		TestTrue(TEXT("the default stands the capsule on the floor"),
			Escape.GetLocation().Z > 95.0f && Escape.GetLocation().Z < 110.0f);
	}

	return !HasAnyErrors();
}

#endif
