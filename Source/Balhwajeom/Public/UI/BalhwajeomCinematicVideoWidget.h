#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomCinematicVideoWidget.generated.h"

class UImage;
class UMediaTexture;

/** Full-screen surface used by the intro controller for pre-rendered movies. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomCinematicVideoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cinematic Video")
	void SetMediaTexture(UMediaTexture* MediaTexture);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_Video;
};
