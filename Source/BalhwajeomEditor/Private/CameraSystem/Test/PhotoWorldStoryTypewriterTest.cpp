#if WITH_DEV_AUTOMATION_TESTS

#include "CameraSystem/PhotoWorldStoryActor.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhotoWorldStoryTypewriterTimingTest,
	"Balhwajeom.Camera.PhotoWorldStory.TypewriterTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhotoWorldStoryTypewriterTimingTest::RunTest(const FString& Parameters)
{
	const UBalhwajeomInvestigationSettings* Settings =
		GetDefault<UBalhwajeomInvestigationSettings>();
	TestNotNull(TEXT("Investigation settings provide global story timing"), Settings);
	if (Settings)
	{
		TestEqual(TEXT("Every story defaults to 20 characters per second"),
			Settings->WorldStoryCharactersPerSecond, 20.0f);
	}
	TestTrue(TEXT("Ten characters at five per second finish in 1.8 seconds"),
		FMath::IsNearlyEqual(
			APhotoWorldStoryActor::CalculateTypingDuration(10, 5.0f), 1.8f));

	return true;
}

#endif
