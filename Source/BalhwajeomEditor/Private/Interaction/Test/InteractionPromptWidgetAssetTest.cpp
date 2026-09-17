#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "MultiShadowText.h"
#include "WidgetBlueprint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractionPromptWidgetAssetTest,
	"Balhwajeom.Interaction.PromptWidget.MultiShadowText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionPromptWidgetAssetTest::RunTest(const FString& Parameters)
{
	const UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/HUD/WBP_Interact.WBP_Interact"));
	if (!TestNotNull(TEXT("WBP_Interact exists"), Blueprint) ||
		!TestNotNull(TEXT("WBP_Interact has a widget tree"),
			Blueprint ? Blueprint->WidgetTree.Get() : nullptr))
	{
		return false;
	}

	const UMultiShadowTextWidget* PromptText = Cast<UMultiShadowTextWidget>(
		Blueprint->WidgetTree->FindWidget(TEXT("TextBlock_50")));
	TestNotNull(TEXT("TextBlock_50 keeps its path-facing name and uses Multi Shadow Text"),
		PromptText);
	TestNotNull(TEXT("interaction reticle remains in the widget tree"),
		Cast<UImage>(Blueprint->WidgetTree->FindWidget(TEXT("Image_108"))));

	UUserWidget* Instance = Blueprint->GeneratedClass
		? NewObject<UUserWidget>(GetTransientPackage(), Blueprint->GeneratedClass)
		: nullptr;
	if (TestNotNull(TEXT("WBP_Interact instantiates"), Instance) &&
		TestTrue(TEXT("WBP_Interact initializes"), Instance->Initialize()))
	{
		UMultiShadowTextWidget* RuntimePrompt = Cast<UMultiShadowTextWidget>(
			Instance->GetWidgetFromName(TEXT("TextBlock_50")));
		if (TestNotNull(TEXT("runtime prompt resolves by the preserved widget name"),
			RuntimePrompt))
		{
			const FText Expected = FText::FromString(TEXT("[ F ] 일기장 보기"));
			RuntimePrompt->SetText(Expected);
			TestTrue(TEXT("runtime prompt text remains writable"),
				RuntimePrompt->Text.EqualTo(Expected));
		}
	}

	return !HasAnyErrors();
}

#endif
