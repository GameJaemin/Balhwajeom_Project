#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"


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
		State.bCanCapture = true;
		State.PhotoID = PhotoID;
		EvidenceStates->AddRow(StateID, State);

		FWordDefinition Word;
		Word.WordID = WordID;
		Words->AddRow(WordID, Word);
		FWordDefinition AlternateWord;
		AlternateWord.WordID = AlternateWordID;
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
	TestFalse(TEXT("Completed Once interaction should not begin again"), Fixture.Subsystem->BeginEvidenceInteraction(InstanceID, ViewData));
	TestFalse(TEXT("Completed Once interaction should not complete again"), Fixture.Subsystem->CompleteEvidenceInteraction(InstanceID, InvestigationSubsystemTests::StateID));
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
		EWordAcquisitionSource::EvidenceInteraction,
		InvestigationSubsystemTests::ObjectID));
	TestFalse(TEXT("The same WordID should not be acquired twice"), Fixture.Subsystem->AcquireWord(
		InvestigationSubsystemTests::WordID,
		EWordAcquisitionSource::Messenger,
		FName(TEXT("MESSAGE_TEST"))));
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
