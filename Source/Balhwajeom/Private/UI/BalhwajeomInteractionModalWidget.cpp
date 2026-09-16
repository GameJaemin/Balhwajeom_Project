#include "UI/BalhwajeomInteractionModalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

UBalhwajeomInteractionModalWidget::UBalhwajeomInteractionModalWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UBalhwajeomInteractionModalWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
}

void UBalhwajeomInteractionModalWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), TEXT("InteractionModalRoot"));
	WidgetTree->RootWidget = RootOverlay;
}

bool UBalhwajeomInteractionModalWidget::Present(
	TSubclassOf<UUserWidget> ContentClass,
	const FText& DocumentText,
	const TArray<FText>& NewlyGrantedKeywords)
{
	BuildWidgetTree();
	if (!RootOverlay || !ContentClass)
	{
		return false;
	}

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		ContentWidget = CreateWidget<UUserWidget>(OwningPlayer, ContentClass);
	}
	else if (UWorld* World = GetWorld())
	{
		ContentWidget = CreateWidget<UUserWidget>(World, ContentClass);
	}
	if (!ContentWidget)
	{
		return false;
	}
	RootOverlay->InsertChildAt(0, ContentWidget);
	if (UOverlaySlot* ContentSlot = Cast<UOverlaySlot>(ContentWidget->Slot))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}

	for (const FName CandidateName : {FName(TEXT("TXT_Document")), FName(TEXT("DiaryText"))})
	{
		if (UTextBlock* DocumentLabel =
			Cast<UTextBlock>(ContentWidget->GetWidgetFromName(CandidateName)))
		{
			DocumentLabel->SetText(DocumentText);
			break;
		}
	}

	for (int32 KeywordIndex = 0; KeywordIndex < 2; ++KeywordIndex)
	{
		const bool bHasKeyword = NewlyGrantedKeywords.IsValidIndex(KeywordIndex) &&
			!NewlyGrantedKeywords[KeywordIndex].IsEmpty();
		const ESlateVisibility KeywordVisibility = bHasKeyword
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed;

		const FName TextWidgetName(*FString::Printf(
			TEXT("Text_Keyword%d"), KeywordIndex + 1));
		if (UTextBlock* KeywordText = Cast<UTextBlock>(
			ContentWidget->GetWidgetFromName(TextWidgetName)))
		{
			KeywordText->SetText(bHasKeyword
				? NewlyGrantedKeywords[KeywordIndex]
				: FText::GetEmpty());
			KeywordText->SetVisibility(KeywordVisibility);
		}

		const FName ImageWidgetName(*FString::Printf(
			TEXT("Image_KeyWord_%d"), KeywordIndex + 1));
		if (UWidget* KeywordImage = ContentWidget->GetWidgetFromName(ImageWidgetName))
		{
			KeywordImage->SetVisibility(KeywordVisibility);
		}
	}
	return true;
}

FReply UBalhwajeomInteractionModalWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::F)
	{
		RequestClose();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBalhwajeomInteractionModalWidget::RequestClose()
{
	OnCloseRequested.Broadcast();
}
