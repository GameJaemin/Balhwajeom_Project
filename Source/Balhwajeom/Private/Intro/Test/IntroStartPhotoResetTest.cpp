#include "Intro/BalhwajeomIntroFlowActor.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Story/StoryStateSubsystem.h"
#include "UI/BalhwajeomScreenFadeWidget.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "IntroFlowActorTestTypes.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntroStartPhotoResetToggleTest,
	"Balhwajeom.Intro.StartPhotoReset.HonorsToggle",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FIntroStartPhotoResetToggleTest::RunTest(const FString& Parameters)
{
	const FString AutomationPhotoDirectory = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Investigation"),
			TEXT("Photos"),
			TEXT("Automation")));
	const FString TestPhotoPath = FPaths::Combine(
		AutomationPhotoDirectory, TEXT("IntroToggle.png"));
	IFileManager::Get().DeleteDirectory(*AutomationPhotoDirectory, false, true);
	IFileManager::Get().MakeDirectory(*AutomationPhotoDirectory, true);
	if (!TestTrue(
		TEXT("The isolated automation photo should be created"),
		FFileHelper::SaveStringToFile(TEXT("test-photo"), *TestPhotoPath)))
	{
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone GameInstance should create a World"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	ABalhwajeomIntroFlowActor* IntroFlow =
		World->SpawnActorDeferred<ABalhwajeomIntroFlowActor>(
			ABalhwajeomIntroFlowActor::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Intro flow actor should spawn"), IntroFlow))
	{
		GameInstance->Shutdown();
		return false;
	}
	UGameplayStatics::FinishSpawningActor(IntroFlow, FTransform::Identity);

	UStoryStateSubsystem* StoryState =
		GameInstance->GetSubsystem<UStoryStateSubsystem>();
	const FGameplayTag PhotographedTag = FGameplayTag::RequestGameplayTag(
		TEXT("Evidence.Photographed.OBJ_01_005"), false);
	StoryState->AddStateTag(PhotographedTag);

	FIntroFlowActorTestAccessor::PrepareTitleScreen(IntroFlow);
	FIntroFlowActorTestAccessor::SetResetPhotosOnStart(IntroFlow, false);
	FIntroFlowActorTestAccessor::RequestStart(IntroFlow);
	TestTrue(
		TEXT("Disabling the option should preserve existing photo files"),
		IFileManager::Get().FileExists(*TestPhotoPath));
	TestTrue(
		TEXT("Disabling the option should preserve photographed story tags"),
		StoryState->HasStateTagExact(PhotographedTag));
	TestEqual(
		TEXT("Starting with reset disabled should still begin the cinematic transition"),
		FIntroFlowActorTestAccessor::GetState(IntroFlow),
		EBalhwajeomIntroState::TransitionToCinematic);

	FIntroFlowActorTestAccessor::PrepareTitleScreen(IntroFlow);
	FIntroFlowActorTestAccessor::SetResetPhotosOnStart(IntroFlow, true);
	FIntroFlowActorTestAccessor::RequestStart(IntroFlow);
	TestFalse(
		TEXT("Enabling the option should delete existing photo files"),
		IFileManager::Get().FileExists(*TestPhotoPath));
	TestFalse(
		TEXT("Enabling the option should remove photographed story tags"),
		StoryState->HasStateTagExact(PhotographedTag));
	TestEqual(
		TEXT("Starting with reset enabled should continue into the cinematic transition"),
		FIntroFlowActorTestAccessor::GetState(IntroFlow),
		EBalhwajeomIntroState::TransitionToCinematic);

	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	IFileManager::Get().DeleteDirectory(*AutomationPhotoDirectory, false, true);
	return true;
}

#endif
