#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Interaction/InspectionLabelPresentation.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInspectionLabelPresentationQualityTest,
	"Balhwajeom.Interaction.InspectionLabel.PreservesScreenSpaceQuality",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

bool FInspectionLabelPresentationQualityTest::RunTest(const FString& Parameters)
{
	const float Distances[] = { 0.0f, 250.0f, 1000.0f, 5000.0f };
	for (const float Distance : Distances)
	{
		float Scale = 0.0f;
		float Opacity = 0.0f;
		BalhwajeomInspectionLabelPresentation::Calculate(
			Distance,
			1000.0f,
			Scale,
			Opacity);

		TestEqual(
			FString::Printf(TEXT("Distance %.0f keeps a 1:1 render scale"), Distance),
			Scale,
			1.0f);
		TestEqual(
			FString::Printf(TEXT("Distance %.0f keeps full glyph contrast"), Distance),
			Opacity,
			1.0f);
	}

	return true;
}

#endif
