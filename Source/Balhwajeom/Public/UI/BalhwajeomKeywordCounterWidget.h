#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomKeywordCounterWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

/** Small gameplay-HUD counter placed directly above the bottom-right TAB hint. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomKeywordCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomKeywordCounterWidget(const FObjectInitializer& ObjectInitializer);

	/** Re-reads the investigation state and displays acquired/total DT_Words rows. */
	void RefreshCount();

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountText;
};
