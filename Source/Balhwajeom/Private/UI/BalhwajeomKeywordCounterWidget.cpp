#include "UI/BalhwajeomKeywordCounterWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"

UBalhwajeomKeywordCounterWidget::UBalhwajeomKeywordCounterWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UBalhwajeomKeywordCounterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
	RefreshCount();
}

void UBalhwajeomKeywordCounterWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("KeywordCounterCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	CountText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TXT_GameplayKeywordCount"));
	CountText->SetText(FText::FromString(TEXT("0/0")));
	CountText->SetJustification(ETextJustify::Center);
	CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CountText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	CountText->SetShadowOffset(FVector2D(1.0f, 1.0f));

	FSlateFontInfo Font = CountText->GetFont();
	Font.Size = 18;
	if (UFont* KeywordFont = LoadObject<UFont>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-4Regular_Font.Freesentation-4Regular_Font")))
	{
		Font.FontObject = KeywordFont;
	}
	CountText->SetFont(Font);

	UCanvasPanelSlot* CountSlot = RootCanvas->AddChildToCanvas(CountText);
	CountSlot->SetAnchors(FAnchors(0.0f, 1.0f));
	CountSlot->SetAlignment(FVector2D(1.0f, 1.0f));
	// WBP_HUID's TAB art occupies the bottom-right corner. This centers the
	// counter over that art while keeping it resolution-independent.
	CountSlot->SetPosition(FVector2D(180.0f, -115.0f));
	CountSlot->SetSize(FVector2D(180.0f, 34.0f));
}

void UBalhwajeomKeywordCounterWidget::RefreshCount()
{
	if (!CountText)
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UBalhwajeomInvestigationSubsystem* Investigation = GameInstance
		? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>()
		: nullptr;
	if (!Investigation)
	{
		CountText->SetText(FText::FromString(TEXT("0/0")));
		return;
	}

	TArray<FAcquiredWordRecord> AcquiredWords;
	Investigation->GetAcquiredWords(AcquiredWords);
	// Clamped to the same fixed total the denominator uses, so a save carrying more acquired words
	// than the design count can never read as e.g. "18/14".
	const int32 AcquiredCount = FMath::Min(
		AcquiredWords.Num(), UBalhwajeomInvestigationSubsystem::DisplayedTotalKeywordCount);
	CountText->SetText(FText::Format(
		NSLOCTEXT("HUD", "GameplayKeywordCount", "{0}/{1}"),
		FText::AsNumber(AcquiredCount),
		FText::AsNumber(UBalhwajeomInvestigationSubsystem::DisplayedTotalKeywordCount)));
}
