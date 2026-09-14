#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvidenceObjectLabelAssetTest,
	"Balhwajeom.Camera.Evidence.ObjectLabel.Assets",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMultiShadowTextRuntimeSetTextTest,
	"Balhwajeom.UI.MultiShadowText.RuntimeSetText",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FMultiShadowTextRuntimeSetTextTest::RunTest(const FString& Parameters)
{
	UClass* MultiShadowTextClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Script/MultiShadowText.MultiShadowTextWidget"));
	TestNotNull(TEXT("Multi Shadow Text widget class should load"), MultiShadowTextClass);
	if (!MultiShadowTextClass)
	{
		return false;
	}

	UObject* MultiShadowText = NewObject<UObject>(
		GetTransientPackage(),
		MultiShadowTextClass);
	TestNotNull(TEXT("Multi Shadow Text widget should be constructible"), MultiShadowText);
	if (!MultiShadowText)
	{
		return false;
	}

	UFunction* SetTextFunction = MultiShadowText->FindFunction(TEXT("SetText"));
	TestNotNull(
		TEXT("Multi Shadow Text should expose SetText(FText) to Blueprint"),
		SetTextFunction);
	if (!SetTextFunction)
	{
		return false;
	}

	struct FSetTextParameters
	{
		FText InText;
	};

	const FText ExpectedText = FText::FromString(TEXT("동적 조사 문구"));
	FSetTextParameters SetTextParameters{ ExpectedText };
	MultiShadowText->ProcessEvent(SetTextFunction, &SetTextParameters);

	const FTextProperty* TextProperty = FindFProperty<FTextProperty>(
		MultiShadowTextClass,
		TEXT("Text"));
	TestNotNull(TEXT("Multi Shadow Text should retain its Text property"), TextProperty);
	if (!TextProperty)
	{
		return false;
	}

	const FText* ActualText = TextProperty->ContainerPtrToValuePtr<FText>(
		MultiShadowText);
	TestNotNull(TEXT("Multi Shadow Text should store the assigned text"), ActualText);
	if (!ActualText)
	{
		return false;
	}

	TestEqual(
		TEXT("SetText should update the text used by the widget"),
		ActualText->ToString(),
		ExpectedText.ToString());
	return true;
}


bool FEvidenceObjectLabelAssetTest::RunTest(const FString& Parameters)
{
	const UTexture2D* DefaultIcon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Icons/DotIcon.DotIcon"));

	TestNotNull(TEXT("Non-photo default icon should be available"), DefaultIcon);

	const UWidgetBlueprint* ObjectLabelBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel"));
	TestNotNull(TEXT("WBP_ObjectLabel should load"), ObjectLabelBlueprint);
	if (!ObjectLabelBlueprint || !DefaultIcon)
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
		TEXT("UseCamera should preserve the non-photo default icon"),
		StatusImage->GetBrush().GetResourceObject(),
		static_cast<UObject*>(const_cast<UTexture2D*>(DefaultIcon)));

	const UClass* MultiShadowTextClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Script/MultiShadowText.MultiShadowTextWidget"));
	TestNotNull(
		TEXT("Multi Shadow Text widget class should load"),
		MultiShadowTextClass);

	const UWidget* LabelText =
		ObjectLabelBlueprint->WidgetTree->FindWidget(TEXT("LabelText"));
	TestNotNull(TEXT("WBP_ObjectLabel should contain LabelText"), LabelText);
	TestTrue(
		TEXT("LabelText should be a Multi Shadow Text widget"),
		LabelText && MultiShadowTextClass && LabelText->IsA(MultiShadowTextClass));

	TestNotNull(
		TEXT("WBP_ObjectLabel should preserve SetLabelText(FText)"),
		ObjectLabelBlueprint->GeneratedClass->FindFunctionByName(
			TEXT("SetLabelText")));

	UUserWidget* ObjectLabelInstance = NewObject<UUserWidget>(
		GetTransientPackage(),
		ObjectLabelBlueprint->GeneratedClass);
	TestNotNull(TEXT("WBP_ObjectLabel should instantiate"), ObjectLabelInstance);
	if (ObjectLabelInstance)
	{
		TestTrue(
			TEXT("WBP_ObjectLabel should initialize its widget tree"),
			ObjectLabelInstance->Initialize());

		struct FSetLabelTextParameters
		{
			FText NewText;
		};

		const FText ExpectedLabel = FText::FromString(TEXT("거리별 동적 라벨"));
		FSetLabelTextParameters SetLabelTextParameters{ ExpectedLabel };
		if (UFunction* SetLabelTextFunction =
			ObjectLabelInstance->FindFunction(TEXT("SetLabelText")))
		{
			ObjectLabelInstance->ProcessEvent(
				SetLabelTextFunction,
				&SetLabelTextParameters);
		}

		UWidget* RuntimeLabel =
			ObjectLabelInstance->GetWidgetFromName(TEXT("LabelText"));
		const FTextProperty* RuntimeTextProperty = RuntimeLabel
			? FindFProperty<FTextProperty>(RuntimeLabel->GetClass(), TEXT("Text"))
			: nullptr;
		const FText* RuntimeText = RuntimeTextProperty
			? RuntimeTextProperty->ContainerPtrToValuePtr<FText>(RuntimeLabel)
			: nullptr;
		TestEqual(
			TEXT("SetLabelText should update the Multi Shadow Text content"),
			RuntimeText ? RuntimeText->ToString() : FString(),
			ExpectedLabel.ToString());
	}

	return true;
}

#endif
