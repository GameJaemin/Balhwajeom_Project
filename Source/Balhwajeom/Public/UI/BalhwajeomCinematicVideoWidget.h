#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCinematicVideoWidget.generated.h"

class UButton;
class UImage;
class UMediaTexture;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCinematicSkipRequestedSignature);

/** Full-screen surface used by the intro controller for pre-rendered movies. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomCinematicVideoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void SetMediaTexture(UMediaTexture* MediaTexture);

	/** Broadcast when the viewer clicks BTN_Skip, if present in the WBP. */
	UPROPERTY(BlueprintAssignable, Category = "Cinematic Video")
	FCinematicSkipRequestedSignature OnSkipRequested;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Video;

	/** Optional. Add a button named BTN_Skip in the WBP to enable the skip control. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Skip;

private:
	UFUNCTION()
	void HandleSkipClicked();
};
