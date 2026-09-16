#include "UI/BalhwajeomKeywordAcquireChipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

void UBalhwajeomKeywordAcquireChipWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureWidgetTree();
}

void UBalhwajeomKeywordAcquireChipWidget::EnsureWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	PillBorder = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("KeywordPill"));
	KeywordLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("KeywordLabel"));
	PillBorder->SetPadding(FMargin(30.0f, 20.0f));
	PillBorder->SetContent(KeywordLabel);
	WidgetTree->RootWidget = PillBorder;
}

void UBalhwajeomKeywordAcquireChipWidget::SetKeywordAppearance(
	const FText& Keyword,
	UFont* FontAsset,
	const int32 FontSize,
	const FLinearColor& TextColor,
	const FLinearColor& BackgroundColor,
	UTexture2D* BackgroundTexture)
{
	EnsureWidgetTree();
	if (!PillBorder || !KeywordLabel)
	{
		return;
	}

	if (BackgroundTexture)
	{
		PillBorder->SetBrushFromTexture(BackgroundTexture);
	}
	else
	{
		PillBorder->SetBrushColor(BackgroundColor);
	}

	KeywordLabel->SetText(Keyword);
	KeywordLabel->SetColorAndOpacity(FSlateColor(TextColor));
	KeywordLabel->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = KeywordLabel->GetFont();
	if (FontAsset)
	{
		Font.FontObject = FontAsset;
	}
	Font.Size = FontSize;
	KeywordLabel->SetFont(Font);
}
