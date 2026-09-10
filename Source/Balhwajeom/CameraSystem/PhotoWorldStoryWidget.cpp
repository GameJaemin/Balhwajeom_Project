#include "PhotoWorldStoryWidget.h"

#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"

UPhotoWorldStoryWidget::UPhotoWorldStoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> DefaultKoreanFont(
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-4Regular_Font.Freesentation-4Regular_Font"));
	if (DefaultKoreanFont.Succeeded())
	{
		StoryFont.FontObject = DefaultKoreanFont.Object;
	}
	StoryFont.Size = 32;
	StoryFont.OutlineSettings.OutlineSize = 2;
	StoryFont.OutlineSettings.OutlineColor = FLinearColor::Black;
}

void UPhotoWorldStoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	bool bCreatedNativeFallback = false;
	if (!StoryText && WidgetTree)
	{
		StoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StoryText"));
		WidgetTree->RootWidget = StoryText;
		bCreatedNativeFallback = true;
	}

	if (StoryText)
	{
		// Story text only wraps at explicit newline characters authored in DT_Photos.
		StoryText->SetAutoWrapText(false);
		StoryText->SetWrapTextAt(0.0f);

		// A real Designer TextBlock owns its own appearance. These values are only the native fallback.
		if (bCreatedNativeFallback)
		{
			StoryText->SetJustification(ETextJustify::Center);
			StoryText->SetFont(StoryFont);
			StoryText->SetColorAndOpacity(FSlateColor(StoryColor));
			StoryText->SetShadowColorAndOpacity(StoryShadowColor);
			StoryText->SetShadowOffset(StoryShadowOffset);
		}
	}
}

void UPhotoWorldStoryWidget::SetStoryText(const FText& NewText)
{
	if (StoryText)
	{
		StoryText->SetText(NewText);
	}
}

int32 UPhotoWorldStoryWidget::GetStoryFontSize() const
{
	return StoryText ? StoryText->GetFont().Size : 0;
}

void UPhotoWorldStoryWidget::SetStoryFontSize(const int32 NewFontSize)
{
	if (!StoryText)
	{
		return;
	}

	FSlateFontInfo Font = StoryText->GetFont();
	Font.Size = FMath::Max(NewFontSize, 1);
	StoryText->SetFont(Font);
}
