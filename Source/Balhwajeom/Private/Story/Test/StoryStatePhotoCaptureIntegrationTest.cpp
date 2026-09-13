#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/BalhwajeomInvestigationSaveGame.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Story/StoryStateSubsystem.h"


#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"


namespace StoryStatePhotoCaptureIntegrationTests
{
	const FString AutomationSaveSlot = TEXT("BalhwajeomInvestigation_Automation");

	struct FFixture
	{
		FFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
		}

		~FFixture()
		{
			if (!GameInstance)
			{
				return;
			}

			GameInstance->Shutdown();
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
	};

	struct FPersistentPhotoFixture
	{
		FPersistentPhotoFixture()
		{
			UGameplayStatics::DeleteGameInSlot(AutomationSaveSlot, 0);
			AbsoluteImagePath = FPaths::Combine(
				FPaths::ProjectSavedDir(),
				RelativeImagePath
			);
			IFileManager::Get().MakeDirectory(
				*FPaths::GetPath(AbsoluteImagePath),
				true
			);
		}

		~FPersistentPhotoFixture()
		{
			if (GameInstance)
			{
				GameInstance->Shutdown();
				if (World)
				{
					GEngine->DestroyWorldContext(World);
					World->DestroyWorld(false);
				}
			}

			UGameplayStatics::DeleteGameInSlot(AutomationSaveSlot, 0);
			IFileManager::Get().Delete(*AbsoluteImagePath, false, true);
		}

		bool SaveCapturedWardrobePhoto()
		{
			if (!FFileHelper::SaveStringToFile(
				TEXT("automation photo"),
				*AbsoluteImagePath))
			{
				return false;
			}

			UBalhwajeomInvestigationSaveGame* SaveGame =
				Cast<UBalhwajeomInvestigationSaveGame>(
					UGameplayStatics::CreateSaveGameObject(
						UBalhwajeomInvestigationSaveGame::StaticClass()
					)
				);
			if (!SaveGame)
			{
				return false;
			}

			FCapturedPhotoRecord PhotoRecord;
			PhotoRecord.PhotoID = TEXT("PHOTO_01_019");
			PhotoRecord.ObjectID = TEXT("OBJ_01_019");
			PhotoRecord.EvidenceInstanceID = FGuid::NewGuid();
			PhotoRecord.CapturedStateID = TEXT("STATE_01_019_NORMAL");
			PhotoRecord.ImageRelativePath = RelativeImagePath;
			PhotoRecord.CapturedTime = FDateTime::UtcNow();
			SaveGame->CapturedPhotos.Add(PhotoRecord);

			return UGameplayStatics::SaveGameToSlot(
				SaveGame,
				AutomationSaveSlot,
				0
			);
		}

		void InitializeGameInstance()
		{
			GameInstance = NewObject<UGameInstance>(GEngine);
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
		}

		const FString RelativeImagePath =
			TEXT("Investigation/Photos/StoryStateRestoreAutomation.photo");
		FString AbsoluteImagePath;
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
	};
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateTracksConfiguredPhotographedEvidenceTest,
	"Balhwajeom.StoryState.Evidence.PhotoCaptureTracksConfiguredObject",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateTracksConfiguredPhotographedEvidenceTest::RunTest(
	const FString& Parameters
)
{
	const StoryStatePhotoCaptureIntegrationTests::FFixture Fixture;
	UStoryStateSubsystem* StoryState =
		Fixture.GameInstance->GetSubsystem<UStoryStateSubsystem>();
	UBalhwajeomInvestigationSubsystem* Investigation =
		Fixture.GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>();

	if (!TestNotNull(TEXT("StoryState subsystem should exist"), StoryState) ||
		!TestNotNull(TEXT("Investigation subsystem should exist"), Investigation))
	{
		return false;
	}

	const FGameplayTag WardrobePhotographedTag =
		FGameplayTag::RequestGameplayTag(
			TEXT("Evidence.Photographed.OBJ_01_019"),
			false
		);
	if (!TestTrue(
		TEXT("The configured wardrobe photographed tag should be registered"),
		WardrobePhotographedTag.IsValid()))
	{
		return false;
	}

	StoryState->ClearStateTags();

	FCapturedPhotoRecord PhotoRecord;
	PhotoRecord.ObjectID = TEXT("OBJ_01_019");
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		Investigation->OnPhotoCaptured.Broadcast(PhotoRecord);
		Investigation->OnPhotoCaptured.Broadcast(PhotoRecord);
	}

	TestTrue(
		TEXT("Capturing a configured object should add its photographed state tag"),
		StoryState->HasStateTagExact(WardrobePhotographedTag)
	);
	TestEqual(
		TEXT("Repeated capture events should keep one exact state tag"),
		StoryState->GetCurrentStateTags().Num(),
		1
	);

	StoryState->ClearStateTags();
	PhotoRecord.ObjectID = TEXT("OBJ_01_006");
	{
		FEditorScriptExecutionGuard ScriptExecutionGuard;
		Investigation->OnPhotoCaptured.Broadcast(PhotoRecord);
	}

	TestTrue(
		TEXT("An object without a configured photographed tag should be ignored"),
		StoryState->GetCurrentStateTags().IsEmpty()
	);

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStoryStateRestoresPhotographedEvidenceTagsTest,
	"Balhwajeom.StoryState.Evidence.RestoresPhotographedTags",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)


bool FStoryStateRestoresPhotographedEvidenceTagsTest::RunTest(
	const FString& Parameters
)
{
	StoryStatePhotoCaptureIntegrationTests::FPersistentPhotoFixture Fixture;
	if (!TestTrue(
		TEXT("The automation photo record should be persisted"),
		Fixture.SaveCapturedWardrobePhoto()))
	{
		return false;
	}

	Fixture.InitializeGameInstance();
	UStoryStateSubsystem* StoryState =
		Fixture.GameInstance->GetSubsystem<UStoryStateSubsystem>();
	if (!TestNotNull(TEXT("StoryState subsystem should exist"), StoryState))
	{
		return false;
	}

	const FGameplayTag WardrobePhotographedTag =
		FGameplayTag::RequestGameplayTag(
			TEXT("Evidence.Photographed.OBJ_01_019"),
			false
		);
	TestTrue(
		TEXT("A restored photo should rebuild its photographed state tag"),
		StoryState->HasStateTagExact(WardrobePhotographedTag)
	);

	return true;
}

#endif
