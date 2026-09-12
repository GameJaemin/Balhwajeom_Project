#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomCameraFocusModel.h"
#include "CameraSystem/BalhwajeomEvidenceTypes.h"
#include "Investigation/EvidenceDefinitions.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomCameraFocusDataDefaultsTest,
	"Balhwajeom.Camera.FocusModel.DataDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomCameraFocusDataDefaultsTest::RunTest(const FString& Parameters)
{
	const FEvidenceStateDefinition State;
	TestEqual(TEXT("State minimum offset defaults to zero"), State.MinimumFocusDistanceOffset, 0.0f);
	TestEqual(TEXT("State maximum offset defaults to zero"), State.MaximumFocusDistanceOffset, 0.0f);

	const FBalhwajeomCameraTargetInfo TargetInfo;
	TestEqual(TEXT("Target snapshot minimum offset defaults to zero"), TargetInfo.MinimumFocusDistanceOffset, 0.0f);
	TestEqual(TEXT("Target snapshot maximum offset defaults to zero"), TargetInfo.MaximumFocusDistanceOffset, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomCameraFocusRangeTest,
	"Balhwajeom.Camera.FocusModel.Range",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomCameraFocusRangeTest::RunTest(const FString& Parameters)
{
	FBalhwajeomFocusRange Range;
	TestTrue(TEXT("Default offsets produce a valid range"),
		FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
			400.0f, 1000.0f, 0.0f, 0.0f, Range));
	TestEqual(TEXT("Default minimum is unchanged"), Range.Minimum, 400.0f);
	TestEqual(TEXT("Default maximum is unchanged"), Range.Maximum, 1000.0f);
	TestTrue(TEXT("Minimum boundary is accepted"), Range.Contains(400.0f));
	TestTrue(TEXT("Maximum boundary is accepted"), Range.Contains(1000.0f));
	TestFalse(TEXT("Distance below minimum is rejected"), Range.Contains(399.0f));
	TestFalse(TEXT("Distance above maximum is rejected"), Range.Contains(1001.0f));

	TestTrue(TEXT("Offsets adjust each bound independently"),
		FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
			400.0f, 1000.0f, -500.0f, 250.0f, Range));
	TestEqual(TEXT("Negative effective minimum clamps to zero"), Range.Minimum, 0.0f);
	TestEqual(TEXT("Positive maximum offset is applied"), Range.Maximum, 1250.0f);

	TestFalse(TEXT("Inverted effective range is invalid"),
		FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
			400.0f, 1000.0f, 800.0f, -200.0f, Range));

	FBalhwajeomFocusRange RangeBeforeZoom;
	FBalhwajeomFocusRange RangeAfterZoom;
	FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
		400.0f, 1000.0f, 25.0f, 75.0f, RangeBeforeZoom);
	// FOV is intentionally absent from the range API: changing zoom cannot alter it.
	FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
		400.0f, 1000.0f, 25.0f, 75.0f, RangeAfterZoom);
	TestEqual(TEXT("Zoom leaves effective minimum unchanged"),
		RangeAfterZoom.Minimum, RangeBeforeZoom.Minimum);
	TestEqual(TEXT("Zoom leaves effective maximum unchanged"),
		RangeAfterZoom.Maximum, RangeBeforeZoom.Maximum);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomCameraFocusBlurCurveTest,
	"Balhwajeom.Camera.FocusModel.BlurCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomCameraFocusBlurCurveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Visual focus depth follows the camera forward axis"),
		FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(
			FVector::ZeroVector,
			FVector::ForwardVector,
			FVector(700.0f, 300.0f, 0.0f)),
		700.0f);
	TestEqual(TEXT("Visual focus depth clamps points behind the camera"),
		FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(
			FVector::ZeroVector,
			FVector::ForwardVector,
			FVector(-100.0f, 0.0f, 0.0f)),
		0.0f);

	const FBalhwajeomFocusRegion Focused =
		FBalhwajeomCameraFocusModel::CalculateFocusedRegion(700.0f, 100.0f);
	TestEqual(TEXT("Focused region stores target distance"), Focused.FocalDistance, 700.0f);
	TestEqual(TEXT("Focused near edge uses blur start distance"), Focused.SharpNear, 600.0f);
	TestEqual(TEXT("Focused far edge uses blur start distance"), Focused.SharpFar, 800.0f);

	const FBalhwajeomFocusRegion Default =
		FBalhwajeomCameraFocusModel::CalculateDefaultRegion(400.0f, 1000.0f);
	TestEqual(TEXT("Default focus uses range midpoint"), Default.FocalDistance, 700.0f);
	TestEqual(TEXT("Default near edge is camera minimum"), Default.SharpNear, 400.0f);
	TestEqual(TEXT("Default far edge is camera maximum"), Default.SharpFar, 1000.0f);

	TestEqual(TEXT("Sharp region has zero blur"),
		FBalhwajeomCameraFocusModel::CalculateBlurStrength(750.0f, Focused, 500.0f, 0.8f),
		0.0f);
	TestEqual(TEXT("Half transition uses quadratic Ease In"),
		FBalhwajeomCameraFocusModel::CalculateBlurStrength(1050.0f, Focused, 500.0f, 0.8f),
		0.2f);
	TestEqual(TEXT("End of transition reaches maximum"),
		FBalhwajeomCameraFocusModel::CalculateBlurStrength(1300.0f, Focused, 500.0f, 0.8f),
		0.8f);
	TestEqual(TEXT("Beyond transition remains clamped"),
		FBalhwajeomCameraFocusModel::CalculateBlurStrength(1600.0f, Focused, 500.0f, 0.8f),
		0.8f);

	const FBalhwajeomFocusRegion Transitioned =
		FBalhwajeomCameraFocusModel::InterpolateRegion(
			Default,
			FBalhwajeomCameraFocusModel::CalculateFocusedRegion(900.0f, 100.0f),
			0.1f,
			1.0f);
	TestEqual(TEXT("Transition interpolates the focus center"), Transitioned.FocalDistance, 720.0f);
	TestEqual(TEXT("Transition interpolates the sharp half width"), Transitioned.SharpNear, 440.0f);
	TestEqual(TEXT("Transition preserves an ordered sharp range"), Transitioned.SharpFar, 1000.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBalhwajeomCameraFocusGraceTest,
	"Balhwajeom.Camera.FocusModel.Grace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBalhwajeomCameraFocusGraceTest::RunTest(const FString& Parameters)
{
	FBalhwajeomFocusGraceState Grace;
	Grace.OnStrictTargetFound();
	TestTrue(TEXT("First 0.05 second miss retains focus"), Grace.ShouldRetainAfterMiss(0.05f, 0.1f));
	Grace.OnStrictTargetFound();
	TestTrue(TEXT("Reacquisition resets elapsed miss time"), Grace.ShouldRetainAfterMiss(0.06f, 0.1f));
	TestFalse(TEXT("Accumulated miss reaching 0.1 seconds expires"),
		Grace.ShouldRetainAfterMiss(0.04f, 0.1f));

	Grace.OnStrictTargetFound();
	TestTrue(TEXT("Single miss below grace retains focus"), Grace.ShouldRetainAfterMiss(0.099f, 0.1f));
	TestFalse(TEXT("Grace expires once total miss exceeds duration"),
		Grace.ShouldRetainAfterMiss(0.002f, 0.1f));
	return true;
}

#endif
