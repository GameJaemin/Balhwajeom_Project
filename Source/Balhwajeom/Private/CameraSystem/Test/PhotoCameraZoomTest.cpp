#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomPhotoCameraZoom.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhotoCameraZoomAlphaTest,
	"Balhwajeom.Camera.ZoomBar.Alpha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhotoCameraZoomAlphaTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomPhotoCameraZoom;

	// The shipped bounds: 35 degrees is fully zoomed in, 90 fully out.
	constexpr float Narrowest = 35.0f;
	constexpr float Widest = 90.0f;
	constexpr float Tolerance = KINDA_SMALL_NUMBER;

	TestEqual(TEXT("the widest field of view reads as no zoom"),
		ResolveZoomAlpha(Widest, Narrowest, Widest), 0.0f, Tolerance);
	TestEqual(TEXT("the narrowest field of view reads as full zoom"),
		ResolveZoomAlpha(Narrowest, Narrowest, Widest), 1.0f, Tolerance);
	TestEqual(TEXT("the middle of the range reads as half zoom"),
		ResolveZoomAlpha(62.5f, Narrowest, Widest), 0.5f, Tolerance);

	// A field of view outside the bounds only happens if something else moved it.
	TestEqual(TEXT("a wider field of view than the bound still reads as no zoom"),
		ResolveZoomAlpha(120.0f, Narrowest, Widest), 0.0f, Tolerance);
	TestEqual(TEXT("a narrower field of view than the bound still reads as full zoom"),
		ResolveZoomAlpha(10.0f, Narrowest, Widest), 1.0f, Tolerance);

	// ZoomCamera() sorts the two bounds before clamping, so this has to agree with it
	// rather than depend on which property holds the smaller number.
	TestEqual(TEXT("swapped bounds give the same answer"),
		ResolveZoomAlpha(62.5f, Widest, Narrowest), 0.5f, Tolerance);

	// A hand-edited asset can collapse the range; reporting 0 beats dividing by it.
	TestEqual(TEXT("a collapsed range reads as no zoom"),
		ResolveZoomAlpha(50.0f, 50.0f, 50.0f), 0.0f, Tolerance);

	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhotoCameraZoomBarPositionTest,
	"Balhwajeom.Camera.ZoomBar.Position",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhotoCameraZoomBarPositionTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomPhotoCameraZoom;

	// The authored endpoints of the bar's travel in WBP_CAM.
	constexpr float MinZoom = 97.0f;
	constexpr float MaxZoom = -103.0f;
	constexpr float Tolerance = KINDA_SMALL_NUMBER;

	TestEqual(TEXT("no zoom parks the bar at the bottom of its travel"),
		ResolveZoomBarPosition(0.0f, MinZoom, MaxZoom), 97.0f, Tolerance);
	TestEqual(TEXT("full zoom parks the bar at the top of its travel"),
		ResolveZoomBarPosition(1.0f, MinZoom, MaxZoom), -103.0f, Tolerance);
	TestEqual(TEXT("half zoom lands halfway between the two"),
		ResolveZoomBarPosition(0.5f, MinZoom, MaxZoom), -3.0f, Tolerance);

	TestEqual(TEXT("alpha below the range stays at the bottom"),
		ResolveZoomBarPosition(-1.0f, MinZoom, MaxZoom), 97.0f, Tolerance);
	TestEqual(TEXT("alpha above the range stays at the top"),
		ResolveZoomBarPosition(2.0f, MinZoom, MaxZoom), -103.0f, Tolerance);

	// Zooming in must always move the bar the same way, never reverse partway.
	float Previous = ResolveZoomBarPosition(0.0f, MinZoom, MaxZoom);
	for (int32 Step = 1; Step <= 10; ++Step)
	{
		const float Current = ResolveZoomBarPosition(Step / 10.0f, MinZoom, MaxZoom);
		if (!TestTrue(TEXT("the bar rises monotonically as zoom increases"), Current < Previous))
		{
			break;
		}
		Previous = Current;
	}

	// Swapping the endpoints has to keep working, so the direction can be flipped from
	// the details panel without touching this code.
	TestEqual(TEXT("swapped endpoints run the bar the other way"),
		ResolveZoomBarPosition(1.0f, MaxZoom, MinZoom), 97.0f, Tolerance);

	return !HasAnyErrors();
}

#endif
