#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Input/Events.h"
#include "InputCoreTypes.h"
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

#endif
