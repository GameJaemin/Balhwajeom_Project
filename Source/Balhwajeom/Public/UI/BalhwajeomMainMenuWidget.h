#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMainMenuWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBalhwajeomStartRequested);

/** Runtime contract for the editable WBP_MainMenu designer asset. */
UCLASS(Blueprintable)
class BALHWAJEOM_API UBalhwajeomMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Title")
	FBalhwajeomStartRequested OnStartRequested;

	UFUNCTION(BlueprintCallable, Category = "Title")
	void SetStartButtonEnabled(bool bEnabled);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Start;

private:
	UFUNCTION()
	void HandleStartClicked();

	bool bStartAccepted = false;
};
