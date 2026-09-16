#include "BalhwajeomCapturePhotoDismissInput.h"

#include "BalhwajeomEvidenceCameraHUD.h"
#include "GameFramework/InputSettings.h"

namespace BalhwajeomCapturePhotoDismissInput
{
bool ShouldDismissOnKey(const FKey& Key, const bool bIsRepeat)
{
	// Holding a key down must not read as a second press.
	if (bIsRepeat || !Key.IsValid())
	{
		return false;
	}

	// Mouse movement, wheel scroll and analog sticks are not presses. The wheel is the zoom
	// control and turns by accident, so scrolling must never dismiss the card. The wheel
	// *click* is an ordinary button and is deliberately left in.
	if (Key.IsAnalog())
	{
		return false;
	}

	// A modifier resting under the hand is not an intent to continue.
	if (Key.IsModifierKey())
	{
		return false;
	}

	// Function keys drive tools (screenshot, profiler, PIE eject). Consuming one would
	// silently disable the tool rather than merely dismissing the card.
	static const TSet<FKey> FunctionKeys = {
		EKeys::F1, EKeys::F2, EKeys::F3, EKeys::F4,
		EKeys::F5, EKeys::F6, EKeys::F7, EKeys::F8,
		EKeys::F9, EKeys::F10, EKeys::F11, EKeys::F12
	};
	if (FunctionKeys.Contains(Key))
	{
		return false;
	}

	// Losing the console would make the card impossible to debug from inside a session.
	if (const UInputSettings* InputSettings = GetDefault<UInputSettings>())
	{
		if (InputSettings->ConsoleKeys.Contains(Key))
		{
			return false;
		}
	}

	return true;
}
}


FCapturePhotoDismissInputProcessor::FCapturePhotoDismissInputProcessor(
	ABalhwajeomEvidenceCameraHUD* InCameraHUD)
	: CameraHUD(InCameraHUD)
{
}

bool FCapturePhotoDismissInputProcessor::HandleKeyDownEvent(
	FSlateApplication& SlateApp,
	const FKeyEvent& InKeyEvent)
{
	ABalhwajeomEvidenceCameraHUD* HUD = CameraHUD.Get();
	if (!HUD ||
		!BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey(
			InKeyEvent.GetKey(), InKeyEvent.IsRepeat()))
	{
		return false;
	}

	return HUD->HandleCapturePhotoDismissInput();
}

bool FCapturePhotoDismissInputProcessor::HandleMouseButtonDownEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& MouseEvent)
{
	ABalhwajeomEvidenceCameraHUD* HUD = CameraHUD.Get();
	if (!HUD ||
		!BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey(
			MouseEvent.GetEffectingButton(), false))
	{
		return false;
	}

	return HUD->HandleCapturePhotoDismissInput();
}

bool FCapturePhotoDismissInputProcessor::HandleMouseButtonDoubleClickEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& MouseEvent)
{
	return HandleMouseButtonDownEvent(SlateApp, MouseEvent);
}
