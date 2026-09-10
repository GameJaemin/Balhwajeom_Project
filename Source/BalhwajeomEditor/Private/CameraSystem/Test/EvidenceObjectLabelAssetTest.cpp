#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "WidgetBlueprint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceObjectLabelAssetTest,
	"Balhwajeom.Camera.Evidence.ObjectLabel.Assets",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceObjectLabelAssetTest::RunTest(const FString& Parameters)
{
	const UTexture2D* RequiredIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoRequired.T_EvidencePhotoRequired"));
	const UTexture2D* CapturedIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoCaptured.T_EvidencePhotoCaptured"));

	TestNotNull(TEXT("Photo-required icon should be imported"), RequiredIcon);
	TestNotNull(TEXT("Photo-captured icon should be imported"), CapturedIcon);

	const UWidgetBlueprint* ObjectLabelBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel"));
	TestNotNull(TEXT("WBP_ObjectLabel should load"), ObjectLabelBlueprint);
	if (!ObjectLabelBlueprint || !RequiredIcon)
	{
		return false;
	}

	const UImage* StatusImage = Cast<UImage>(
		ObjectLabelBlueprint->WidgetTree->FindWidget(TEXT("UseCamera")));
	TestNotNull(TEXT("WBP_ObjectLabel should contain the UseCamera image"), StatusImage);
	if (!StatusImage)
	{
		return false;
	}

	TestEqual(
		TEXT("UseCamera should default to the photo-required icon"),
		StatusImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(const_cast<UTexture2D*>(RequiredIcon)));

	return true;
}

#endif
