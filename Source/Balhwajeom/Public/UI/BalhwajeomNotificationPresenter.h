#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BalhwajeomNotificationPresenter.generated.h"

class UBalhwajeomNotificationBannerWidget;


/**
 * Owns the one top-of-screen notification banner.
 *
 * Lives on the player controller because it needs a viewport, the way the tutorial overlay
 * presenter does. It keeps a single banner widget and re-shows it, so two notifications in
 * a row replace each other instead of stacking.
 */
UCLASS(ClassGroup = (Balhwajeom), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomNotificationPresenter : public UActorComponent
{
	GENERATED_BODY()

public:
	UBalhwajeomNotificationPresenter();

	/** Puts Message on screen for the banner's own hold time. An empty text does nothing. */
	UFUNCTION(BlueprintCallable, Category = "Notification")
	void ShowNotification(const FText& Message);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification")
	TSubclassOf<UBalhwajeomNotificationBannerWidget> BannerWidgetClass;

	/** Below the tutorial overlay (2000) and the interaction modal (1300): this never blocks them. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Notification")
	int32 ViewportZOrder = 1200;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomNotificationBannerWidget> BannerWidget;
};


/**
 * Reaches the local player's notification banner from anywhere -- a door, a subsystem, a
 * Blueprint -- without the caller needing to know where the presenter lives.
 */
namespace BalhwajeomNotification
{
	/** No-op when there is no local controller or the text is empty. */
	BALHWAJEOM_API void Show(const UObject* WorldContextObject, const FText& Message);
}
