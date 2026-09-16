#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "InputCoreTypes.h"

class ABalhwajeomEvidenceCameraHUD;


namespace BalhwajeomCapturePhotoDismissInput
{
	/**
	 * Whether a raw press should count as "dismiss the capture card".
	 * Kept separate from the processor so the policy is testable without Slate.
	 */
	BALHWAJEOM_API bool ShouldDismissOnKey(const FKey& Key, bool bIsRepeat);
}


/**
 * Swallows presses at the Slate level while the capture result card is on screen.
 *
 * The card used to be dismissed by a left click riding on TakePhoto, which left every other
 * action to be refused one guard at a time - and TAB slipped through, arming the tablet's
 * pending-open while the camera was refusing to leave. Intercepting ahead of the viewport
 * means no game input system sees the press at all, so actions added later are covered
 * without needing a guard of their own.
 */
class FCapturePhotoDismissInputProcessor : public IInputProcessor
{
public:
	explicit FCapturePhotoDismissInputProcessor(ABalhwajeomEvidenceCameraHUD* InCameraHUD);

	virtual void Tick(
		const float DeltaTime,
		FSlateApplication& SlateApp,
		TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(
		FSlateApplication& SlateApp,
		const FKeyEvent& InKeyEvent) override;

	virtual bool HandleMouseButtonDownEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override;

	/** Without this the second press of a double click would reach the game. */
	virtual bool HandleMouseButtonDoubleClickEvent(
		FSlateApplication& SlateApp,
		const FPointerEvent& MouseEvent) override;

	virtual const TCHAR* GetDebugName() const override
	{
		return TEXT("BalhwajeomCapturePhotoDismiss");
	}

private:
	TWeakObjectPtr<ABalhwajeomEvidenceCameraHUD> CameraHUD;
};
