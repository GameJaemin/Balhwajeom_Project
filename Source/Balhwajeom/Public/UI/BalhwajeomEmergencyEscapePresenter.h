#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BalhwajeomEmergencyEscapePresenter.generated.h"

class UBalhwajeomConfirmPromptWidget;
class UBalhwajeomScreenFadeWidget;


/**
 * The way out when the player is wedged in the level geometry.
 *
 * Asks first, because the escape moves them somewhere they did not walk to. On yes it
 * blacks the screen out, teleports and fades back: the move itself is never visible, since
 * a character sliding across the room reads as a bug rather than a rescue.
 *
 * Lives on the player controller -- it needs the viewport, the input mode and the pawn.
 */
UCLASS(ClassGroup = (Balhwajeom), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomEmergencyEscapePresenter : public UActorComponent
{
	GENERATED_BODY()

public:
	UBalhwajeomEmergencyEscapePresenter();

	/** Bound to the EmergencyEscape action. Ignored while the prompt or a teleport is already running. */
	UFUNCTION(BlueprintCallable, Category = "Emergency Escape")
	void RequestEmergencyEscape();

	/** True from the shortcut being pressed until the player is back in control. */
	UFUNCTION(BlueprintPure, Category = "Emergency Escape")
	bool IsEscapeInProgress() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Widgets")
	TSubclassOf<UBalhwajeomConfirmPromptWidget> PromptWidgetClass;

	/** Reuses the intro's fade layer rather than introducing a second black plane. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Widgets")
	TSoftClassPtr<UBalhwajeomScreenFadeWidget> ScreenFadeWidgetClass;

	/** Above the tutorial overlay: being stuck outranks being taught. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Widgets")
	int32 PromptZOrder = 2100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Text")
	FText PromptTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Text")
	FText PromptMessage;

	/**
	 * Where to stand relative to the door, in the door's own space.
	 *
	 * A gate door actor is placed on its hinge, not in the middle of its panel, so this has
	 * to account for both the step back from the doorway and the offset to its centre.
	 *
	 * The default is measured from room3's GateDoor_Exit: its panel is thin along local X
	 * with the room on the -X side, and its hinge sits 70cm off the panel centre in Y. Z
	 * lifts the capsule to standing height, matching the level's PlayerStart.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Destination")
	FVector EscapeLocalOffset = FVector(-150.0f, 70.0f, 100.0f);

	/**
	 * Which way to face on arrival, relative to the door's own yaw. 0 looks along the door's
	 * local +X, which is back at the doorway from the default offset's side of it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Destination",
		meta = (ClampMin = "-180.0", ClampMax = "180.0"))
	float EscapeLocalYaw = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeToBlackSeconds = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float FadeFromBlackSeconds = 0.5f;

	/** Held at full black around the teleport so the move can never be seen mid-flight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Emergency Escape|Fade",
		meta = (ClampMin = "0.0", Units = "s"))
	float BlackHoldSeconds = 0.15f;

private:
	void HandleConfirmed();
	void HandleCancelled();
	void HandlePromptFadeOutFinished();

	UFUNCTION()
	void HandleFadeToBlackFinished();

	UFUNCTION()
	void HandleFadeFromBlackFinished();

	void PerformTeleport();
	void BeginFadeBackIn();

	/** Nearest gate door's spot, falling back to a PlayerStart when the level has no doors. */
	bool ResolveEscapeDestination(FTransform& OutTransform) const;

	void ApplyPromptInputMode(bool bPromptOpen);
	void ClosePrompt(bool bTeleport);

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomConfirmPromptWidget> PromptWidget;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomScreenFadeWidget> ScreenFadeWidget;

	FTimerHandle BlackHoldTimerHandle;

	bool bPromptOpen = false;
	bool bTeleportPending = false;
	bool bFadingBack = false;

	/** Restored on the way out, so the escape cannot hand back a cursor the game never had. */
	bool bPreviousShowMouseCursor = false;
	bool bChangedMoveInput = false;
	bool bChangedLookInput = false;
};
