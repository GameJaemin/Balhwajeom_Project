#include "Tutorial/BalhwajeomTutorialOverlayInput.h"

#include "CameraSystem/BalhwajeomCapturePhotoDismissInput.h"
#include "Tutorial/BalhwajeomTutorialOverlayPresenter.h"

FTutorialOverlayInputProcessor::FTutorialOverlayInputProcessor(
	UBalhwajeomTutorialOverlayPresenter* InPresenter)
	: Presenter(InPresenter)
{
}


bool FTutorialOverlayInputProcessor::IsBlocking() const
{
	const UBalhwajeomTutorialOverlayPresenter* LivePresenter = Presenter.Get();
	return LivePresenter && LivePresenter->IsOverlayOnScreen();
}


bool FTutorialOverlayInputProcessor::HandleKeyDownEvent(
	FSlateApplication& SlateApp,
	const FKeyEvent& InKeyEvent)
{
	if (!IsBlocking())
	{
		return false;
	}

	// Same "is this an intent to continue" policy as the capture result card, so any key
	// means the same thing in both places: no repeats, no modifiers, no function or
	// console keys. Presses that fail it are still swallowed, just not acted on.
	if (BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey(
		InKeyEvent.GetKey(), InKeyEvent.IsRepeat()))
	{
		Presenter->RequestDismiss();
	}
	return true;
}


bool FTutorialOverlayInputProcessor::HandleKeyUpEvent(
	FSlateApplication& SlateApp,
	const FKeyEvent& InKeyEvent)
{
	// Releases always pass through, even while blocking. Swallowing one leaves the game
	// believing the key is still held: hold W as the overlay appears, let go behind it,
	// and the character walks away by itself the moment the overlay closes.
	return false;
}


bool FTutorialOverlayInputProcessor::HandleMouseButtonDownEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& MouseEvent)
{
	if (!IsBlocking())
	{
		return false;
	}

	if (BalhwajeomCapturePhotoDismissInput::ShouldDismissOnKey(
		MouseEvent.GetEffectingButton(), false))
	{
		Presenter->RequestDismiss();
	}
	return true;
}


bool FTutorialOverlayInputProcessor::HandleMouseButtonUpEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& MouseEvent)
{
	return IsBlocking();
}


bool FTutorialOverlayInputProcessor::HandleMouseButtonDoubleClickEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& MouseEvent)
{
	return HandleMouseButtonDownEvent(SlateApp, MouseEvent);
}


bool FTutorialOverlayInputProcessor::HandleMouseMoveEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& MouseEvent)
{
	return IsBlocking();
}


bool FTutorialOverlayInputProcessor::HandleMouseWheelOrGestureEvent(
	FSlateApplication& SlateApp,
	const FPointerEvent& InWheelEvent,
	const FPointerEvent* InGestureEvent)
{
	return IsBlocking();
}


bool FTutorialOverlayInputProcessor::HandleAnalogInputEvent(
	FSlateApplication& SlateApp,
	const FAnalogInputEvent& InAnalogInputEvent)
{
	return IsBlocking();
}
