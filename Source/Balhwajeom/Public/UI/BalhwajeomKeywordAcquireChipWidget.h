#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomKeywordAcquireChipWidget.generated.h"

class UBorder;
class UFont;
class UTextBlock;
class UTexture2D;

/** Reusable keyword-acquisition pill shared by photo cards and interaction modals. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomKeywordAcquireChipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetKeywordAppearance(
		const FText& Keyword,
		UFont* FontAsset,
		int32 FontSize,
		const FLinearColor& TextColor,
		const FLinearColor& BackgroundColor,
		UTexture2D* BackgroundTexture = nullptr);

protected:
	virtual void NativeOnInitialized() override;

private:
	void EnsureWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PillBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> KeywordLabel;
};
