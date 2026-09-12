#include "UI/JMInteractionPromptWidgetBase.h"

#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UJMInteractionPromptWidgetBase::RebuildWidget()
{
    // A Widget Blueprint subclass owns its Designer tree. Native Slate is only the fallback widget.
    if (WidgetTree && WidgetTree->RootWidget)
    {
        // The native fallback can leave the preview UserWidget collapsed before a Designer tree exists.
        // Always expose the Blueprint tree while editing so PromptText remains visible in the Designer.
        if (IsDesignTime())
        {
            SetVisibility(ESlateVisibility::Visible);
        }
        return Super::RebuildWidget();
    }

    NativePromptFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FMath::Max(1, AppliedStyle.FontSize));

    TSharedRef<SOverlay> Root = SNew(SOverlay);
    Root->AddSlot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SAssignNew(NativeOffsetBox, SBox)
            [
                SAssignNew(NativePromptContainer, SBorder)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SAssignNew(NativePromptText, STextBlock)
                    .Font(NativePromptFont)
                    .Justification(AppliedStyle.TextJustification.GetValue())
                ]
            ]
        ];

    ApplyPromptStyle_Implementation(AppliedStyle);
    SetVisibility(AppliedStyle.HiddenMode == EJMInteractionPromptHiddenMode::Hidden
        ? ESlateVisibility::Hidden
        : ESlateVisibility::Collapsed);
    return Root;
}

void UJMInteractionPromptWidgetBase::SetInteractionPrompt_Implementation(const FJMInteractionPrompt& Prompt)
{
    SetPromptText(Prompt.DisplayText);
    SetPromptVisible(Prompt.bVisible && !Prompt.DisplayText.IsEmpty());
}

void UJMInteractionPromptWidgetBase::ClearInteractionPrompt_Implementation()
{
    SetPromptVisible(false);
    SetPromptText(FText::GetEmpty());
}

void UJMInteractionPromptWidgetBase::SetPromptText_Implementation(const FText& NewPrompt)
{
    if (DisplayedPromptText.EqualTo(NewPrompt))
    {
        return;
    }

    DisplayedPromptText = NewPrompt;
    if (NativePromptText.IsValid())
    {
        NativePromptText->SetText(NewPrompt);
    }
    if (PromptText)
    {
        PromptText->SetText(NewPrompt);
    }
    OnPromptTextChanged.Broadcast(NewPrompt);
}

void UJMInteractionPromptWidgetBase::SetPromptVisible_Implementation(bool bVisible)
{
    const bool bVisibilityChanged = bPromptVisible != bVisible;
    bPromptVisible = bVisible;
    const ESlateVisibility HiddenVisibility = AppliedStyle.HiddenMode == EJMInteractionPromptHiddenMode::Hidden
        ? ESlateVisibility::Hidden
        : ESlateVisibility::Collapsed;
    SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : HiddenVisibility);
    if (bVisibilityChanged)
    {
        if (bVisible) OnPromptShown.Broadcast();
        else OnPromptHidden.Broadcast();
    }
}

void UJMInteractionPromptWidgetBase::ApplyPromptStyle_Implementation(const FJMInteractionPromptStyle& NewStyle)
{
    AppliedStyle = NewStyle;
    AppliedStyle.FontSize = FMath::Clamp(AppliedStyle.FontSize, 1, 128);
    AppliedStyle.RenderScale.X = FMath::Max(AppliedStyle.RenderScale.X, 0.01);
    AppliedStyle.RenderScale.Y = FMath::Max(AppliedStyle.RenderScale.Y, 0.01);

    if (NativeOffsetBox.IsValid())
    {
        NativeOffsetBox->SetRenderTransform(FSlateRenderTransform(FVector2f(
            static_cast<float>(AppliedStyle.ScreenOffset.X),
            static_cast<float>(AppliedStyle.ScreenOffset.Y))));
    }
    if (NativePromptContainer.IsValid())
    {
        NativePromptContainer->SetPadding(AppliedStyle.Padding);
        NativePromptContainer->SetRenderTransformPivot(FVector2f(0.5f, 0.5f));
        NativePromptContainer->SetRenderTransform(FSlateRenderTransform(FScale2D(
            static_cast<float>(AppliedStyle.RenderScale.X),
            static_cast<float>(AppliedStyle.RenderScale.Y))));
    }
    if (NativePromptText.IsValid())
    {
        NativePromptFont.Size = AppliedStyle.FontSize;
        NativePromptText->SetFont(NativePromptFont);
        NativePromptText->SetJustification(AppliedStyle.TextJustification.GetValue());
    }

    UWidget* StyleTarget = PromptContainer ? static_cast<UWidget*>(PromptContainer) : static_cast<UWidget*>(PromptText);
    if (StyleTarget)
    {
        // The Designer target remains centered by its anchors/alignment; only its internal render transform moves/scales.
        StyleTarget->SetRenderTranslation(AppliedStyle.ScreenOffset);
        StyleTarget->SetRenderScale(AppliedStyle.RenderScale);
        StyleTarget->SetRenderTransformPivot(FVector2D(0.5, 0.5));
    }
    if (PromptContainer)
    {
        PromptContainer->SetPadding(AppliedStyle.Padding);
    }
    if (PromptText)
    {
        FSlateFontInfo FontInfo = PromptText->GetFont();
        FontInfo.Size = AppliedStyle.FontSize;
        PromptText->SetFont(FontInfo);
        PromptText->SetJustification(AppliedStyle.TextJustification.GetValue());
    }

    if (!bPromptVisible)
    {
        SetVisibility(AppliedStyle.HiddenMode == EJMInteractionPromptHiddenMode::Hidden
            ? ESlateVisibility::Hidden
            : ESlateVisibility::Collapsed);
    }
    OnPromptStyleApplied.Broadcast(AppliedStyle);
}
