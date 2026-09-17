#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/BalhwajeomMainMenuHoverFade.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMainMenuHoverFadeCurveTest,
	"Balhwajeom.Title.MainMenu.HoverFade.Curve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMainMenuHoverFadeCurveTest::RunTest(const FString& Parameters)
{
	constexpr float Tolerance = KINDA_SMALL_NUMBER;

	TestEqual(TEXT("the curve starts at zero"),
		BalhwajeomMainMenuHoverFade::EaseInOut(0.0f), 0.0f, Tolerance);
	TestEqual(TEXT("the curve ends at one"),
		BalhwajeomMainMenuHoverFade::EaseInOut(1.0f), 1.0f, Tolerance);
	TestEqual(TEXT("the curve is symmetric about its midpoint"),
		BalhwajeomMainMenuHoverFade::EaseInOut(0.5f), 0.5f, Tolerance);

	// Out-of-range input comes from a hand-edited duration, not from AdvanceHoverAlpha.
	TestEqual(TEXT("input below the range clamps to zero"),
		BalhwajeomMainMenuHoverFade::EaseInOut(-1.0f), 0.0f, Tolerance);
	TestEqual(TEXT("input above the range clamps to one"),
		BalhwajeomMainMenuHoverFade::EaseInOut(2.0f), 1.0f, Tolerance);

	// This is what "no popping" means: the ends move slower than the middle, so the
	// fade eases in and out instead of starting and stopping at full speed.
	const float NearStart = BalhwajeomMainMenuHoverFade::EaseInOut(0.05f);
	const float MidLow = BalhwajeomMainMenuHoverFade::EaseInOut(0.475f);
	const float MidHigh = BalhwajeomMainMenuHoverFade::EaseInOut(0.525f);
	const float NearEnd = 1.0f - BalhwajeomMainMenuHoverFade::EaseInOut(0.95f);
	const float MiddleSlope = MidHigh - MidLow;
	TestTrue(TEXT("the fade eases in rather than starting at full speed"),
		NearStart < MiddleSlope);
	TestTrue(TEXT("the fade eases out rather than stopping at full speed"),
		NearEnd < MiddleSlope);

	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMainMenuHoverFadeAlphaTest,
	"Balhwajeom.Title.MainMenu.HoverFade.Alpha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMainMenuHoverFadeAlphaTest::RunTest(const FString& Parameters)
{
	constexpr float Duration = 0.2f;
	constexpr float Tolerance = KINDA_SMALL_NUMBER;

	TestEqual(TEXT("half the duration covers half the way in"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.0f, true, 0.1f, Duration),
		0.5f, Tolerance);
	TestEqual(TEXT("the full duration reaches the hovered end"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.0f, true, Duration, Duration),
		1.0f, Tolerance);
	TestEqual(TEXT("entering never overshoots"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.9f, true, Duration, Duration),
		1.0f, Tolerance);
	TestEqual(TEXT("leaving runs back toward the idle end"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(1.0f, false, 0.1f, Duration),
		0.5f, Tolerance);
	TestEqual(TEXT("leaving never undershoots"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.1f, false, Duration, Duration),
		0.0f, Tolerance);

	// A designer can switch the fade off, and a hand-edited asset can hold nonsense.
	TestEqual(TEXT("a zero duration snaps to hovered"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.0f, true, 0.016f, 0.0f),
		1.0f, Tolerance);
	TestEqual(TEXT("a zero duration snaps to idle"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(1.0f, false, 0.016f, 0.0f),
		0.0f, Tolerance);
	TestEqual(TEXT("a negative duration is treated as no fade"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.0f, true, 0.016f, -1.0f),
		1.0f, Tolerance);
	TestEqual(TEXT("a paused frame holds the current alpha"),
		BalhwajeomMainMenuHoverFade::AdvanceHoverAlpha(0.25f, true, 0.0f, Duration),
		0.25f, Tolerance);

	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMainMenuHoverFadeOpacityTest,
	"Balhwajeom.Title.MainMenu.HoverFade.Opacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMainMenuHoverFadeOpacityTest::RunTest(const FString& Parameters)
{
	// The shipped values: fully opaque until hovered, then down to half.
	constexpr float Idle = 1.0f;
	constexpr float Hover = 0.5f;
	constexpr float Tolerance = KINDA_SMALL_NUMBER;

	TestEqual(TEXT("an untouched button is fully opaque"),
		BalhwajeomMainMenuHoverFade::ResolveOpacity(0.0f, Idle, Hover), Idle, Tolerance);
	TestEqual(TEXT("a fully hovered button sits at the hover value"),
		BalhwajeomMainMenuHoverFade::ResolveOpacity(1.0f, Idle, Hover), Hover, Tolerance);
	TestEqual(TEXT("the midpoint sits halfway between the two"),
		BalhwajeomMainMenuHoverFade::ResolveOpacity(0.5f, Idle, Hover), 0.75f, Tolerance);

	const float Quarter = BalhwajeomMainMenuHoverFade::ResolveOpacity(0.25f, Idle, Hover);
	TestTrue(TEXT("every intermediate opacity stays between the two values"),
		Quarter < Idle && Quarter > Hover);

	// Reversing the values has to keep working, so the direction can be flipped from
	// the details panel without touching this code.
	TestEqual(TEXT("a brightening fade reads the other way round"),
		BalhwajeomMainMenuHoverFade::ResolveOpacity(1.0f, 0.5f, 1.0f), 1.0f, Tolerance);

	return !HasAnyErrors();
}

#endif
