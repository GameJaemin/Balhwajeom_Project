#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CameraSystem/BalhwajeomEvidenceCameraHUD.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuideAssetTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.Assets",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuideAssetTest::RunTest(const FString& Parameters)
{
	const UWidgetBlueprint* FocusGuideBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_EvidenceFocusGuide.WBP_EvidenceFocusGuide"));
	TestNotNull(TEXT("WBP_EvidenceFocusGuide should load"), FocusGuideBlueprint);
	if (!FocusGuideBlueprint || !FocusGuideBlueprint->WidgetTree || !FocusGuideBlueprint->GeneratedClass)
	{
		return false;
	}

	TestNotNull(
		TEXT("WBP_EvidenceFocusGuide should contain the UseCamera image"),
		Cast<UImage>(FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("UseCamera"))));
	TestNotNull(
		TEXT("WBP_EvidenceFocusGuide should contain the LabelText text block"),
		Cast<UTextBlock>(FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("LabelText"))));
	TestNotNull(
		TEXT("WBP_EvidenceFocusGuide should provide SetLabelText"),
		FocusGuideBlueprint->GeneratedClass->FindFunctionByName(TEXT("SetLabelText")));

	const FClassProperty* FocusGuideClassProperty = FindFProperty<FClassProperty>(
		ABalhwajeomEvidenceCameraHUD::StaticClass(),
		TEXT("FocusGuideWidgetClass"));
	TestNotNull(
		TEXT("Evidence camera HUD should configure a focus-guide widget class"),
		FocusGuideClassProperty);
	if (!FocusGuideClassProperty)
	{
		return false;
	}

	const UObject* ConfiguredClass = FocusGuideClassProperty->GetObjectPropertyValue_InContainer(
		GetDefault<ABalhwajeomEvidenceCameraHUD>());
	TestEqual(
		TEXT("Evidence camera HUD should use WBP_EvidenceFocusGuide"),
		ConfiguredClass,
		static_cast<const UObject*>(FocusGuideBlueprint->GeneratedClass));

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuideStableAnchorTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.StableIconAnchor",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuideStableAnchorTest::RunTest(const FString& Parameters)
{
	const UWidgetBlueprint* FocusGuideBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_EvidenceFocusGuide.WBP_EvidenceFocusGuide"));
	if (!TestNotNull(TEXT("WBP_EvidenceFocusGuide should load"), FocusGuideBlueprint) ||
		!FocusGuideBlueprint->WidgetTree)
	{
		return false;
	}

	const UImage* StatusImage = Cast<UImage>(
		FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("UseCamera")));
	const UTextBlock* LabelText = Cast<UTextBlock>(
		FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("LabelText")));
	const UPanelWidget* LabelContainer = LabelText ? LabelText->GetParent() : nullptr;
	const UCanvasPanelSlot* StatusSlot = StatusImage
		? Cast<UCanvasPanelSlot>(StatusImage->Slot)
		: nullptr;
	const UCanvasPanelSlot* LabelContainerSlot = LabelContainer
		? Cast<UCanvasPanelSlot>(LabelContainer->Slot)
		: nullptr;
	if (!TestNotNull(TEXT("UseCamera should occupy a Canvas slot"), StatusSlot) ||
		!TestNotNull(TEXT("Label container should occupy a Canvas slot"), LabelContainerSlot))
	{
		return false;
	}

	const FAnchors StatusAnchors = StatusSlot->GetAnchors();
	const FAnchors LabelAnchors = LabelContainerSlot->GetAnchors();
	TestTrue(
		TEXT("UseCamera should use a stable top-left anchor"),
		StatusAnchors.Minimum.Equals(FVector2D::ZeroVector) &&
			StatusAnchors.Maximum.Equals(FVector2D::ZeroVector));
	TestTrue(
		TEXT("Label container should use the same stable top-left anchor"),
		LabelAnchors.Minimum.Equals(FVector2D::ZeroVector) &&
			LabelAnchors.Maximum.Equals(FVector2D::ZeroVector));
	TestTrue(
		TEXT("UseCamera center should remain at local 25,25"),
		StatusSlot->GetPosition().Equals(FVector2D(25.0f, 25.0f)));
	TestTrue(
		TEXT("Label should remain ten pixels beyond the icon's right edge"),
		LabelContainerSlot->GetPosition().Equals(FVector2D(60.0f, 25.0f)));

	return true;
}

#endif
