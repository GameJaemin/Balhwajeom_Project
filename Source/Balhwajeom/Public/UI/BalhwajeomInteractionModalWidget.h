#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomInteractionModalWidget.generated.h"

class UOverlay;
class UUserWidget;

DECLARE_MULTICAST_DELEGATE(FOnBalhwajeomInteractionModalCloseRequested);

/** Hosts a data-authored screen widget and overlays newly acquired keyword chips. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomInteractionModalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomInteractionModalWidget(const FObjectInitializer& ObjectInitializer);

	bool Present(
		TSubclassOf<UUserWidget> ContentClass,
		const FText& DocumentText,
		const TArray<FText>& NewlyGrantedKeywords);

	FOnBalhwajeomInteractionModalCloseRequested OnCloseRequested;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(
		const FGeometry& InGeometry,
		const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void RequestClose();

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ContentWidget;
};
