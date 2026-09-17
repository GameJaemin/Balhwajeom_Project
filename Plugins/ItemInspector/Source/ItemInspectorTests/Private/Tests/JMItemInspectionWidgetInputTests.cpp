#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "ItemInspection/JMItemInspectionSubsystem.h"
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

namespace
{
	FPointerEvent MakeButtonEvent(const FKey& Button, const FVector2D& Position)
	{
		TSet<FKey> PressedButtons;
		PressedButtons.Add(Button);
		return FPointerEvent(
			0,
			Position,
			Position,
			PressedButtons,
			Button,
			0.0f,
			FModifierKeysState());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJMItemInspectionPointerFocusRetentionTest,
	"JM.ItemInspector.WidgetInput.PointerKeepsKeyboardFocus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMItemInspectionPointerFocusRetentionTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UJMItemInspectionWidgetBase> WidgetOwner(
		NewObject<UJMItemInspectionWidgetBase>());
	UJMItemInspectionWidgetBase* Widget = WidgetOwner.Get();
	if (!TestNotNull(TEXT("Inspection widget exists"), Widget))
	{
		return false;
	}

	Widget->TakeWidget();
	const FGeometry FullScreenGeometry = FGeometry::MakeRoot(
		FVector2D(1920.0f, 1080.0f),
		FSlateLayoutTransform());
	const FVector2D PointerPosition(960.0f, 540.0f);

	// A right click that is not consumed here reaches SViewport, which takes keyboard focus for the
	// game viewport and leaves F and Escape with nothing to close the inspector.
	const FReply RightDownReply = Widget->NativeOnPreviewMouseButtonDown(
		FullScreenGeometry,
		MakeButtonEvent(EKeys::RightMouseButton, PointerPosition));
	TestTrue(
		TEXT("A right click is consumed before it can reach the game viewport"),
		RightDownReply.IsEventHandled());

	const FReply RightUpReply = Widget->NativeOnMouseButtonUp(
		FullScreenGeometry,
		MakeButtonEvent(EKeys::RightMouseButton, PointerPosition));
	TestTrue(
		TEXT("The matching right release is consumed as well"),
		RightUpReply.IsEventHandled());

	// A consumed right click must not be mistaken for the start of a rotation drag.
	const FReply MoveAfterRightClickReply = Widget->NativeOnMouseMove(
		FullScreenGeometry,
		MakeButtonEvent(EKeys::RightMouseButton, FVector2D(980.0f, 560.0f)));
	TestFalse(
		TEXT("Moving the mouse after a right click does not rotate the preview"),
		MoveAfterRightClickReply.IsEventHandled());

	// Closing must still work once the pointer has been used.
	const FKeyEvent CloseKeyEvent(EKeys::F, FModifierKeysState(), 0, false, 0, 0);
	TestTrue(
		TEXT("F still closes the inspector after a right click"),
		Widget->NativeOnKeyDown(FullScreenGeometry, CloseKeyEvent).IsEventHandled());

	// The left button keeps its bubble-phase behaviour, where the drag starts.
	const FPointerEvent LeftButtonEvent = MakeButtonEvent(EKeys::LeftMouseButton, PointerPosition);
	TestFalse(
		TEXT("The left button is left to the bubble phase"),
		Widget->NativeOnPreviewMouseButtonDown(FullScreenGeometry, LeftButtonEvent).IsEventHandled());
	TestTrue(
		TEXT("The left button still starts a preview drag"),
		Widget->NativeOnMouseButtonDown(FullScreenGeometry, LeftButtonEvent).IsEventHandled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJMItemInspectionFocusRestorePolicyTest,
	"JM.ItemInspector.WidgetInput.FocusRestorePolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMItemInspectionFocusRestorePolicyTest::RunTest(const FString& Parameters)
{
	using JMItemInspectionFocus::ShouldRestoreWidgetFocus;

	TestTrue(
		TEXT("Focus is taken back from the game viewport while inspecting"),
		ShouldRestoreWidgetFocus(true, true, false, true));
	TestFalse(
		TEXT("Focus is left alone while the inspector already owns it"),
		ShouldRestoreWidgetFocus(true, true, true, false));
	TestFalse(
		TEXT("Focus is left with the console or another widget"),
		ShouldRestoreWidgetFocus(true, true, false, false));
	TestFalse(
		TEXT("A closed inspector never takes focus"),
		ShouldRestoreWidgetFocus(false, true, false, true));
	TestFalse(
		TEXT("A widget that left the viewport never takes focus"),
		ShouldRestoreWidgetFocus(true, false, false, true));
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
