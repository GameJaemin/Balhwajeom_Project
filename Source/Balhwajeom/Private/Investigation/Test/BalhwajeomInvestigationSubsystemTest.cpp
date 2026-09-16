#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/BalhwajeomInvestigationSaveGame.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Story/StoryStateTags.h"
#include "UObject/UnrealType.h"


#if WITH_DEV_AUTOMATION_TESTS

struct FInvestigationSubsystemTestAccessor
{
	static void SetTables(
		UBalhwajeomInvestigationSubsystem* Subsystem,
		UDataTable* EvidenceDefinitions,
		UDataTable* EvidenceStates,
		UDataTable* Words,
		UDataTable* Photos,
		UDataTable* KeywordDocuments,
		UDataTable* KeywordChoices,
		UDataTable* Sentences,
		UDataTable* Characters)
	{
		Subsystem->EvidenceDefinitionsTable = EvidenceDefinitions;
		Subsystem->EvidenceStatesTable = EvidenceStates;
		Subsystem->WordsTable = Words;
		Subsystem->PhotosTable = Photos;
		Subsystem->KeywordDocumentsTable = KeywordDocuments;
		Subsystem->KeywordChoicesTable = KeywordChoices;
		Subsystem->SentencesTable = Sentences;
		Subsystem->CharactersTable = Characters;
	}

	static bool Validate(const UBalhwajeomInvestigationSubsystem* Subsystem)
	{
		return Subsystem->ValidateLoadedDataTables();
	}

	static void SeedPersistentPhotoState(
		UBalhwajeomInvestigationSubsystem* Subsystem,
		const FCapturedPhotoRecord& PhotoRecord,
		FName WordID)
	{
		Subsystem->CapturedPhotos.Add(PhotoRecord.PhotoID, PhotoRecord);

		FAcquiredWordRecord WordRecord;
		WordRecord.WordID = WordID;
		WordRecord.SourceType = EWordAcquisitionSource::PhotoCapture;
		WordRecord.SourceID = PhotoRecord.PhotoID;
		Subsystem->AcquiredWords.Add(WordID, WordRecord);
	}
};

namespace InvestigationSubsystemTests
{
const FName ObjectID(TEXT("OBJ_TEST"));
const FName StateID(TEXT("STATE_TEST_ONCE"));
const FName WordID(TEXT("WORD_TEST"));
const FName AlternateWordID(TEXT("WORD_TEST_ALTERNATE"));
const FName PhotoID(TEXT("PHOTO_TEST"));
const FName SentenceID(TEXT("SENT_PHOTO_TEST"));
const FName StatementID(TEXT("SENT_STATEMENT_TEST"));
const FName CharacterID(TEXT("CHAR_TEST"));

template <typename RowType>
UDataTable* MakeTable()
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = RowType::StaticStruct();
	return Table;
}

struct FFixture
{
	FFixture()
		: GameInstance(NewObject<UGameInstance>())
		, Subsystem(NewObject<UBalhwajeomInvestigationSubsystem>(GameInstance))
		, EvidenceDefinitions(MakeTable<FEvidenceDefinition>())
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
		Character.FolderName = FText::FromString(TEXT("테스트 인물"));
		Characters->AddRow(CharacterID, Character);

		FEvidenceDefinition Evidence;
		Evidence.ObjectID = ObjectID;
		Evidence.InitialStateID = StateID;
		EvidenceDefinitions->AddRow(ObjectID, Evidence);

		FEvidenceStateDefinition State;
		State.StateID = StateID;
		State.ObjectID = ObjectID;
		State.InteractionBehavior = EEvidenceInteractionBehavior::Once;
		State.InteractionPresentation = EEvidenceInteractionPresentation::SimpleText;
		State.GrantedWordIDs.Add(WordID);
		State.GrantedWordIDs.Add(AlternateWordID);
		State.bCanCapture = true;
		State.PhotoID = PhotoID;
		EvidenceStates->AddRow(StateID, State);

		FWordDefinition Word;
		Word.WordID = WordID;
		Word.RelatedCharacterIDs.Add(CharacterID);
		Words->AddRow(WordID, Word);
		FWordDefinition AlternateWord;
		AlternateWord.WordID = AlternateWordID;
		AlternateWord.RelatedCharacterIDs.Add(CharacterID);
		Words->AddRow(AlternateWordID, AlternateWord);

		FKeywordDocumentDefinition Document;
		Document.KeywordDocumentID = TEXT("DOC_TEST");
		Document.DocumentText = FText::FromString(TEXT("Test document"));
		KeywordDocuments->AddRow(Document.KeywordDocumentID, Document);

		FKeywordChoiceDefinition Choice;
		Choice.ChoiceID = TEXT("CHOICE_TEST");
		Choice.KeywordDocumentID = Document.KeywordDocumentID;
		Choice.DisplayText = FText::FromString(TEXT("Test choice"));
		Choice.GrantedWordID = WordID;
		KeywordChoices->AddRow(Choice.ChoiceID, Choice);

		FPhotoDefinition Photo;
		Photo.PhotoID = PhotoID;
		Photo.PhotoSentenceID = SentenceID;
		Photo.CharacterID = CharacterID;
		Photo.GrantedWordIDs.Add(WordID);
		Photos->AddRow(PhotoID, Photo);

		FSentenceDefinition PhotoSentence;
		PhotoSentence.SentenceID = SentenceID;
		PhotoSentence.SentenceType = ESentenceType::PhotoAnalysis;
		PhotoSentence.ResultText = FText::FromString(TEXT("Photo analysis result"));

