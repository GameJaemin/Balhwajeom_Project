#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraSystem/BalhwajeomPhotoCameraComponent.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

struct FScreenshotCaptureDiskFallbackTestAccessor
{
	static void CompleteEngineDiskCapture(
		UBalhwajeomPhotoCameraComponent* Camera,
		FBalhwajeomPendingPhotoCapture&& Pending)
	{
		Camera->PendingCapture = MoveTemp(Pending);
		Camera->bReceivedScreenshotPixels = false;
		Camera->HandleScreenshotProcessed();
	}
};

namespace ScreenshotCaptureDiskFallbackTests
{
	const FName ObjectID(TEXT("OBJ_SCREENSHOT_FALLBACK"));
	const FName StateID(TEXT("STATE_SCREENSHOT_FALLBACK"));
	const FName PhotoID(TEXT("PHOTO_SCREENSHOT_FALLBACK"));
	const FName CharacterID(TEXT("CHAR_SCREENSHOT_FALLBACK"));

	template <typename RowType>
	UDataTable* MakeTable()
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = RowType::StaticStruct();
		return Table;
	}

	struct FConfiguredTables
	{
		FConfiguredTables()
			: EvidenceDefinitions(MakeTable<FEvidenceDefinition>())
			, EvidenceStates(MakeTable<FEvidenceStateDefinition>())
			, Words(MakeTable<FWordDefinition>())
			, Photos(MakeTable<FPhotoDefinition>())
			, KeywordDocuments(MakeTable<FKeywordDocumentDefinition>())
			, KeywordChoices(MakeTable<FKeywordChoiceDefinition>())
			, Sentences(MakeTable<FSentenceDefinition>())
			, Characters(MakeTable<FCharacterDefinition>())
		{
			FCharacterDefinition Character;
			Character.CharacterID = CharacterID;
			Character.FolderName = FText::FromString(TEXT("Screenshot fallback test"));
			Characters->AddRow(CharacterID, Character);

			FEvidenceDefinition Evidence;
			Evidence.ObjectID = ObjectID;
			Evidence.InitialStateID = StateID;
			EvidenceDefinitions->AddRow(ObjectID, Evidence);

			FEvidenceStateDefinition State;
			State.StateID = StateID;
			State.ObjectID = ObjectID;
			State.InteractionBehavior = EEvidenceInteractionBehavior::None;
			State.InteractionPresentation = EEvidenceInteractionPresentation::None;
			State.bCanCapture = true;
			State.PhotoID = PhotoID;
			EvidenceStates->AddRow(StateID, State);

			FPhotoDefinition Photo;
			Photo.PhotoID = PhotoID;
			Photo.CharacterID = CharacterID;
			Photos->AddRow(PhotoID, Photo);
		}

		UDataTable* EvidenceDefinitions;
		UDataTable* EvidenceStates;
		UDataTable* Words;
		UDataTable* Photos;
		UDataTable* KeywordDocuments;
		UDataTable* KeywordChoices;
		UDataTable* Sentences;
		UDataTable* Characters;
	};

	struct FSettingsOverride
	{
		explicit FSettingsOverride(const FConfiguredTables& Tables)
			: Settings(GetMutableDefault<UBalhwajeomInvestigationSettings>())
			, OriginalEvidenceDefinitions(Settings->EvidenceDefinitionsTable)
			, OriginalEvidenceStates(Settings->EvidenceStatesTable)
			, OriginalWords(Settings->WordsTable)
			, OriginalPhotos(Settings->PhotosTable)
			, OriginalKeywordDocuments(Settings->KeywordDocumentsTable)
			, OriginalKeywordChoices(Settings->KeywordChoicesTable)
			, OriginalSentences(Settings->SentencesTable)
			, OriginalCharacters(Settings->CharactersTable)
		{
			Settings->EvidenceDefinitionsTable = Tables.EvidenceDefinitions;
			Settings->EvidenceStatesTable = Tables.EvidenceStates;
			Settings->WordsTable = Tables.Words;
			Settings->PhotosTable = Tables.Photos;
			Settings->KeywordDocumentsTable = Tables.KeywordDocuments;
			Settings->KeywordChoicesTable = Tables.KeywordChoices;
			Settings->SentencesTable = Tables.Sentences;
			Settings->CharactersTable = Tables.Characters;
		}

		~FSettingsOverride()
		{
			Settings->EvidenceDefinitionsTable = OriginalEvidenceDefinitions;
			Settings->EvidenceStatesTable = OriginalEvidenceStates;
			Settings->WordsTable = OriginalWords;
			Settings->PhotosTable = OriginalPhotos;
			Settings->KeywordDocumentsTable = OriginalKeywordDocuments;
			Settings->KeywordChoicesTable = OriginalKeywordChoices;
			Settings->SentencesTable = OriginalSentences;
			Settings->CharactersTable = OriginalCharacters;
		}

		UBalhwajeomInvestigationSettings* Settings;
		TSoftObjectPtr<UDataTable> OriginalEvidenceDefinitions;
		TSoftObjectPtr<UDataTable> OriginalEvidenceStates;
		TSoftObjectPtr<UDataTable> OriginalWords;
		TSoftObjectPtr<UDataTable> OriginalPhotos;
		TSoftObjectPtr<UDataTable> OriginalKeywordDocuments;
		TSoftObjectPtr<UDataTable> OriginalKeywordChoices;
		TSoftObjectPtr<UDataTable> OriginalSentences;
		TSoftObjectPtr<UDataTable> OriginalCharacters;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScreenshotCaptureDiskFallbackTest,
	"Balhwajeom.Camera.ScreenshotCaptureDiskFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScreenshotCaptureDiskFallbackTest::RunTest(const FString& Parameters)
{
	using namespace ScreenshotCaptureDiskFallbackTests;

	const FConfiguredTables Tables;
	const FSettingsOverride SettingsOverride(Tables);
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	UBalhwajeomInvestigationSubsystem* Investigation = GameInstance->GetSubsystem<
		UBalhwajeomInvestigationSubsystem>();

	if (!TestNotNull(TEXT("Standalone world exists"), World) ||
		!TestNotNull(TEXT("Investigation subsystem exists"), Investigation))
	{
		GameInstance->Shutdown();
		return false;
	}

	Investigation->ResetPersistentPhotoGallery();
	FGuid EvidenceInstanceID = FGuid::NewGuid();
	FName CurrentStateID;
	if (!TestTrue(
		TEXT("Test evidence registers"),
		Investigation->RegisterEvidenceActor(EvidenceInstanceID, ObjectID, CurrentStateID)))
	{
		GameInstance->Shutdown();
		return false;
	}

	const FString RelativePath = FPaths::Combine(
		TEXT("Investigation"), TEXT("Photos"), TEXT("Automation"),
		TEXT("ScreenshotCaptureDiskFallback.png"));
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectSavedDir(), RelativePath));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsolutePath), true);

	TArray<FColor> Pixels;
	Pixels.Init(FColor(32, 96, 192, 255), 4);
	const FImageView Image(Pixels.GetData(), 2, 2);
	if (!TestTrue(
		TEXT("Engine disk path contains a valid PNG before completion"),
		FImageUtils::SaveImageByExtension(*AbsolutePath, Image)))
	{
		GameInstance->Shutdown();
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	UBalhwajeomPhotoCameraComponent* Camera =
		NewObject<UBalhwajeomPhotoCameraComponent>(Owner);
	Owner->AddInstanceComponent(Camera);

	FBalhwajeomPendingPhotoCapture Pending;
	Pending.RequestID = FGuid::NewGuid();
	Pending.TargetSnapshot.EvidenceInstanceID = EvidenceInstanceID;
	Pending.TargetSnapshot.ObjectID = ObjectID;
	Pending.TargetSnapshot.StateID = StateID;
	Pending.TargetSnapshot.PhotoID = PhotoID;
	Pending.TargetSnapshot.bCanCapture = true;
	Pending.RequestedTime = FDateTime::UtcNow();
	Pending.RelativePath = RelativePath;
	Pending.AbsolutePath = AbsolutePath;
	FScreenshotCaptureDiskFallbackTestAccessor::CompleteEngineDiskCapture(
		Camera, MoveTemp(Pending));

	TestTrue(
		TEXT("A valid engine-saved screenshot registers without a memory-pixel callback"),
		Investigation->HasCapturedPhoto(PhotoID));
	TestTrue(
		TEXT("The registered engine-saved PNG remains on disk"),
		IFileManager::Get().FileSize(*AbsolutePath) > 0);

	Investigation->ResetPersistentPhotoGallery();
	GameInstance->Shutdown();
	return true;
}

#endif
