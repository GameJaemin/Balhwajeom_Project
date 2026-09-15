#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomEvidenceFocusGuideLayout.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuideCentersOnGuidePositionTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.RuntimeCentersOnGuidePosition",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuideCentersOnGuidePositionTest::RunTest(const FString& Parameters)
{
	const FVector2D GuidePosition(400.0, 300.0);
	TestTrue(
		TEXT("A 67x50 focus guide should place its center exactly on GuidePosition"),
		BalhwajeomEvidenceFocusGuideLayout::CalculateWidgetPosition(GuidePosition)
			.Equals(FVector2D(366.5, 275.0)));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuidePreservesAuthoredIconSizeTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.RuntimePreservesAuthoredIconSize",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuidePreservesAuthoredIconSizeTest::RunTest(const FString& Parameters)
{
	UTexture2D* RequiredIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoRequired.T_EvidencePhotoRequired"));
	UTexture2D* CapturedIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoCaptured.T_EvidencePhotoCaptured"));
	if (!TestNotNull(TEXT("Required evidence icon should load"), RequiredIcon))
	{
		return false;
	}
	if (!TestNotNull(TEXT("Captured evidence icon should load"), CapturedIcon))
	{
		return false;
	}

	UImage* StatusImage = NewObject<UImage>(GetTransientPackage());
	FSlateBrush AuthoredBrush = StatusImage->GetBrush();
	AuthoredBrush.ImageSize = FVector2D(67.0, 50.0);
	StatusImage->SetBrush(AuthoredBrush);
	BalhwajeomEvidenceFocusGuideLayout::ApplyStatusTexture(StatusImage, RequiredIcon);

	TestEqual(
		TEXT("Runtime texture swap should apply the requested icon"),
		StatusImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(RequiredIcon));
	TestTrue(
		TEXT("Required-icon swap should preserve the authored 67x50 brush size"),
		StatusImage->GetBrush().ImageSize.Equals(FVector2D(67.0, 50.0)));

	BalhwajeomEvidenceFocusGuideLayout::ApplyStatusTexture(StatusImage, CapturedIcon);
	TestEqual(
		TEXT("Runtime texture swap should apply the captured icon"),
		StatusImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(CapturedIcon));
	TestTrue(
		TEXT("Captured-icon swap should preserve the authored 67x50 brush size"),
		StatusImage->GetBrush().ImageSize.Equals(FVector2D(67.0, 50.0)));
	return true;
}

#endif
