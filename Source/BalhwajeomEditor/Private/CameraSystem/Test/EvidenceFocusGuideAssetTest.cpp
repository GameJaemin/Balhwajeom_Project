#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CameraSystem/BalhwajeomEvidenceCameraHUD.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Engine/DataTable.h"
#include "Investigation/EvidenceDefinitions.h"
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
	const UClass* MultiShadowTextClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Script/MultiShadowText.MultiShadowTextWidget"));
	const UWidget* LabelText =
		FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("LabelText"));
	const UWidget* SubLabelText =
		FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("LabelText_sub"));
	TestNotNull(TEXT("Multi Shadow Text widget class should load"), MultiShadowTextClass);
	TestTrue(
		TEXT("WBP_EvidenceFocusGuide LabelText should use Multi Shadow Text"),
		LabelText && MultiShadowTextClass && LabelText->IsA(MultiShadowTextClass));
	TestTrue(
		TEXT("WBP_EvidenceFocusGuide LabelText_sub should use Multi Shadow Text"),
		SubLabelText && MultiShadowTextClass && SubLabelText->IsA(MultiShadowTextClass));
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
	FEvidenceCaptureBlockedLabelsDataTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.CaptureBlockedLabelsData",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceCaptureBlockedLabelsDataTest::RunTest(const FString& Parameters)
{
	const UDataTable* EvidenceStates = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates.DT_EvidenceStates"));
	if (!TestNotNull(TEXT("DT_EvidenceStates should load"), EvidenceStates))
	{
		return false;
	}

	struct FExpectedLabel
	{
		const TCHAR* StateID;
		const TCHAR* Label;
	};
	static const FExpectedLabel ExpectedLabels[] =
	{
		{ TEXT("STATE_01_001_DUST"), TEXT("사진을 찍기 전에 먼지부터 털어 보자.") },
		{ TEXT("STATE_01_002_DUST"), TEXT("사진을 찍기 전에 먼지부터 털어 보자.") },
		{ TEXT("STATE_01_003_DUST"), TEXT("사진을 찍기 전에 먼지부터 털어 보자.") },
		{ TEXT("STATE_01_004_CLOSED"), TEXT("일기장을 먼저 펼쳐 보자.") },
		{ TEXT("STATE_01_005_FRONT"), TEXT("고데기를 뒤집어 반대편을 확인해 보자.") }
	};

	for (const FExpectedLabel& Expected : ExpectedLabels)
	{
		const FEvidenceStateDefinition* State =
			EvidenceStates->FindRow<FEvidenceStateDefinition>(
				FName(Expected.StateID),
				TEXT("CaptureBlockedLabelsData"));
		if (TestNotNull(Expected.StateID, State))
		{
			TestEqual(
				*FString::Printf(TEXT("%s should use its authored capture guidance"), Expected.StateID),
				State->CaptureBlockedLabel.ToString(),
				FString(Expected.Label));
		}
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuideMatchesObjectLabelTextStyleTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.MatchesObjectLabelTextStyle",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuideMatchesObjectLabelTextStyleTest::RunTest(
	const FString& Parameters)
{
	const UWidgetBlueprint* FocusGuideBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_EvidenceFocusGuide.WBP_EvidenceFocusGuide"));
	const UWidgetBlueprint* ObjectLabelBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel"));
	const UClass* MultiShadowTextClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Script/MultiShadowText.MultiShadowTextWidget"));
	if (!TestNotNull(TEXT("Focus-guide WBP should load"), FocusGuideBlueprint) ||
		!TestNotNull(TEXT("Object-label WBP should load"), ObjectLabelBlueprint) ||
		!TestNotNull(TEXT("Multi Shadow Text widget class should load"), MultiShadowTextClass) ||
		!FocusGuideBlueprint->WidgetTree ||
		!ObjectLabelBlueprint->WidgetTree)
	{
		return false;
	}

	const UWidget* CameraLabel =
		FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("LabelText"));
	const UWidget* ObjectLabel =
		ObjectLabelBlueprint->WidgetTree->FindWidget(TEXT("LabelText"));
	if (!TestTrue(
			TEXT("Camera LabelText should use Multi Shadow Text"),
			CameraLabel && CameraLabel->IsA(MultiShadowTextClass)) ||
		!TestTrue(
			TEXT("Object LabelText should use Multi Shadow Text"),
			ObjectLabel && ObjectLabel->IsA(MultiShadowTextClass)))
	{
		return false;
	}

	static const FName StylePropertyNames[] = {
		TEXT("Font"),
		TEXT("TextColor"),
		TEXT("Justification"),
		TEXT("bAutoWrapText"),
		TEXT("WrapTextAt"),
		TEXT("ShadowLayers")
	};
	for (const FName PropertyName : StylePropertyNames)
	{
		const FProperty* Property = MultiShadowTextClass->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s style property should exist"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("Camera %s should match the object label"), *PropertyName.ToString()),
				Property->Identical_InContainer(CameraLabel, ObjectLabel));
		}
	}

	const FArrayProperty* ShadowLayersProperty = FindFProperty<FArrayProperty>(
		MultiShadowTextClass,
		TEXT("ShadowLayers"));
	if (TestNotNull(TEXT("Multi Shadow Text should expose ShadowLayers"), ShadowLayersProperty))
	{
		FScriptArrayHelper CameraShadowLayers(
			ShadowLayersProperty,
			ShadowLayersProperty->ContainerPtrToValuePtr<void>(CameraLabel));
		TestEqual(
			TEXT("Camera label should use the same two shadow layers as the object label"),
			CameraShadowLayers.Num(),
			2);
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceFocusGuideMultiShadowRuntimeTextTest,
	"Balhwajeom.Camera.Evidence.FocusGuide.MultiShadowRuntimeText",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FEvidenceFocusGuideMultiShadowRuntimeTextTest::RunTest(
	const FString& Parameters)
{
	const UWidgetBlueprint* FocusGuideBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Camera/WBP_EvidenceFocusGuide.WBP_EvidenceFocusGuide"));
	const UClass* MultiShadowTextClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Script/MultiShadowText.MultiShadowTextWidget"));
	if (!TestNotNull(TEXT("Focus-guide WBP should load"), FocusGuideBlueprint) ||
		!FocusGuideBlueprint->GeneratedClass ||
		!TestNotNull(TEXT("Multi Shadow Text widget class should load"), MultiShadowTextClass))
	{
		return false;
	}

	UUserWidget* FocusGuide = NewObject<UUserWidget>(
		GetTransientPackage(),
		FocusGuideBlueprint->GeneratedClass);
	if (!TestNotNull(TEXT("Focus-guide widget should instantiate"), FocusGuide) ||
		!TestTrue(TEXT("Focus-guide widget should initialize"), FocusGuide->Initialize()))
	{
		return false;
	}

	UWidget* LabelText = FocusGuide->GetWidgetFromName(TEXT("LabelText"));
	if (!TestTrue(
			TEXT("Runtime LabelText should use Multi Shadow Text"),
			LabelText && LabelText->IsA(MultiShadowTextClass)))
	{
		return false;
	}

	UFunction* SetLabelTextFunction = FocusGuide->FindFunction(TEXT("SetLabelText"));
	if (!TestNotNull(TEXT("Focus guide should expose SetLabelText"), SetLabelTextFunction))
	{
		return false;
	}

	struct FSetLabelTextParameters
	{
		FText Label;
	};
	const FText ExpectedText = FText::FromString(TEXT("카메라 포커스 라벨"));
	FSetLabelTextParameters SetLabelTextParameters{ ExpectedText };
	FocusGuide->ProcessEvent(SetLabelTextFunction, &SetLabelTextParameters);

	const FTextProperty* TextProperty = FindFProperty<FTextProperty>(
		MultiShadowTextClass,
		TEXT("Text"));
	const FText* ActualText = TextProperty
		? TextProperty->ContainerPtrToValuePtr<FText>(LabelText)
		: nullptr;
	TestEqual(
		TEXT("Runtime SetLabelText should update the camera Multi Shadow Text content"),
		ActualText ? ActualText->ToString() : FString(),
		ExpectedText.ToString());
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
	const UWidget* LabelText =
		FocusGuideBlueprint->WidgetTree->FindWidget(TEXT("LabelText"));
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
	TestFalse(
		TEXT("UseCamera should keep its explicit Canvas size"),
		StatusSlot->GetAutoSize());
	TestTrue(
		TEXT("UseCamera should be centered on its Canvas position"),
		StatusSlot->GetAlignment().Equals(FVector2D(0.5f, 0.5f)));
	TestTrue(
		TEXT("Label container should start at its Canvas position and remain vertically centered"),
		LabelContainerSlot->GetAlignment().Equals(FVector2D(0.0f, 0.5f)));
	TestTrue(
		TEXT("UseCamera should preserve the 67x50 camera icon aspect ratio"),
		StatusSlot->GetSize().Equals(FVector2D(67.0f, 50.0f)));
	TestTrue(
		TEXT("UseCamera center should remain aligned to the guide point"),
		StatusSlot->GetPosition().Equals(FVector2D(33.5f, 25.0f)));
	TestTrue(
		TEXT("Label should remain ten pixels beyond the icon's right edge"),
		LabelContainerSlot->GetPosition().Equals(FVector2D(77.0f, 25.0f)));

	return true;
}

#endif
