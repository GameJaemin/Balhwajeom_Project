#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomScreenFadeWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBalhwajeomFadeFinished);

/** Full-screen black overlay with completion-driven fades. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomScreenFadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Fade")
	FBalhwajeomFadeFinished OnFadeToBlackFinished;

	UPROPERTY(BlueprintAssignable, Category = "Fade")
	FBalhwajeomFadeFinished OnFadeFromBlackFinished;

	UFUNCTION(BlueprintCallable, Category = "Fade")
	void SetBlackImmediately();

	UFUNCTION(BlueprintCallable, Category = "Fade")
	void FadeToBlack(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Fade")
	void FadeFromBlack(float Duration);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BeginFade(float TargetOpacity, float Duration);

	float FadeStartOpacity = 1.0f;
	float FadeTargetOpacity = 1.0f;
	float FadeDuration = 0.0f;
	float FadeElapsed = 0.0f;
	bool bFading = false;
};
