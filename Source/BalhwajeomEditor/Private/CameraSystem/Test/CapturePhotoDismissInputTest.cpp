#include "CameraSystem/BalhwajeomCapturePhotoDismissInput.h"

#include "GameFramework/InputSettings.h"
#include "Misc/AutomationTest.h"


#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapturePhotoDismissKeyPolicyTest,
	"Balhwajeom.Camera.CapturePhotoDismiss.KeyPolicy",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FCapturePhotoDismissKeyPolicyTest::RunTest(const FString& Parameters)
{
	using BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey;

	// Ordinary presses dismiss, whatever they normally do.
	TestTrue(
		TEXT("Left click should dismiss"),
		ShouldDismissOnKey(EKeys::LeftMouseButton, false));
	TestTrue(
		TEXT("Tab should dismiss, not open the tablet"),
		ShouldDismissOnKey(EKeys::Tab, false));
	TestTrue(
		TEXT("Escape should dismiss, not exit camera mode"),
		ShouldDismissOnKey(EKeys::Escape, false));
	TestTrue(
		TEXT("A letter key should dismiss"),
		ShouldDismissOnKey(EKeys::E, false));
	TestTrue(
		TEXT("Right click should dismiss"),
		ShouldDismissOnKey(EKeys::RightMouseButton, false));

	// The wheel is the zoom control and turns by accident; its click is an ordinary button.
	TestTrue(
		TEXT("Wheel click should dismiss"),
		ShouldDismissOnKey(EKeys::MiddleMouseButton, false));
	TestFalse(
		TEXT("Wheel scroll should never dismiss"),
		ShouldDismissOnKey(EKeys::MouseWheelAxis, false));
	TestFalse(
		TEXT("Mouse movement should never dismiss"),
		ShouldDismissOnKey(EKeys::MouseX, false));
	TestFalse(
		TEXT("Mouse movement should never dismiss"),
		ShouldDismissOnKey(EKeys::MouseY, false));

	// Holding a key must read as one press, not a stream of them.
	TestFalse(
		TEXT("Key repeats should not count as fresh presses"),
		ShouldDismissOnKey(EKeys::E, true));

	// A hand resting on a modifier is not an intent to continue.
	TestFalse(
		TEXT("Shift alone should not dismiss"),
		ShouldDismissOnKey(EKeys::LeftShift, false));
	TestFalse(
		TEXT("Ctrl alone should not dismiss"),
		ShouldDismissOnKey(EKeys::LeftControl, false));
	TestFalse(
		TEXT("Alt alone should not dismiss"),
		ShouldDismissOnKey(EKeys::LeftAlt, false));

	// Consuming a tool shortcut would disable the tool, not just close the card.
	TestFalse(
		TEXT("F9 should stay available for screenshots"),
		ShouldDismissOnKey(EKeys::F9, false));
	TestFalse(
		TEXT("F11 should stay available"),
		ShouldDismissOnKey(EKeys::F11, false));

	if (const UInputSettings* InputSettings = GetDefault<UInputSettings>())
	{
		for (const FKey& ConsoleKey : InputSettings->ConsoleKeys)
		{
			TestFalse(
				*FString::Printf(
					TEXT("Console key '%s' should stay available"),
					*ConsoleKey.ToString()),
				ShouldDismissOnKey(ConsoleKey, false));
		}
	}

	TestFalse(
		TEXT("An invalid key should be ignored"),
		ShouldDismissOnKey(FKey(), false));

	return true;
}

#endif
