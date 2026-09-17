#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/BalhwajeomNotificationBanner.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNotificationBannerScheduleTest,
	"Balhwajeom.UI.NotificationBanner.Schedule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNotificationBannerScheduleTest::RunTest(const FString& Parameters)
{
	using namespace BalhwajeomNotificationBanner;

	constexpr float Tolerance = KINDA_SMALL_NUMBER;
	constexpr float FadeIn = 0.25f;
	constexpr float Hold = 2.5f;
	constexpr float FadeOut = 0.6f;

	// Nothing is drawn before the schedule starts.
	TestEqual(TEXT("the banner starts invisible"),
		ResolveOpacity(0.0f, FadeIn, Hold, FadeOut), 0.0f, Tolerance);
	TestEqual(TEXT("a negative elapsed stays invisible"),
		ResolveOpacity(-1.0f, FadeIn, Hold, FadeOut), 0.0f, Tolerance);

	TestEqual(TEXT("the fade in is linear to its midpoint"),
		ResolveOpacity(FadeIn * 0.5f, FadeIn, Hold, FadeOut), 0.5f, Tolerance);

	// The whole hold is fully opaque -- this is the part the player actually reads.
	TestEqual(TEXT("the banner is opaque as the fade in ends"),
		ResolveOpacity(FadeIn, FadeIn, Hold, FadeOut), 1.0f, Tolerance);
	TestEqual(TEXT("the banner is opaque through the hold"),
		ResolveOpacity(FadeIn + Hold * 0.5f, FadeIn, Hold, FadeOut), 1.0f, Tolerance);
	TestEqual(TEXT("the banner is still opaque at the end of the hold"),
		ResolveOpacity(FadeIn + Hold - 0.01f, FadeIn, Hold, FadeOut), 1.0f, Tolerance);

	TestEqual(TEXT("the fade out is linear to its midpoint"),
		ResolveOpacity(FadeIn + Hold + FadeOut * 0.5f, FadeIn, Hold, FadeOut), 0.5f, Tolerance);
	TestEqual(TEXT("the banner is gone at the end of the schedule"),
		ResolveOpacity(FadeIn + Hold + FadeOut, FadeIn, Hold, FadeOut), 0.0f, Tolerance);
	TestEqual(TEXT("the banner stays gone past the schedule"),
		ResolveOpacity(60.0f, FadeIn, Hold, FadeOut), 0.0f, Tolerance);

	// Zero-length fades are valid settings meaning "appear/vanish at once", not a divide by zero.
	TestEqual(TEXT("a zero fade in is opaque immediately"),
		ResolveOpacity(0.01f, 0.0f, Hold, FadeOut), 1.0f, Tolerance);
	TestEqual(TEXT("a zero fade out vanishes immediately"),
		ResolveOpacity(Hold + 0.01f, 0.0f, Hold, 0.0f), 0.0f, Tolerance);

	// Monotone up then down, with no spike between the segments.
	{
		float Previous = 0.0f;
		bool bSawPeak = false;
		for (int32 Step = 0; Step <= 240; ++Step)
		{
			const float Elapsed = Step * (1.0f / 60.0f);
			const float Opacity = ResolveOpacity(Elapsed, FadeIn, Hold, FadeOut);
			TestTrue(TEXT("opacity stays within range"), Opacity >= 0.0f && Opacity <= 1.0f);
			if (Opacity >= 1.0f - Tolerance)
			{
				bSawPeak = true;
			}
			else if (bSawPeak)
			{
				TestTrue(TEXT("opacity only falls once it has peaked"), Opacity <= Previous + Tolerance);
			}
			Previous = Opacity;
		}
		TestTrue(TEXT("the banner reaches full opacity at some point"), bSawPeak);
	}

	// IsFinished is what takes the banner out of the tick, so it has to agree with the curve.
	TestFalse(TEXT("the banner is not finished while fading in"),
		IsFinished(FadeIn * 0.5f, FadeIn, Hold, FadeOut));
	TestFalse(TEXT("the banner is not finished during the hold"),
		IsFinished(FadeIn + Hold * 0.5f, FadeIn, Hold, FadeOut));
	TestFalse(TEXT("the banner is not finished while fading out"),
		IsFinished(FadeIn + Hold + FadeOut * 0.5f, FadeIn, Hold, FadeOut));
	TestTrue(TEXT("the banner is finished at the end of the schedule"),
		IsFinished(FadeIn + Hold + FadeOut, FadeIn, Hold, FadeOut));
	TestTrue(TEXT("an all-zero schedule is finished at once"),
		IsFinished(0.0f, 0.0f, 0.0f, 0.0f));

	return !HasAnyErrors();
}

#endif
