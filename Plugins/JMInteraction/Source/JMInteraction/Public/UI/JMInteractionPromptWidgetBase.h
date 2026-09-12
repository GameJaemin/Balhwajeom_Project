#pragma once

#include "Blueprint/UserWidget.h"
#include "Types/JMInteractionTypes.h"
#include "JMInteractionPromptWidgetBase.generated.h"

class STextBlock;
class SBorder;
class SBox;
class UBorder;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMPromptTextEvent, FText, PromptText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJMPromptVisibilityEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJMPromptStyleEvent, FJMInteractionPromptStyle, PromptStyle);

UCLASS(BlueprintType, Blueprintable)
class JMINTERACTION_API UJMInteractionPromptWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "JM Interaction|UI")
    FJMPromptVisibilityEvent OnPromptShown;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction|UI")
    FJMPromptVisibilityEvent OnPromptHidden;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction|UI")
    FJMPromptTextEvent OnPromptTextChanged;

    UPROPERTY(BlueprintAssignable, Category = "JM Interaction|UI")
    FJMPromptStyleEvent OnPromptStyleApplied;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction|UI")
    void SetInteractionPrompt(const FJMInteractionPrompt& Prompt);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction|UI")
    void ClearInteractionPrompt();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction|UI")
    void SetPromptText(const FText& NewPrompt);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction|UI")
    void SetPromptVisible(bool bVisible);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "JM Interaction|UI")
    void ApplyPromptStyle(const FJMInteractionPromptStyle& NewStyle);

    UFUNCTION(BlueprintPure, Category = "JM Interaction|UI")
    FText GetDisplayedPromptText() const { return DisplayedPromptText; }

    UFUNCTION(BlueprintPure, Category = "JM Interaction|UI")
    bool IsPromptVisible() const { return bPromptVisible; }

    UFUNCTION(BlueprintPure, Category = "JM Interaction|UI")
    FJMInteractionPromptStyle GetAppliedPromptStyle() const { return AppliedStyle; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SetInteractionPrompt_Implementation(const FJMInteractionPrompt& Prompt);
    virtual void ClearInteractionPrompt_Implementation();
    virtual void SetPromptText_Implementation(const FText& NewPrompt);
    virtual void SetPromptVisible_Implementation(bool bVisible);
    virtual void ApplyPromptStyle_Implementation(const FJMInteractionPromptStyle& NewStyle);

    /**
     * Optional Designer binding. Name a TextBlock "PromptText" and enable Is Variable.
     * The base class then updates text, font size, justification, offset, and scale automatically.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Interaction|UI")
    TObjectPtr<UTextBlock> PromptText = nullptr;

    /**
     * Optional Designer binding for a background/padded container.
     * Omit it for a text-only prompt, or name a Border "PromptContainer" to style the whole prompt.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Interaction|UI")
    TObjectPtr<UBorder> PromptContainer = nullptr;

private:
    TSharedPtr<SBox> NativeOffsetBox;
    TSharedPtr<SBorder> NativePromptContainer;
    TSharedPtr<STextBlock> NativePromptText;
    FSlateFontInfo NativePromptFont;

    UPROPERTY(Transient)
    FText DisplayedPromptText;

    UPROPERTY(Transient)
    FJMInteractionPromptStyle AppliedStyle;

    UPROPERTY(Transient)
    bool bPromptVisible = false;
};
