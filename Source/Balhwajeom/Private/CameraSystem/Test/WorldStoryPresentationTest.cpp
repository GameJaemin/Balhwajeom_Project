#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CameraSystem/BalhwajeomEvidenceActor.h"
#include "CameraSystem/PhotoWorldStoryActor.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldStoryAnchorPresentationTest,
	"Balhwajeom.Camera.WorldStory.AnchorPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldStoryAnchorPresentationTest::RunTest(const FString& Parameters)
{
	const ABalhwajeomEvidenceActor* Evidence = GetDefault<ABalhwajeomEvidenceActor>();
	const USceneComponent* StoryAnchor = nullptr;
	const UTextRenderComponent* PreviewText = nullptr;
	TInlineComponentArray<USceneComponent*> EvidenceComponents(Evidence);
	for (const USceneComponent* Component : EvidenceComponents)
	{
		if (Component && Component->GetFName() == TEXT("StoryAnchor"))
		{
			StoryAnchor = Component;
		}
		else if (Component && Component->GetFName() == TEXT("StoryAnchorPreviewText"))
		{
			PreviewText = Cast<UTextRenderComponent>(Component);
		}
	}

	TestNotNull(TEXT("Evidence exposes a StoryAnchor"), StoryAnchor);
	if (StoryAnchor)
	{
		TestTrue(TEXT("StoryAnchor rotation remains absolute in world space"),
			StoryAnchor->IsUsingAbsoluteRotation());
	}
	TestNotNull(TEXT("StoryAnchor has an editor preview caption"), PreviewText);
	if (PreviewText)
	{
		TestTrue(TEXT("StoryAnchor preview caption is hidden in game"), PreviewText->bHiddenInGame);
	}

	const APhotoWorldStoryActor* StoryActor = GetDefault<APhotoWorldStoryActor>();
	const FPhotoDefinition PhotoDefaults;
	TestTrue(TEXT("Photos default the final cue duration to 2.5 seconds"),
		FMath::IsNearlyEqual(PhotoDefaults.LastCueDurationSeconds, 2.5f));

	TInlineComponentArray<UWidgetComponent*> StoryPanels(StoryActor);
	const UWidgetComponent* FrontPanel = nullptr;
	const UWidgetComponent* BackPanel = nullptr;
	for (const UWidgetComponent* Panel : StoryPanels)
	{
		if (Panel && Panel->GetFName() == TEXT("StoryWidget"))
		{
			FrontPanel = Panel;
		}
		else if (Panel && Panel->GetFName() == TEXT("BackStoryWidget"))
		{
			BackPanel = Panel;
		}
	}

	TestNotNull(TEXT("World story has a front text panel"), FrontPanel);
	TestNotNull(TEXT("World story has an opposite-facing text panel"), BackPanel);
	const UAudioComponent* StoryAudio = StoryActor->FindComponentByClass<UAudioComponent>();
	TestNotNull(TEXT("World story has a cue sound component"), StoryAudio);
	if (StoryAudio)
	{
		TestFalse(TEXT("Cue sound completion does not control caption completion"),
			StoryAudio->OnAudioFinished.IsBound());
	}
	if (FrontPanel && BackPanel)
	{
		TestFalse(TEXT("Front text panel uses its bright front face only"), FrontPanel->GetTwoSided());
		TestFalse(TEXT("Back text panel uses its bright front face only"), BackPanel->GetTwoSided());
		TestTrue(TEXT("Back text panel faces 180 degrees away from the front"),
			FMath::IsNearlyEqual(FMath::Abs(BackPanel->GetRelativeRotation().Yaw), 180.0f));
	}

	return true;
}

#endif
