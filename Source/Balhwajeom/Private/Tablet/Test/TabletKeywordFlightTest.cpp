#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Tablet/BalhwajeomTabletKeywordFlight.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTabletKeywordFlightCurveTest,
	"Balhwajeom.Tablet.KeywordFlight.Curve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTabletKeywordFlightCurveTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomTabletKeywordFlight;

	constexpr float Tolerance = KINDA_SMALL_NUMBER;
	constexpr float Duration = 0.25f;
	constexpr float Exponent = 3.0f;

	// Landing exactly is what lets the caller commit on the frame the chip arrives.
	TestEqual(TEXT("the flight starts at zero progress"),
		ResolveEaseOutAlpha(0.0f, Duration, Exponent), 0.0f, Tolerance);
	TestEqual(TEXT("the flight ends at full progress"),
		ResolveEaseOutAlpha(Duration, Duration, Exponent), 1.0f, Tolerance);
	TestEqual(TEXT("an overrun flight stays at full progress"),
		ResolveEaseOutAlpha(Duration * 4.0f, Duration, Exponent), 1.0f, Tolerance);

	// A zero duration is a valid setting -- it means land on the first tick, not divide by zero.
	TestEqual(TEXT("a zero duration lands immediately"),
		ResolveEaseOutAlpha(0.0f, 0.0f, Exponent), 1.0f, Tolerance);
	TestEqual(TEXT("a negative duration lands immediately"),
		ResolveEaseOutAlpha(0.01f, -1.0f, Exponent), 1.0f, Tolerance);

	// Ease-out: the chip shoots out and decelerates, so the first half of the time has to
	// cover well over half the distance. At exponent 3 the halfway point is 7/8.
	{
		const float Halfway = ResolveEaseOutAlpha(Duration * 0.5f, Duration, Exponent);
		TestTrue(TEXT("half the time covers most of the distance"), Halfway > 0.5f);
		TestEqual(TEXT("cubic ease-out is 7/8 at the midpoint"), Halfway, 0.875f, 0.001f);
	}

	// Exponent 1 is the documented "no easing" setting.
	TestEqual(TEXT("exponent one is constant speed"),
		ResolveEaseOutAlpha(Duration * 0.5f, Duration, 1.0f), 0.5f, Tolerance);

	// Raising the exponent is the knob for "stronger ease": more of the trip happens in the
	// first half, so the chip leaves harder and drifts into the blank.
	{
		const float Weaker = ResolveEaseOutAlpha(Duration * 0.25f, Duration, 3.0f);
		const float Stronger = ResolveEaseOutAlpha(Duration * 0.25f, Duration, 5.0f);
		TestTrue(TEXT("a higher exponent front-loads more of the flight"), Stronger > Weaker);
		TestTrue(TEXT("a stronger ease still starts and ends in the same place"),
			ResolveEaseOutAlpha(0.0f, Duration, 5.0f) == 0.0f &&
			ResolveEaseOutAlpha(Duration, Duration, 5.0f) == 1.0f);
	}

	// Monotone, and never past the target.
	{
		float PreviousAlpha = 0.0f;
		for (int32 Step = 1; Step <= 15; ++Step)
		{
			const float Alpha = ResolveEaseOutAlpha(Step * (1.0f / 60.0f), Duration, Exponent);
			TestTrue(TEXT("progress never goes backwards"), Alpha >= PreviousAlpha - Tolerance);
			TestTrue(TEXT("progress never exceeds one"), Alpha <= 1.0f + Tolerance);
			PreviousAlpha = Alpha;
		}
		TestEqual(TEXT("the flight has arrived by the end of its duration"),
			PreviousAlpha, 1.0f, Tolerance);
	}

	// Position and scale are pinned to the endpoints.
	{
		const FVector2D Start(100.0f, 200.0f);
		const FVector2D End(640.0f, 480.0f);
		TestEqual(TEXT("zero progress sits on the clicked chip"),
			ResolveFlightPosition(Start, End, 0.0f), Start);
		TestEqual(TEXT("full progress sits on the blank"),
			ResolveFlightPosition(Start, End, 1.0f), End);
		TestEqual(TEXT("out-of-range progress is clamped to the blank"),
			ResolveFlightPosition(Start, End, 3.0f), End);
	}

	TestEqual(TEXT("the chip leaves at the start scale"),
		ResolveFlightScale(1.15f, 0.0f), 1.15f, Tolerance);
	TestEqual(TEXT("the chip lands at its normal size"),
		ResolveFlightScale(1.15f, 1.0f), 1.0f, Tolerance);
	TestEqual(TEXT("a start scale of one disables the pop"),
		ResolveFlightScale(1.0f, 0.4f), 1.0f, Tolerance);

	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTabletKeywordFlightSlotTest,
	"Balhwajeom.Tablet.KeywordFlight.SlotChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTabletKeywordFlightSlotTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomTabletKeywordFlight;

	const TArray<int32> Slots = {0, 1, 2};

	TestEqual(TEXT("an empty sentence takes its first blank"),
		ResolveFirstOpenSlot(Slots, {}, {}), 0);

	TestEqual(TEXT("a submitted blank is skipped"),
		ResolveFirstOpenSlot(Slots, {0}, {}), 1);

	// The reason this function exists. A chip still in the air has only reserved its blank,
	// so the submission list does not know about it yet -- without this, a second click
	// would pick the same blank and one of the two keywords would be lost.
	TestEqual(TEXT("a blank already being flown at is skipped"),
		ResolveFirstOpenSlot(Slots, {}, {0}), 1);
	TestEqual(TEXT("submitted and in-flight blanks are both skipped"),
		ResolveFirstOpenSlot(Slots, {0}, {1}), 2);

	// Three quick clicks on a three-blank sentence fill three different blanks, and the
	// fourth has nowhere to go rather than overwriting one.
	TestEqual(TEXT("every blank claimed leaves nothing open"),
		ResolveFirstOpenSlot(Slots, {}, {0, 1, 2}), INDEX_NONE);
	TestEqual(TEXT("a fully submitted sentence leaves nothing open"),
		ResolveFirstOpenSlot(Slots, {0, 1, 2}, {}), INDEX_NONE);

	// TMap::GetKeys gives no ordering guarantee, so the lowest open index is chosen rather
	// than the first one encountered.
	TestEqual(TEXT("unsorted slot indices still pick the lowest open one"),
		ResolveFirstOpenSlot({2, 0, 1}, {0}, {}), 1);
	TestEqual(TEXT("unsorted slot indices with no claims pick the lowest"),
		ResolveFirstOpenSlot({5, 2, 9}, {}, {}), 2);

	TestEqual(TEXT("a sentence with no blanks leaves nothing open"),
		ResolveFirstOpenSlot({}, {}, {}), INDEX_NONE);

	// Stale entries for blanks this sentence does not have must not be able to block it.
	TestEqual(TEXT("claims outside the sentence are ignored"),
		ResolveFirstOpenSlot(Slots, {7}, {8}), 0);

	return !HasAnyErrors();
}

#endif