		FSentenceWordSlot WordSlot;
		WordSlot.SlotIndex = 0;
		WordSlot.CorrectWordID = WordID;
		PhotoSentence.WordSlots.Add(WordSlot);
		Sentences->AddRow(SentenceID, PhotoSentence);

		FSentenceDefinition Statement;
		Statement.SentenceID = StatementID;
		Statement.SentenceType = ESentenceType::Statement;
		Statement.CharacterID = CharacterID;
		Statement.ResultText = FText::FromString(TEXT("Test result text"));
		Statement.RequiredPhotoCount = 1;

		FSentencePhotoSlot PhotoSlot;
		PhotoSlot.SlotIndex = 0;
		PhotoSlot.CorrectPhotoID = PhotoID;
		Statement.PhotoSlots.Add(PhotoSlot);
		Sentences->AddRow(StatementID, Statement);

		FInvestigationSubsystemTestAccessor::SetTables(
			Subsystem,
			EvidenceDefinitions,
			EvidenceStates,
			Words,
			Photos,
			KeywordDocuments,
			KeywordChoices,
			Sentences,
			Characters);
	}

	FGuid RegisterTestEvidence() const
	{
		const FGuid InstanceID = FGuid::NewGuid();
		FName CurrentStateID;
		const bool bRegistered =
			Subsystem->RegisterEvidenceActor(InstanceID, ObjectID, CurrentStateID);
		check(bRegistered && CurrentStateID == StateID);
		return InstanceID;
	}

	FCapturedPhotoRecord MakePhotoRecord(FGuid InstanceID) const
	{
		FCapturedPhotoRecord Record;
		Record.PhotoID = PhotoID;
		Record.ObjectID = ObjectID;
		Record.EvidenceInstanceID = InstanceID;
		Record.CapturedStateID = StateID;
		Record.ImageRelativePath = TEXT("InvestigationTests/Photo.png");
		return Record;
	}

	UGameInstance* GameInstance;
	UBalhwajeomInvestigationSubsystem* Subsystem;
	UDataTable* EvidenceDefinitions;
	UDataTable* EvidenceStates;
	UDataTable* Words;
	UDataTable* Photos;
	UDataTable* KeywordDocuments;
	UDataTable* KeywordChoices;
	UDataTable* Sentences;
	UDataTable* Characters;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationSettingsConfigurationTest,
	"Balhwajeom.Investigation.SettingsConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationSettingsConfigurationTest::RunTest(const FString& Parameters)
{
	const UBalhwajeomInvestigationSettings* Settings =
		GetDefault<UBalhwajeomInvestigationSettings>();
	TestNotNull(TEXT("Investigation settings should exist"), Settings);
	if (Settings == nullptr)
	{
		return false;
	}

	auto TestConfiguredTable = [this](
		const TCHAR* TableName,
		const TSoftObjectPtr<UDataTable>& TableReference,
		const UScriptStruct* ExpectedRowStruct)
	{
		UDataTable* Table = TableReference.LoadSynchronous();
		TestNotNull(TableName, Table);
		if (Table != nullptr)
		{
			TestEqual(
				FString::Printf(TEXT("%s should use the expected row structure"), TableName),
				Table->GetRowStruct(),
				ExpectedRowStruct);
		}
	};

	TestConfiguredTable(
		TEXT("EvidenceDefinitionsTable"),
		Settings->EvidenceDefinitionsTable,
		FEvidenceDefinition::StaticStruct());
	TestConfiguredTable(
		TEXT("EvidenceStatesTable"),
		Settings->EvidenceStatesTable,
		FEvidenceStateDefinition::StaticStruct());
	TestConfiguredTable(TEXT("WordsTable"), Settings->WordsTable, FWordDefinition::StaticStruct());
	TestConfiguredTable(TEXT("PhotosTable"), Settings->PhotosTable, FPhotoDefinition::StaticStruct());
	TestConfiguredTable(
		TEXT("KeywordDocumentsTable"),
		Settings->KeywordDocumentsTable,
		FKeywordDocumentDefinition::StaticStruct());
	TestConfiguredTable(
		TEXT("KeywordChoicesTable"),
		Settings->KeywordChoicesTable,
		FKeywordChoiceDefinition::StaticStruct());
	TestConfiguredTable(
		TEXT("SentencesTable"),
		Settings->SentencesTable,
		FSentenceDefinition::StaticStruct());
	TestConfiguredTable(
		TEXT("CharactersTable"),
		Settings->CharactersTable,
		FCharacterDefinition::StaticStruct());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationDefinitionLookupTest,
	"Balhwajeom.Investigation.DefinitionLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationDefinitionLookupTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	FEvidenceDefinition Evidence;
	FEvidenceStateDefinition State;
	FPhotoDefinition PhotoDefinition;
	FCharacterDefinition CharacterDefinition;

	TestTrue(
		TEXT("A known ObjectID should resolve"),
		Fixture.Subsystem->GetEvidenceDefinition(InvestigationSubsystemTests::ObjectID, Evidence));
	TestEqual(TEXT("Resolved ObjectID should match"), Evidence.ObjectID, InvestigationSubsystemTests::ObjectID);
	TestTrue(
		TEXT("A known StateID should resolve"),
		Fixture.Subsystem->GetEvidenceStateDefinition(InvestigationSubsystemTests::StateID, State));
	TestFalse(
		TEXT("An unknown ObjectID should fail safely"),
		Fixture.Subsystem->GetEvidenceDefinition(FName(TEXT("OBJ_MISSING")), Evidence));
	TestTrue(
		TEXT("Existing PhotoID should resolve"),
		Fixture.Subsystem->GetPhotoDefinition(
			InvestigationSubsystemTests::PhotoID,
			PhotoDefinition));
	TestEqual(
		TEXT("Resolved PhotoID should match"),
		PhotoDefinition.PhotoID,
		InvestigationSubsystemTests::PhotoID);
	TestTrue(
		TEXT("A known CharacterID should resolve"),
		Fixture.Subsystem->GetCharacterDefinition(
			InvestigationSubsystemTests::CharacterID,
			CharacterDefinition));
	TestEqual(
		TEXT("Resolved CharacterID should match"),
		CharacterDefinition.CharacterID,
		InvestigationSubsystemTests::CharacterID);
	TestFalse(
		TEXT("Unknown PhotoID should fail"),
		Fixture.Subsystem->GetPhotoDefinition(
			FName(TEXT("PHOTO_Missing")),
			PhotoDefinition));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationPhotoSentenceLookupRejectsStatementTest,
	"Balhwajeom.Investigation.PhotoSentenceLookup.RejectsStatement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationPhotoSentenceLookupRejectsStatementTest::RunTest(
	const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	FPhotoDefinition Photo;
	Fixture.Subsystem->GetPhotoDefinition(
		InvestigationSubsystemTests::PhotoID,
		Photo);
	Photo.PhotoSentenceID = InvestigationSubsystemTests::StatementID;
	Fixture.Photos->RemoveRow(InvestigationSubsystemTests::PhotoID);
	Fixture.Photos->AddRow(InvestigationSubsystemTests::PhotoID, Photo);

	FPhotoDefinition ResolvedPhoto;
	TestFalse(
		TEXT("A Statement sentence must not resolve as a photo-analysis sentence"),
		Fixture.Subsystem->GetPhotoDefinitionBySentenceID(
			InvestigationSubsystemTests::StatementID,
			ResolvedPhoto));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationPhotoSentenceLookupRejectsDuplicateTest,
	"Balhwajeom.Investigation.PhotoSentenceLookup.RejectsDuplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationPhotoSentenceLookupRejectsDuplicateTest::RunTest(
	const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	FPhotoDefinition DuplicatePhoto;
	Fixture.Subsystem->GetPhotoDefinition(
		InvestigationSubsystemTests::PhotoID,
		DuplicatePhoto);
	DuplicatePhoto.PhotoID = TEXT("PHOTO_TEST_DUPLICATE");
	Fixture.Photos->AddRow(DuplicatePhoto.PhotoID, DuplicatePhoto);

	FPhotoDefinition ResolvedPhoto;
	TestFalse(
		TEXT("A duplicated PhotoSentenceID must not resolve nondeterministically"),
		Fixture.Subsystem->GetPhotoDefinitionBySentenceID(
			InvestigationSubsystemTests::SentenceID,
			ResolvedPhoto));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationConfiguredDataValidationTest,
	"Balhwajeom.Investigation.ConfiguredDataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationConfiguredDataValidationTest::RunTest(const FString& Parameters)
{
	const UBalhwajeomInvestigationSettings* Settings = GetDefault<UBalhwajeomInvestigationSettings>();
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UBalhwajeomInvestigationSubsystem* Subsystem =
		NewObject<UBalhwajeomInvestigationSubsystem>(GameInstance);
	FInvestigationSubsystemTestAccessor::SetTables(
		Subsystem,
		Settings->EvidenceDefinitionsTable.LoadSynchronous(),
		Settings->EvidenceStatesTable.LoadSynchronous(),
		Settings->WordsTable.LoadSynchronous(),
		Settings->PhotosTable.LoadSynchronous(),
		Settings->KeywordDocumentsTable.LoadSynchronous(),
		Settings->KeywordChoicesTable.LoadSynchronous(),
		Settings->SentencesTable.LoadSynchronous(),
		Settings->CharactersTable.LoadSynchronous());
	TestTrue(TEXT("Configured prototype DataTables should pass all cross-reference checks"),
		FInvestigationSubsystemTestAccessor::Validate(Subsystem));
	FPhotoDefinition StoryPhoto;
	TestTrue(TEXT("Configured pillow photo should resolve"),
		Subsystem->GetPhotoDefinition(FName(TEXT("PHOTO_01_013")), StoryPhoto));
	TestEqual(TEXT("Configured pillow photo should contain two timed story cues"),
		StoryPhoto.WorldStoryCues.Num(), 2);
	if (StoryPhoto.WorldStoryCues.Num() == 2)
	{
		TestTrue(TEXT("First story cue should start at zero"),
			FMath::IsNearlyZero(StoryPhoto.WorldStoryCues[0].StartTimeSeconds));
		TestTrue(TEXT("Second story cue should follow the first"),
			StoryPhoto.WorldStoryCues[1].StartTimeSeconds >
			StoryPhoto.WorldStoryCues[0].StartTimeSeconds);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationChapter01PhaseActivationDataTest,
	"Balhwajeom.Investigation.Progression.Chapter01ActivationData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationChapter01PhaseActivationDataTest::RunTest(
	const FString& Parameters)
{
	const UBalhwajeomInvestigationSettings* Settings =
		GetDefault<UBalhwajeomInvestigationSettings>();
	if (!TestNotNull(TEXT("Investigation settings should exist"), Settings))
	{
		return false;
	}

	const UClass* SettingsClass = Settings->GetClass();
	if (!TestNotNull(
		TEXT("Phase system toggle should be editable in Investigation settings"),
		SettingsClass->FindPropertyByName(TEXT("bEnableChapter01PhaseSystem"))) ||
		!TestNotNull(
			TEXT("Phase 01 ObjectID list should be editable in Investigation settings"),
			SettingsClass->FindPropertyByName(TEXT("Phase01ObjectIDs"))) ||
		!TestNotNull(
			TEXT("Phase 02 ObjectID list should be editable in Investigation settings"),
			SettingsClass->FindPropertyByName(TEXT("Phase02ObjectIDs"))) ||
		!TestNotNull(
			TEXT("Phase 03 ObjectID list should be editable in Investigation settings"),
			SettingsClass->FindPropertyByName(TEXT("Phase03ObjectIDs"))))
	{
		return false;
	}

	const TCHAR* SettingsSection =
		TEXT("/Script/Balhwajeom.BalhwajeomInvestigationSettings");
	bool bPhaseSystemEnabled = false;
	TestTrue(
		TEXT("Chapter 01 phase system toggle should be configured"),
		GConfig->GetBool(
			SettingsSection,
			TEXT("bEnableChapter01PhaseSystem"),
			bPhaseSystemEnabled,
			GGameIni));
	TestTrue(
		TEXT("Chapter 01 phase system should be enabled by default"),
		bPhaseSystemEnabled);

	auto ReadConfiguredObjectIDs = [SettingsSection](const TCHAR* PropertyName)
	{
		TArray<FString> Values;
		GConfig->GetArray(SettingsSection, PropertyName, Values, GGameIni);

		TArray<FName> ObjectIDs;
		ObjectIDs.Reserve(Values.Num());
		for (const FString& Value : Values)
		{
			ObjectIDs.Add(FName(Value));
		}
		return ObjectIDs;
	};

	const TArray<FName> Phase01Objects = ReadConfiguredObjectIDs(
		TEXT("Phase01ObjectIDs"));
	const TArray<FName> Phase02Objects = ReadConfiguredObjectIDs(
		TEXT("Phase02ObjectIDs"));
	const TArray<FName> Phase03Objects = ReadConfiguredObjectIDs(
		TEXT("Phase03ObjectIDs"));

	const TArray<FName> ExpectedPhase01Objects = {
		TEXT("OBJ_01_018"), TEXT("OBJ_01_020"),
		TEXT("OBJ_01_005"), TEXT("OBJ_01_019")
	};
	const TArray<FName> ExpectedPhase02Objects = {
		TEXT("OBJ_01_004"), TEXT("OBJ_01_022"), TEXT("OBJ_01_021"),
		TEXT("OBJ_01_025"), TEXT("OBJ_01_024"), TEXT("OBJ_01_023"),
		TEXT("OBJ_01_015")
	};
	const TArray<FName> ExpectedPhase03Objects = {
		TEXT("OBJ_01_016"), TEXT("OBJ_01_017"), TEXT("OBJ_01_010")
	};

	TestTrue(TEXT("Phase 01 ObjectIDs should match the authored list"),
		Phase01Objects == ExpectedPhase01Objects);
	TestTrue(TEXT("Phase 02 ObjectIDs should match the authored list"),
		Phase02Objects == ExpectedPhase02Objects);
	TestTrue(TEXT("Phase 03 ObjectIDs should match the authored list"),
		Phase03Objects == ExpectedPhase03Objects);

	UDataTable* Definitions = Settings
		? Settings->EvidenceDefinitionsTable.LoadSynchronous()
		: nullptr;
	if (!TestNotNull(TEXT("Configured evidence definitions should load"), Definitions))
	{
		return false;
	}

	TSet<FName> UniqueConfiguredObjectIDs;
	for (const TArray<FName>* PhaseObjects : {
		&Phase01Objects, &Phase02Objects, &Phase03Objects })
	{
		for (const FName ObjectID : *PhaseObjects)
		{
			TestNotNull(
				FString::Printf(TEXT("%s definition should exist"), *ObjectID.ToString()),
				Definitions->FindRow<FEvidenceDefinition>(
					ObjectID, TEXT("PhaseActivationTest")));
			TestFalse(
				FString::Printf(TEXT("%s should appear in only one phase"), *ObjectID.ToString()),
				UniqueConfiguredObjectIDs.Contains(ObjectID));
			UniqueConfiguredObjectIDs.Add(ObjectID);
		}
	}

	const FEvidenceDefinition* Phase01Obstacle =
		Definitions->FindRow<FEvidenceDefinition>(
			TEXT("Obstacle_Phase01"), TEXT("PhaseActivationTest"));
	if (TestNotNull(TEXT("Phase 01 obstacle definition should exist"), Phase01Obstacle))
	{
		TestTrue(
			TEXT("Phase 01 obstacle should require phase 01 completion before clearing"),
			Phase01Obstacle->ClearRequiredTag ==
				BalhwajeomGameplayTags::Story_Chapter_01_Phase_01_Completed);
		TestTrue(
			TEXT("Phase 01 obstacle should unlock phase 02 after clearing"),
			Phase01Obstacle->GrantedTagOnClear ==
				BalhwajeomGameplayTags::Story_Chapter_01_Phase_02_Unlocked);
	}

	const FEvidenceDefinition* Phase02Obstacle =
		Definitions->FindRow<FEvidenceDefinition>(
			TEXT("Obstacle_Phase02"), TEXT("PhaseActivationTest"));
	if (TestNotNull(TEXT("Phase 02 obstacle definition should exist"), Phase02Obstacle))
	{
		TestTrue(
			TEXT("Phase 02 obstacle should require phase 02 completion before clearing"),
			Phase02Obstacle->ClearRequiredTag ==
				BalhwajeomGameplayTags::Story_Chapter_01_Phase_02_Completed);
		TestTrue(
			TEXT("Phase 02 obstacle should unlock phase 03 after clearing"),
			Phase02Obstacle->GrantedTagOnClear ==
				BalhwajeomGameplayTags::Story_Chapter_01_Phase_03_Unlocked);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationChapter01PhaseActivationResolutionTest,
	"Balhwajeom.Investigation.Progression.Chapter01ActivationResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationChapter01PhaseActivationResolutionTest::RunTest(
	const FString& Parameters)
{
	const UBalhwajeomInvestigationSettings* Settings =
		GetDefault<UBalhwajeomInvestigationSettings>();
	if (!TestNotNull(TEXT("Investigation settings should exist"), Settings))
	{
		return false;
	}

	UFunction* ResolveFunction = Settings->FindFunction(
		TEXT("ResolveChapter01RequiredActivationTag"));
	if (!TestNotNull(
		TEXT("Investigation settings should expose the phase activation resolver"),
		ResolveFunction))
	{
		return false;
	}

	struct FResolveActivationTagParameters
	{
		FName ObjectID;
		FGameplayTag AuthoredActivationTag;
		FGameplayTag ReturnValue;
	};

	auto Resolve = [ResolveFunction](
		UBalhwajeomInvestigationSettings* TargetSettings,
		const FName ObjectID,
		const FGameplayTag AuthoredActivationTag)
	{
		FResolveActivationTagParameters Parameters{
			ObjectID,
			AuthoredActivationTag,
			FGameplayTag()
		};
		TargetSettings->ProcessEvent(ResolveFunction, &Parameters);
		return Parameters.ReturnValue;
	};

	const FGameplayTag TutorialDoneTag = FGameplayTag::RequestGameplayTag(
		TEXT("Tutorial.Stage.Done"), false);
	const FGameplayTag AuthoredFallbackTag = FGameplayTag::RequestGameplayTag(
		TEXT("Runtime.Lock.PhotoCamera"), false);
	if (!TestTrue(TEXT("Tutorial completion tag should be registered"),
		TutorialDoneTag.IsValid()) ||
		!TestTrue(TEXT("Fallback test tag should be registered"),
			AuthoredFallbackTag.IsValid()))
	{
		return false;
	}

	UBalhwajeomInvestigationSettings* MutableSettings =
		DuplicateObject<UBalhwajeomInvestigationSettings>(
			Settings, GetTransientPackage());
	if (!TestNotNull(TEXT("Mutable test settings should be created"), MutableSettings))
	{
		return false;
	}

	TestTrue(TEXT("Phase 01 objects should require tutorial completion"),
		Resolve(MutableSettings, TEXT("OBJ_01_018"), FGameplayTag()) ==
			TutorialDoneTag);
	TestTrue(TEXT("Phase 02 objects should require phase 02 unlock"),
		Resolve(MutableSettings, TEXT("OBJ_01_024"), FGameplayTag()) ==
			BalhwajeomGameplayTags::Story_Chapter_01_Phase_02_Unlocked);
	TestTrue(TEXT("Phase 03 objects should require phase 03 unlock"),
		Resolve(MutableSettings, TEXT("OBJ_01_010"), FGameplayTag()) ==
			BalhwajeomGameplayTags::Story_Chapter_01_Phase_03_Unlocked);
	TestTrue(TEXT("Unlisted objects should preserve their authored activation tag"),
		Resolve(MutableSettings, TEXT("OBJ_01_006"), AuthoredFallbackTag) ==
			AuthoredFallbackTag);

	FBoolProperty* EnabledProperty = FindFProperty<FBoolProperty>(
		MutableSettings->GetClass(), TEXT("bEnableChapter01PhaseSystem"));
	if (!TestNotNull(TEXT("Phase system toggle property should exist"), EnabledProperty))
	{
		return false;
	}
	EnabledProperty->SetPropertyValue_InContainer(MutableSettings, false);

	TestFalse(TEXT("Disabling the phase system should remove configured phase gates"),
		Resolve(MutableSettings, TEXT("OBJ_01_024"), FGameplayTag()).IsValid());
	TestTrue(TEXT("Disabling phase gates should preserve unrelated authored gates"),
		Resolve(MutableSettings, TEXT("OBJ_01_006"), AuthoredFallbackTag) ==
			AuthoredFallbackTag);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationChapter01PhaseSettingsValidationTest,
	"Balhwajeom.Investigation.Progression.Chapter01SettingsValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationChapter01PhaseSettingsValidationTest::RunTest(
	const FString& Parameters)
{
	const UBalhwajeomInvestigationSettings* DefaultSettings =
		GetDefault<UBalhwajeomInvestigationSettings>();
	UBalhwajeomInvestigationSettings* InvalidSettings =
		DuplicateObject<UBalhwajeomInvestigationSettings>(
			DefaultSettings, GetTransientPackage());
	if (!TestNotNull(TEXT("Invalid test settings should be created"), InvalidSettings))
	{
		return false;
	}

	InvalidSettings->Phase02ObjectIDs.Add(TEXT("OBJ_01_018"));
	InvalidSettings->Phase03ObjectIDs.Add(TEXT("OBJ_DOES_NOT_EXIST"));

	FDataValidationContext ValidationContext;
	const UBalhwajeomInvestigationSettings* ConstInvalidSettings = InvalidSettings;
	TestTrue(
		TEXT("Duplicate and unknown phase ObjectIDs should invalidate settings"),
		ConstInvalidSettings->IsDataValid(ValidationContext) ==
			EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationOnceInteractionTest,
	"Balhwajeom.Investigation.OnceInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationOnceInteractionTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	const FGuid InstanceID = Fixture.RegisterTestEvidence();
	FEvidenceInteractionViewData ViewData;

	TestTrue(TEXT("Once interaction should begin the first time"), Fixture.Subsystem->BeginEvidenceInteraction(InstanceID, ViewData));
	TestTrue(TEXT("Once interaction should complete the first time"), Fixture.Subsystem->CompleteEvidenceInteraction(InstanceID, ViewData.StateID));
	TestTrue(TEXT("Completing an interaction should grant its configured word"),
		Fixture.Subsystem->HasAcquiredWord(InvestigationSubsystemTests::WordID));
	TestFalse(TEXT("Completed Once interaction should not begin again"), Fixture.Subsystem->BeginEvidenceInteraction(InstanceID, ViewData));
	TestFalse(TEXT("Completed Once interaction should not complete again"), Fixture.Subsystem->CompleteEvidenceInteraction(InstanceID, InvestigationSubsystemTests::StateID));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationInteractionGrantedWordsTest,
	"Balhwajeom.Investigation.InteractionGrantedWords",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationInteractionGrantedWordsTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	const FGuid InstanceID = Fixture.RegisterTestEvidence();
	FEvidenceInteractionViewData ViewData;
	TArray<FName> NewlyGrantedWordIDs;
	TestTrue(TEXT("Configured interaction should begin"),
		Fixture.Subsystem->BeginEvidenceInteraction(InstanceID, ViewData));
	TestTrue(TEXT("Configured interaction should complete"),
		Fixture.Subsystem->CompleteEvidenceInteractionWithGrantedWords(
			InstanceID, ViewData.StateID, NewlyGrantedWordIDs));
	TestEqual(TEXT("Both configured keywords should be reported"),
		NewlyGrantedWordIDs.Num(), 2);
	TestTrue(TEXT("Primary keyword should be reported"),
		NewlyGrantedWordIDs.Contains(InvestigationSubsystemTests::WordID));
	TestTrue(TEXT("Alternate keyword should be reported"),
		NewlyGrantedWordIDs.Contains(InvestigationSubsystemTests::AlternateWordID));

	TArray<FAcquiredWordRecord> AcquiredWords;
	Fixture.Subsystem->GetAcquiredWords(AcquiredWords);
	const FAcquiredWordRecord* FoundWord = AcquiredWords.FindByPredicate(
		[](const FAcquiredWordRecord& Candidate)
		{
			return Candidate.WordID == InvestigationSubsystemTests::WordID;
		});
	if (TestNotNull(TEXT("Granted word record should exist"), FoundWord))
	{
		TestEqual(TEXT("Interaction keyword should preserve its source type"),
			FoundWord->SourceType, EWordAcquisitionSource::EvidenceInteraction);
		TestEqual(TEXT("Interaction keyword should preserve its source state"),
			FoundWord->SourceID, InvestigationSubsystemTests::StateID);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationDuplicateWordTest,
	"Balhwajeom.Investigation.DuplicateWord",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationDuplicateWordTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	TestTrue(TEXT("A known word should be acquired once"), Fixture.Subsystem->AcquireWord(
		InvestigationSubsystemTests::WordID,
		EWordAcquisitionSource::Messenger,
		FName(TEXT("MESSAGE_TEST"))));
	TestFalse(TEXT("The same WordID should not be acquired twice"), Fixture.Subsystem->AcquireWord(
		InvestigationSubsystemTests::WordID,
		EWordAcquisitionSource::EvidenceInteraction,
		InvestigationSubsystemTests::ObjectID));
	TArray<FAcquiredWordRecord> CharacterWords;
	Fixture.Subsystem->GetAcquiredWordsForCharacter(
		InvestigationSubsystemTests::CharacterID,
		CharacterWords);
	TestEqual(TEXT("The acquired word should appear in its assigned character folder"), CharacterWords.Num(), 1);
	TestEqual(
		TEXT("The character-folder query should preserve the acquired WordID"),
		CharacterWords[0].WordID,
		InvestigationSubsystemTests::WordID);
	TestEqual(
		TEXT("The character-folder query should preserve Messenger provenance"),
		CharacterWords[0].SourceType,
		EWordAcquisitionSource::Messenger);
	TestEqual(
		TEXT("The character-folder query should preserve MessageID"),
		CharacterWords[0].SourceID,
		FName(TEXT("MESSAGE_TEST")));
	Fixture.Subsystem->GetAcquiredWordsForCharacter(TEXT("CHAR_OTHER"), CharacterWords);
	TestTrue(TEXT("An unrelated character folder should not receive the word"), CharacterWords.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationKeywordChoiceTest,
	"Balhwajeom.Investigation.KeywordChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationKeywordChoiceTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	TArray<FKeywordChoiceDefinition> Choices;
	Fixture.Subsystem->GetKeywordChoicesForDocument(TEXT("DOC_TEST"), Choices);
	TestEqual(TEXT("Document should expose its normalized choice rows"), Choices.Num(), 1);
	TestTrue(TEXT("Selecting a valid choice should acquire its word"),
		Fixture.Subsystem->SelectKeywordChoice(
			TEXT("DOC_TEST"), TEXT("CHOICE_TEST"), EWordAcquisitionSource::EvidenceInteraction));
	TestTrue(TEXT("Choice word should be available to sentence solving"),
		Fixture.Subsystem->HasAcquiredWord(InvestigationSubsystemTests::WordID));
	TestFalse(TEXT("A choice cannot be selected twice"),
		Fixture.Subsystem->SelectKeywordChoice(
			TEXT("DOC_TEST"), TEXT("CHOICE_TEST"), EWordAcquisitionSource::EvidenceInteraction));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationDuplicatePhotoTest,
	"Balhwajeom.Investigation.DuplicatePhoto",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationDuplicatePhotoTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	const FCapturedPhotoRecord Record = Fixture.MakePhotoRecord(Fixture.RegisterTestEvidence());
	TestTrue(TEXT("A valid photo should register once"), Fixture.Subsystem->RegisterCapturedPhoto(Record));
	TestTrue(TEXT("Registered PhotoID should be reported as captured"), Fixture.Subsystem->HasCapturedPhoto(Record.PhotoID));
	TestTrue(TEXT("Photo capture should grant its configured word"), Fixture.Subsystem->HasAcquiredWord(InvestigationSubsystemTests::WordID));
	TestFalse(TEXT("The same PhotoID should not register twice"), Fixture.Subsystem->RegisterCapturedPhoto(Record));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationResetPersistentPhotoGalleryTest,
	"Balhwajeom.Investigation.PhotoGallery.ResetPersistentState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationResetPersistentPhotoGalleryTest::RunTest(
	const FString& Parameters)
{
	const FString AutomationPhotoDirectory = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Investigation"),
			TEXT("Photos"),
			TEXT("Automation")));
	const FString TestPhotoPath = FPaths::Combine(
		AutomationPhotoDirectory, TEXT("ResetPersistentState.png"));
	const FString AutomationSaveSlot = TEXT("BalhwajeomInvestigation_Automation");

	IFileManager::Get().DeleteDirectory(*AutomationPhotoDirectory, false, true);
	UGameplayStatics::DeleteGameInSlot(AutomationSaveSlot, 0);
	IFileManager::Get().MakeDirectory(*AutomationPhotoDirectory, true);
	if (!TestTrue(
		TEXT("The isolated automation photo should be created"),
		FFileHelper::SaveStringToFile(TEXT("test-photo"), *TestPhotoPath)))
	{
		return false;
	}

	UBalhwajeomInvestigationSaveGame* SaveGame =
		NewObject<UBalhwajeomInvestigationSaveGame>();
	if (!TestTrue(
		TEXT("The isolated automation gallery save should be created"),
		UGameplayStatics::SaveGameToSlot(SaveGame, AutomationSaveSlot, 0)))
	{
		IFileManager::Get().DeleteDirectory(*AutomationPhotoDirectory, false, true);
		return false;
	}

	const InvestigationSubsystemTests::FFixture Fixture;
	FCapturedPhotoRecord PhotoRecord =
		Fixture.MakePhotoRecord(Fixture.RegisterTestEvidence());
	PhotoRecord.ImageRelativePath =
		TEXT("Investigation/Photos/Automation/ResetPersistentState.png");
	FInvestigationSubsystemTestAccessor::SeedPersistentPhotoState(
		Fixture.Subsystem,
		PhotoRecord,
		InvestigationSubsystemTests::WordID);

	TestTrue(
		TEXT("Resetting the persistent gallery should report success"),
		Fixture.Subsystem->ResetPersistentPhotoGallery());
	TestFalse(
		TEXT("The captured photo should be removed from runtime state"),
		Fixture.Subsystem->HasCapturedPhoto(PhotoRecord.PhotoID));
	TestFalse(
		TEXT("A word restored only from that photo should be removed"),
		Fixture.Subsystem->HasAcquiredWord(InvestigationSubsystemTests::WordID));
	TestFalse(
		TEXT("The persisted image should be deleted"),
		IFileManager::Get().FileExists(*TestPhotoPath));
	TestFalse(
		TEXT("The gallery SaveGame slot should be deleted"),
		UGameplayStatics::DoesSaveGameExist(AutomationSaveSlot, 0));

	IFileManager::Get().DeleteDirectory(*AutomationPhotoDirectory, false, true);
	UGameplayStatics::DeleteGameInSlot(AutomationSaveSlot, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationInvalidStateTransitionTest,
	"Balhwajeom.Investigation.InvalidStateTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationInvalidStateTransitionTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	const FName InvalidObjectID(TEXT("OBJ_INVALID"));
	const FName OtherObjectID(TEXT("OBJ_OTHER"));
	const FName InvalidStateID(TEXT("STATE_INVALID_CHANGE"));
	const FName OtherStateID(TEXT("STATE_OTHER"));

	FEvidenceDefinition InvalidObject;
	InvalidObject.ObjectID = InvalidObjectID;
	InvalidObject.InitialStateID = InvalidStateID;
	Fixture.EvidenceDefinitions->AddRow(InvalidObjectID, InvalidObject);

	FEvidenceDefinition OtherObject;
	OtherObject.ObjectID = OtherObjectID;
	OtherObject.InitialStateID = OtherStateID;
	Fixture.EvidenceDefinitions->AddRow(OtherObjectID, OtherObject);

	FEvidenceStateDefinition InvalidState;
	InvalidState.StateID = InvalidStateID;
	InvalidState.ObjectID = InvalidObjectID;
	InvalidState.InteractionBehavior = EEvidenceInteractionBehavior::ChangeState;
	InvalidState.NextStateID = OtherStateID;
	Fixture.EvidenceStates->AddRow(InvalidStateID, InvalidState);

	FEvidenceStateDefinition OtherState;
	OtherState.StateID = OtherStateID;
	OtherState.ObjectID = OtherObjectID;
	Fixture.EvidenceStates->AddRow(OtherStateID, OtherState);

	const FGuid InstanceID = FGuid::NewGuid();
	FName CurrentStateID;
	TestTrue(TEXT("Evidence with a valid own initial state should register"), Fixture.Subsystem->RegisterEvidenceActor(InstanceID, InvalidObjectID, CurrentStateID));
	TestFalse(TEXT("A state transition into another ObjectID should be rejected"), Fixture.Subsystem->CompleteEvidenceInteraction(InstanceID, InvalidStateID));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationSentenceValidationTest,
	"Balhwajeom.Investigation.SentenceValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationSentenceValidationTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	Fixture.Subsystem->AcquireWord(
		InvestigationSubsystemTests::WordID,
		EWordAcquisitionSource::EvidenceInteraction,
		InvestigationSubsystemTests::ObjectID);
	Fixture.Subsystem->RegisterCapturedPhoto(
		Fixture.MakePhotoRecord(Fixture.RegisterTestEvidence()));

	FSentenceSubmission Submission;
	FSubmittedWordSlot SubmittedWord;
	SubmittedWord.SlotIndex = 0;
	SubmittedWord.WordID = InvestigationSubsystemTests::WordID;
	Submission.SubmittedWords.Add(SubmittedWord);

	FText ResultText;
	TestTrue(TEXT("An acquired correct word should solve the photo analysis"), Fixture.Subsystem->ValidateSentence(
		InvestigationSubsystemTests::SentenceID,
		Submission,
		ResultText));
	TestTrue(TEXT("Solved state should be queryable"), Fixture.Subsystem->IsSentenceSolved(InvestigationSubsystemTests::SentenceID));

	FSentenceSubmission StatementSubmission;
	FSubmittedPhotoSlot SubmittedPhoto;
	SubmittedPhoto.SlotIndex = 0;
	SubmittedPhoto.PhotoID = InvestigationSubsystemTests::PhotoID;
	StatementSubmission.SubmittedPhotos.Add(SubmittedPhoto);
	TestTrue(TEXT("A completed analysis photo should solve the statement"), Fixture.Subsystem->ValidateSentence(
		InvestigationSubsystemTests::StatementID,
		StatementSubmission,
		ResultText));
	TestEqual(TEXT("Solved statement should return its ResultText"), ResultText.ToString(), FString(TEXT("Test result text")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationSentenceOrderGroupTest,
	"Balhwajeom.Investigation.SentenceOrderGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationSentenceOrderGroupTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	Fixture.Subsystem->AcquireWord(
		InvestigationSubsystemTests::WordID,
		EWordAcquisitionSource::EvidenceInteraction,
		InvestigationSubsystemTests::ObjectID);
	Fixture.Subsystem->AcquireWord(
		InvestigationSubsystemTests::AlternateWordID,
		EWordAcquisitionSource::EvidenceInteraction,
		InvestigationSubsystemTests::ObjectID);

	FSentenceDefinition* Sentence = Fixture.Sentences->FindRow<FSentenceDefinition>(
		InvestigationSubsystemTests::SentenceID,
		TEXT("OrderGroup test"));
	if (!TestNotNull(TEXT("The test sentence should exist"), Sentence))
	{
		return false;
	}
	Sentence->WordSlots.Reset();
	Sentence->WordSlots.Add({0, InvestigationSubsystemTests::WordID, 1});
	Sentence->WordSlots.Add({1, InvestigationSubsystemTests::AlternateWordID, 1});

	FSentenceSubmission Submission;
	Submission.SubmittedWords.Add({0, InvestigationSubsystemTests::AlternateWordID});
	Submission.SubmittedWords.Add({1, InvestigationSubsystemTests::WordID});
	FText ResultText;
	TestTrue(
		TEXT("Words in the same non-zero OrderGroup should be accepted in either order"),
		Fixture.Subsystem->ValidateSentence(
			InvestigationSubsystemTests::SentenceID,
			Submission,
			ResultText));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInvestigationDataValidationTest,
	"Balhwajeom.Investigation.DataValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInvestigationDataValidationTest::RunTest(const FString& Parameters)
{
	const InvestigationSubsystemTests::FFixture Fixture;
	TestTrue(TEXT("A fully linked test database should validate"), FInvestigationSubsystemTestAccessor::Validate(Fixture.Subsystem));

	FPhotoDefinition MismatchedPhoto;
	MismatchedPhoto.PhotoID = FName(TEXT("PHOTO_WRONG_INTERNAL_ID"));
	MismatchedPhoto.PhotoSentenceID = InvestigationSubsystemTests::SentenceID;
	MismatchedPhoto.CharacterID = InvestigationSubsystemTests::CharacterID;
	Fixture.Photos->RemoveRow(InvestigationSubsystemTests::PhotoID);
	Fixture.Photos->AddRow(InvestigationSubsystemTests::PhotoID, MismatchedPhoto);

	AddExpectedError(TEXT("mismatched internal ID"), EAutomationExpectedErrorFlags::Contains, 1);
	AddExpectedError(TEXT("validation failed"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("A Row Name/internal ID mismatch should fail validation"), FInvestigationSubsystemTestAccessor::Validate(Fixture.Subsystem));
	return true;
}

#endif
