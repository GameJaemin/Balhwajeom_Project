#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomEvidenceFocusGuideLayout.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceStatusIconUsesTwoStatesTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.RuntimeUsesTwoIconStates",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceStatusIconUsesTwoStatesTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Capture-enabled uncaptured evidence should use the camera icon"),
		BalhwajeomEvidenceFocusGuideLayout::ShouldUsePhotoRequiredIcon(true, false));
	TestFalse(
		TEXT("Capture-disabled evidence should use the default dot icon"),
		BalhwajeomEvidenceFocusGuideLayout::ShouldUsePhotoRequiredIcon(false, false));
	TestFalse(
		TEXT("Captured evidence should use the default dot icon"),
		BalhwajeomEvidenceFocusGuideLayout::ShouldUsePhotoRequiredIcon(true, true));
	TestFalse(
		TEXT("Captured and disabled evidence should use the default dot icon"),
		BalhwajeomEvidenceFocusGuideLayout::ShouldUsePhotoRequiredIcon(false, true));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuideUsesCloserViewFeedbackTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.UsesCloserViewFeedback",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuideUsesCloserViewFeedbackTest::RunTest(const FString& Parameters)
{
	const FText AuthoredLabel = FText::FromString(TEXT("낡은 스노우글로브다."));
	TestEqual(
		TEXT("A target that is too small replaces the authored label with capture guidance"),
		BalhwajeomEvidenceFocusGuideLayout::ResolveLabelText(AuthoredLabel, true).ToString(),
		FString(TEXT("조금 더 가까이 가거나 확대해 보자.")));
	TestEqual(
		TEXT("A large-enough target keeps its authored label"),
		BalhwajeomEvidenceFocusGuideLayout::ResolveLabelText(AuthoredLabel, false).ToString(),
		AuthoredLabel.ToString());
	return true;
}


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
	UTexture2D* DefaultIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/DotIcon.DotIcon"));
	if (!TestNotNull(TEXT("Required evidence icon should load"), RequiredIcon))
	{
		return false;
	}
	if (!TestNotNull(TEXT("Default evidence icon should load"), DefaultIcon))
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

	BalhwajeomEvidenceFocusGuideLayout::ApplyStatusTexture(StatusImage, DefaultIcon);
	TestEqual(
		TEXT("Runtime texture swap should apply the default icon"),
		StatusImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(DefaultIcon));
	TestTrue(
		TEXT("Default-icon swap should preserve the authored 67x50 brush size"),
		StatusImage->GetBrush().ImageSize.Equals(FVector2D(67.0, 50.0)));
	return true;
}

#endif
