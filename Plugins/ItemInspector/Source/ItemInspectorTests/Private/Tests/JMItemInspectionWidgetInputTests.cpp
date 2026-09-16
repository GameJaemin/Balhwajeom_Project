#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "ItemInspection/JMItemInspectionWidgetBase.h"
#include "Layout/Geometry.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJMItemInspectionFullScreenPointerInputTest,
	"JM.ItemInspector.WidgetInput.FullScreenPointerArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMItemInspectionFullScreenPointerInputTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UJMItemInspectionWidgetBase> WidgetOwner(
		NewObject<UJMItemInspectionWidgetBase>());
	UJMItemInspectionWidgetBase* Widget = WidgetOwner.Get();
	if (!TestNotNull(TEXT("Inspection widget exists"), Widget))
	{
		return false;
	}

	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	TestTrue(
		TEXT("The full-screen inspection widget participates in Slate pointer hit testing"),
		SlateWidget->GetVisibility().IsHitTestVisible());

	const FGeometry FullScreenGeometry = FGeometry::MakeRoot(
		FVector2D(1920.0f, 1080.0f),
		FSlateLayoutTransform());
	TSet<FKey> PressedButtons;
	PressedButtons.Add(EKeys::LeftMouseButton);
	const FPointerEvent PointerNearScreenCorner(
		0,
		FVector2D(50.0f, 50.0f),
		FVector2D(50.0f, 50.0f),
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());

	const FReply Reply = Widget->NativeOnMouseButtonDown(
		FullScreenGeometry,
		PointerNearScreenCorner);

	TestTrue(
		TEXT("A left click anywhere inside the full inspection widget starts preview dragging"),
		Reply.IsEventHandled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJMItemInspectionPreviewFillsAvailableLayoutTest,
	"JM.ItemInspector.WidgetLayout.PreviewFillsAvailableArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMItemInspectionPreviewFillsAvailableLayoutTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UJMItemInspectionWidgetBase> WidgetOwner(
		NewObject<UJMItemInspectionWidgetBase>());
	UJMItemInspectionWidgetBase* Widget = WidgetOwner.Get();
	if (!TestNotNull(TEXT("Inspection widget exists"), Widget))
	{
		return false;
	}

	Widget->SetInspectionData(nullptr);
	Widget->TakeWidget();
	USizeBox* PreviewSizeBox = Cast<USizeBox>(Widget->GetWidgetFromName(TEXT("PreviewSizeBox")));
	if (!TestNotNull(TEXT("Preview size box exists"), PreviewSizeBox))
	{
		return false;
	}

	TestFalse(
		TEXT("Preview width is not constrained to the legacy square"),
		PreviewSizeBox->IsWidthOverride());
	TestFalse(
		TEXT("Preview height is not constrained to the legacy square"),
		PreviewSizeBox->IsHeightOverride());

	UScaleBoxSlot* PreviewSlot = Cast<UScaleBoxSlot>(PreviewSizeBox->Slot);
	if (!TestNotNull(TEXT("Preview occupies a scale-box slot"), PreviewSlot))
	{
		return false;
	}
	TestEqual(TEXT("Preview fills the available width"), PreviewSlot->GetHorizontalAlignment(), HAlign_Fill);
	TestEqual(TEXT("Preview fills the available height"), PreviewSlot->GetVerticalAlignment(), VAlign_Fill);

	UScaleBox* ContentScaleBox = Cast<UScaleBox>(Widget->GetWidgetFromName(TEXT("ContentRow")));
	if (!TestNotNull(TEXT("Preview content scale box exists"), ContentScaleBox))
	{
		return false;
	}
	TestEqual(
		TEXT("Preview may scale up to fill a larger viewport"),
		ContentScaleBox->GetStretchDirection(),
		EStretchDirection::Both);

	UImage* PreviewImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("PreviewImage")));
	if (!TestNotNull(TEXT("Preview image exists"), PreviewImage))
	{
		return false;
	}
	UScaleBox* PreviewAspectBox = Cast<UScaleBox>(PreviewImage->GetParent());
	if (!TestNotNull(TEXT("Preview image is wrapped by an aspect-preserving scale box"), PreviewAspectBox))
	{
		return false;
	}
	TestEqual(TEXT("Preview texture uses aspect-fit scaling"), PreviewAspectBox->GetStretch(), EStretch::ScaleToFit);
	TestEqual(
		TEXT("Preview texture can scale in both directions"),
		PreviewAspectBox->GetStretchDirection(),
		EStretchDirection::Both);
	return true;
}

#endif
