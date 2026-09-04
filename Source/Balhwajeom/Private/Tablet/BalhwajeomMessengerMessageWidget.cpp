#include "Tablet/BalhwajeomMessengerMessageWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Tablet/BalhwajeomMessengerKeywordWidget.h"

namespace
{
	const TCHAR* KeywordWidgetClassPath =
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerKeyword.WBP_MessengerKeyword_C");
}

void UBalhwajeomMessengerMessageWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RenderMessage();
}

void UBalhwajeomMessengerMessageWidget::SetupMessage(const FST_MessengerMessage& InMessageData)
{
	MessageData = InMessageData;
	RenderMessage();
}

bool UBalhwajeomMessengerMessageWidget::IsKeywordDataValid(
	const FST_MessengerMessage& InMessageData)
{
	const FString Message = InMessageData.Message.ToString();
	const FString Keyword = InMessageData.KeywordText.ToString();
	if (Keyword.IsEmpty() || InMessageData.WordID.IsEmpty())
	{
		return false;
	}

	const int32 FirstIndex = Message.Find(Keyword, ESearchCase::CaseSensitive);
	if (FirstIndex == INDEX_NONE)
	{
		return false;
	}

	// The prototype deliberately supports a maximum of one keyword occurrence.
	return Message.Find(
		Keyword,
		ESearchCase::CaseSensitive,
		ESearchDir::FromStart,
		FirstIndex + Keyword.Len()) == INDEX_NONE;
}

void UBalhwajeomMessengerMessageWidget::RenderMessage()
{
	if (TXT_SenderName)
	{
		TXT_SenderName->SetText(MessageData.SenderName);
		TXT_SenderName->SetJustification(
			MessageData.bIsPlayer ? ETextJustify::Right : ETextJustify::Left);
	}

	if (BRD_Bubble)
	{
		BRD_Bubble->SetBrushColor(
			MessageData.bIsPlayer
				? FLinearColor(0.53f, 0.35f, 0.12f, 0.96f)
				: FLinearColor(0.25f, 0.18f, 0.11f, 0.96f));
	}

	auto SetSpacerRule = [](USpacer* Spacer, const ESlateSizeRule::Type Rule)
	{
		if (Spacer)
		{
			if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Spacer->Slot))
			{
				Slot->SetSize(FSlateChildSize(Rule));
			}
		}
	};
	SetSpacerRule(Spacer_Left, MessageData.bIsPlayer ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic);
	SetSpacerRule(Spacer_Right, MessageData.bIsPlayer ? ESlateSizeRule::Automatic : ESlateSizeRule::Fill);

	bHasInteractiveKeyword = false;
	if (!WB_MessageContent)
	{
		return;
	}

	WB_MessageContent->ClearChildren();
	if (!IsKeywordDataValid(MessageData))
	{
		AddTextSegment(MessageData.Message.ToString());
		return;
	}

	const FString FullMessage = MessageData.Message.ToString();
	const FString Keyword = MessageData.KeywordText.ToString();
	const int32 KeywordIndex = FullMessage.Find(Keyword, ESearchCase::CaseSensitive);
	AddTextSegment(FullMessage.Left(KeywordIndex));

	const TSubclassOf<UBalhwajeomMessengerKeywordWidget> ResolvedClass = ResolveKeywordWidgetClass();
	UWorld* World = GetWorld();
	UBalhwajeomMessengerKeywordWidget* KeywordWidget = World && ResolvedClass
		? CreateWidget<UBalhwajeomMessengerKeywordWidget>(World, ResolvedClass)
		: nullptr;
	if (!KeywordWidget)
	{
		WB_MessageContent->ClearChildren();
		AddTextSegment(FullMessage);
		return;
	}

#if WITH_EDITOR
	if (!GetOwningLocalPlayer())
	{
		KeywordWidget->InitializeForAutomatedTest();
	}
#endif
	KeywordWidget->SetupKeyword(MessageData.KeywordText, MessageData.WordID);
	WB_MessageContent->AddChild(KeywordWidget);
	AddTextSegment(FullMessage.Mid(KeywordIndex + Keyword.Len()));
	bHasInteractiveKeyword = true;
}

void UBalhwajeomMessengerMessageWidget::AddTextSegment(const FString& Segment)
{
	if (!WB_MessageContent || Segment.IsEmpty())
	{
		return;
	}

	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(FText::FromString(Segment));
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
	Text->SetAutoWrapText(false);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 22;
	Text->SetFont(Font);
	WB_MessageContent->AddChild(Text);
}

TSubclassOf<UBalhwajeomMessengerKeywordWidget>
UBalhwajeomMessengerMessageWidget::ResolveKeywordWidgetClass()
{
	if (!KeywordWidgetClass)
	{
		KeywordWidgetClass = LoadClass<UBalhwajeomMessengerKeywordWidget>(nullptr, KeywordWidgetClassPath);
	}
	return KeywordWidgetClass;
}
