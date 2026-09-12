#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/JMInteractionComponent.h"
#include "Settings/JMInteractionSettings.h"
#include "UI/JMInteractionPromptWidgetBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FJMInteractionPromptDefaultsTest,
    "JM.Interaction.Prompt.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMInteractionPromptDefaultsTest::RunTest(const FString& Parameters)
{
    const UJMInteractionSettings* Settings = UJMInteractionSettings::Get();
    TestNotNull(TEXT("JM Interaction settings are available"), Settings);
    if (!Settings) return false;

    TestEqual(TEXT("Prompt detection runs before input by default"), Settings->DefaultTraceMode, EJMInteractionTraceMode::Timer);
    TestEqual(TEXT("Default prompt offset is centered horizontally"), Settings->DefaultPromptStyle.ScreenOffset.X, 0.0);
    TestEqual(TEXT("Default prompt is below the center dot"), Settings->DefaultPromptStyle.ScreenOffset.Y, 80.0);
    TestEqual(TEXT("Default prompt font size"), Settings->DefaultPromptStyle.FontSize, 24);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FJMInteractionPromptWidgetLifecycleTest,
    "JM.Interaction.Prompt.WidgetLifecycleAndStyle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMInteractionPromptWidgetLifecycleTest::RunTest(const FString& Parameters)
{
    UJMInteractionPromptWidgetBase* Widget = NewObject<UJMInteractionPromptWidgetBase>();
    TestNotNull(TEXT("Native prompt widget can be created"), Widget);
    if (!Widget) return false;

    TestFalse(TEXT("Prompt begins hidden"), Widget->IsPromptVisible());

    FJMInteractionPromptStyle Style;
    Style.ScreenOffset = FVector2D(15.0, 96.0);
    Style.FontSize = 31;
    Style.RenderScale = FVector2D(1.25, 1.25);
    Style.Padding = FMargin(20.0f, 10.0f);
    Widget->ApplyPromptStyle(Style);

    const FJMInteractionPromptStyle AppliedStyle = Widget->GetAppliedPromptStyle();
    TestEqual(TEXT("Screen offset is applied"), AppliedStyle.ScreenOffset, Style.ScreenOffset);
    TestEqual(TEXT("Font size is applied"), AppliedStyle.FontSize, Style.FontSize);
    TestEqual(TEXT("Render scale is applied"), AppliedStyle.RenderScale, Style.RenderScale);

    FJMInteractionPrompt Prompt;
    Prompt.DisplayText = FText::FromString(TEXT("E키를 눌러 획득"));
    Prompt.bVisible = true;
    Widget->SetInteractionPrompt(Prompt);
    TestTrue(TEXT("Detected target shows prompt before interaction"), Widget->IsPromptVisible());
    TestTrue(TEXT("Prompt text is applied"), Widget->GetDisplayedPromptText().EqualTo(Prompt.DisplayText));

    Widget->ClearInteractionPrompt();
    TestFalse(TEXT("Cleared target hides prompt"), Widget->IsPromptVisible());
    TestTrue(TEXT("Cleared target removes prompt text"), Widget->GetDisplayedPromptText().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FJMInteractionPromptSuppressionTest,
    "JM.Interaction.Prompt.Suppression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJMInteractionPromptSuppressionTest::RunTest(const FString& Parameters)
{
    UJMInteractionComponent* Component = NewObject<UJMInteractionComponent>();
    TestNotNull(TEXT("Interaction component can be created"), Component);
    if (!Component) return false;

    TestFalse(TEXT("Prompt is not suppressed by default"), Component->IsPromptSuppressed());
    Component->SetPromptSuppressed(true);
    TestTrue(TEXT("Modal UI can suppress the prompt"), Component->IsPromptSuppressed());
    Component->SetPromptSuppressed(false);
    TestFalse(TEXT("Modal UI can restore the prompt"), Component->IsPromptSuppressed());
    return true;
}

#endif
