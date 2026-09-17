#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"

class UBalhwajeomTutorialOverlayPresenter;


/**
 * Swallows every press while a tutorial overlay is on screen.
 *
 * A tutorial screen has to be read, so nothing behind it may react -- not the tablet
 * buttons underneath it, not movement, not the camera. Intercepting ahead of the viewport
 * means no game input system sees the press at all, which is the same approach the capture
 * result card takes, and it covers actions added later without a guard of their own.
 */
class FTutorialOverlayInputProcessor : public IInputProcessor
{
public:
	explicit FTutorialOverlayInputProcessor(UBalhwajeomTutorialOverlayPresenter* InPresenter);

	virtual void Tick(
		const float DeltaTime,
		FSlateApplication& SlateApp,
		TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(
		FSlateApplication& SlateApp,
		const FKeyEvent& InKeyEvent) override;

	virtual bool HandleKeyUpEvent(
		FSlateApplication& SlateApp,
		const FKeyEvent& InKeyEvent) override;

	virtual bool HandleMouseButtonDownEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override;

	virtual bool HandleMouseButtonUpEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override;

	/** Without this the second press of a double click would reach the game. */
	virtual bool HandleMouseButtonDoubleClickEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override;

	/** Looking around behind a tutorial screen would be as wrong as walking. */
	virtual bool HandleMouseMoveEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override;

	virtual bool HandleMouseWheelOrGestureEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& InWheelEvent,
		const FPointerEvent* InGestureEvent) override;

	virtual bool HandleAnalogInputEvent(
		FSlateApplication& SlateApp,
		const FAnalogInputEvent& InAnalogInputEvent) override;

	virtual const TCHAR* GetDebugName() const override
	{
		return TEXT("BalhwajeomTutorialOverlay");
	}

private:
	/** False once the presenter has gone, so a stale processor never eats real input. */
	bool IsBlocking() const;

	TWeakObjectPtr<UBalhwajeomTutorialOverlayPresenter> Presenter;
};
