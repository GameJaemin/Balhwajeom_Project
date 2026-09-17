#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Tutorial/BalhwajeomTutorialFlow.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialHintGateConfigurationTest,
	"Balhwajeom.Tutorial.HintGateConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialHintGateConfigurationTest::RunTest(const FString& Parameters)
{
	const UBalhwajeomTutorialFlow* Flow = LoadObject<UBalhwajeomTutorialFlow>(
		nullptr, TEXT("/Game/Balhwajeom/Data/Tutorial/DA_TutorialFlow_Room2"));
	if (!TestNotNull(TEXT("DA_TutorialFlow_Room2 should load"), Flow))
	{
		return false;
	}

	// StepID -> the overlay that has to be read before the hint may appear. A step that
	// points at nothing is listed with an empty tag.
	const TMap<FName, FName> ExpectedGates = {
		{ TEXT("TabletIntro"), NAME_None },
		{ TEXT("DustTeach"), TEXT("Tutorial.Overlay.Seen.Interaction") },
		{ TEXT("PhotoPrompt"), TEXT("Tutorial.Overlay.Seen.PhotoCamera") },
		{ TEXT("Photograph"), TEXT("Tutorial.Overlay.Seen.MemoryObject") },
		{ TEXT("CompleteFamilyPhoto"), TEXT("Tutorial.Overlay.Seen.Tablet") },
		{ TEXT("Done"), NAME_None }
	};

	for (const FBalhwajeomTutorialStep& Step : Flow->Steps)
	{
		const FString StepLabel = Step.StepID.IsNone()
			? FString(TEXT("(unnamed)"))
			: Step.StepID.ToString();

		// The overlays darken the screen themselves. A second dim underneath them left
		// the room looking unlit for the whole tutorial.
		TestEqual(
			FString::Printf(TEXT("Step %s should not dim the screen"), *StepLabel),
			static_cast<int32>(Step.DimMode),
			static_cast<int32>(EBalhwajeomTutorialDimMode::Off));

		const FName* ExpectedGate = ExpectedGates.Find(Step.StepID);
		if (!ExpectedGate)
		{
			// An unnamed step is the flow's opening wait and shows nothing.
			TestEqual(
				FString::Printf(TEXT("Step %s should point at nothing"), *StepLabel),
				static_cast<int32>(Step.HintTarget),
				static_cast<int32>(EBalhwajeomTutorialHintTarget::None));
			continue;
		}

		if (ExpectedGate->IsNone())
		{
			TestEqual(
				FString::Printf(TEXT("Step %s should point at nothing"), *StepLabel),
				static_cast<int32>(Step.HintTarget),
				static_cast<int32>(EBalhwajeomTutorialHintTarget::None));
			TestTrue(
				FString::Printf(TEXT("Step %s needs no gate without a hint"), *StepLabel),
				Step.HintRequiredTags.IsEmpty());
			continue;
		}

		TestNotEqual(
			FString::Printf(TEXT("Step %s should point at something"), *StepLabel),
			static_cast<int32>(Step.HintTarget),
			static_cast<int32>(EBalhwajeomTutorialHintTarget::None));

		// Without the gate the icon starts blinking underneath the overlay that is still
		// explaining it, which is exactly the overlap this pass removed.
		const FGameplayTag GateTag = FGameplayTag::RequestGameplayTag(*ExpectedGate, false);
		TestTrue(
			FString::Printf(TEXT("Step %s should wait for %s"),
				*StepLabel, *ExpectedGate->ToString()),
			GateTag.IsValid() && Step.HintRequiredTags.HasTagExact(GateTag));
		TestEqual(
			FString::Printf(TEXT("Step %s should wait for exactly one overlay"), *StepLabel),
			Step.HintRequiredTags.Num(), 1);
	}
	return true;
}

#endif
