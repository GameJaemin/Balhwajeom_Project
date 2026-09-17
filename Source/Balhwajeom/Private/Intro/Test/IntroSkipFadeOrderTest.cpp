#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "IntroFlowActorTestTypes.h"


namespace
{
/** Spawns an intro flow actor inside a throwaway standalone world. */
ABalhwajeomIntroFlowActor* SpawnIntroFlowActor(UGameInstance& GameInstance)
{
	UWorld* World = GameInstance.GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABalhwajeomIntroFlowActor* IntroFlow =
		World->SpawnActorDeferred<ABalhwajeomIntroFlowActor>(
			ABalhwajeomIntroFlowActor::StaticClass(),
			FTransform::Identity);
	if (IntroFlow)
	{
		UGameplayStatics::FinishSpawningActor(IntroFlow, FTransform::Identity);
	}
	return IntroFlow;
}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntroSkipFadeOrderTest,
	"Balhwajeom.Intro.Skip.FadesBeforeRemovingMovie",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FIntroSkipFadeOrderTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABalhwajeomIntroFlowActor* IntroFlow = SpawnIntroFlowActor(*GameInstance);
	if (!TestNotNull(TEXT("Intro flow actor should spawn"), IntroFlow))
	{
		GameInstance->Shutdown();
		return false;
	}

	// --- Intro skip -------------------------------------------------------
	FIntroFlowActorTestAccessor::PrepareCinematic(
		IntroFlow, EBalhwajeomIntroState::Cinematic);
	FIntroFlowActorTestAccessor::RequestSkip(IntroFlow);

	TestEqual(
		TEXT("Skipping the intro should begin the fade to gameplay"),
		FIntroFlowActorTestAccessor::GetState(IntroFlow),
		EBalhwajeomIntroState::TransitionToGameplay);
	// The regression this test exists for: tearing the movie down here exposed the level for a
	// frame before the fade started, so the player saw the cut first and the fade afterwards.
	TestNotNull(
		TEXT("The movie must stay on screen while the fade to black plays"),
		FIntroFlowActorTestAccessor::GetCinematicVideoWidget(IntroFlow));
	TestTrue(
		TEXT("Movie audio should duck along with the fade instead of cutting out"),
		FIntroFlowActorTestAccessor::IsAudioFadingWithScreen(IntroFlow));

	// A second click during the fade must not start another transition.
	FIntroFlowActorTestAccessor::RequestSkip(IntroFlow);
	TestEqual(
		TEXT("Skipping again during the fade should be ignored"),
		FIntroFlowActorTestAccessor::GetState(IntroFlow),
		EBalhwajeomIntroState::TransitionToGameplay);
	TestNotNull(
		TEXT("Skipping again during the fade should not tear the movie down"),
		FIntroFlowActorTestAccessor::GetCinematicVideoWidget(IntroFlow));

	FIntroFlowActorTestAccessor::FinishFadeToBlack(IntroFlow);
	TestNull(
		TEXT("The movie should be removed once the screen is fully black"),
		FIntroFlowActorTestAccessor::GetCinematicVideoWidget(IntroFlow));
	// Otherwise the fade back in would ramp the closed movie's audio up again.
	TestFalse(
		TEXT("Audio ducking should be released before the fade from black"),
		FIntroFlowActorTestAccessor::IsAudioFadingWithScreen(IntroFlow));

	// --- Ending skip ------------------------------------------------------
	// Only the transition is driven here; completing this fade reopens the level.
	FIntroFlowActorTestAccessor::PrepareCinematic(
		IntroFlow, EBalhwajeomIntroState::Ending);
	FIntroFlowActorTestAccessor::RequestSkip(IntroFlow);

	TestEqual(
		TEXT("Skipping the ending should begin the fade back to the title"),
		FIntroFlowActorTestAccessor::GetState(IntroFlow),
		EBalhwajeomIntroState::TransitionToTitle);
	TestNotNull(
		TEXT("The ending movie must also stay on screen while the fade plays"),
		FIntroFlowActorTestAccessor::GetCinematicVideoWidget(IntroFlow));

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
