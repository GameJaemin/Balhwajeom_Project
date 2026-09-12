#include "Investigation/BalhwajeomInvestigationSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Investigation/BalhwajeomInvestigationSaveGame.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogBalhwajeomInvestigation, Log, All);

namespace
{
const FString PhotoGallerySaveSlot = TEXT("BalhwajeomInvestigation");
const FString AutomationPhotoGallerySaveSlot = TEXT("BalhwajeomInvestigation_Automation");

const FString& GetPhotoGallerySaveSlot()
{
#if WITH_DEV_AUTOMATION_TESTS
	if (GIsAutomationTesting)
	{
		return AutomationPhotoGallerySaveSlot;
	}
#endif
	return PhotoGallerySaveSlot;
}

template <typename RowType>
const RowType* FindInvestigationRow(
	const UDataTable* Table,
	FName RowID,
	const TCHAR* TableName)
{
	if (RowID.IsNone())
	{
		UE_LOG(
			LogBalhwajeomInvestigation,
			Warning,
			TEXT("Cannot search '%s' with an empty ID."),
			TableName);
		return nullptr;
	}

	if (!IsValid(Table))
	{
		UE_LOG(
			LogBalhwajeomInvestigation,
			Warning,
			TEXT("Cannot find row '%s': DataTable '%s' is not loaded."),
			*RowID.ToString(),
			TableName);
		return nullptr;
	}

	const FString Context = FString::Printf(
		TEXT("Investigation lookup in %s"),
		TableName);
	return Table->FindRow<RowType>(RowID, Context, true);
}

template <typename RowType, typename IDGetter>
bool ValidateTableRowIDs(
	const UDataTable* Table,
	const TCHAR* TableName,
	IDGetter GetInternalID)
{
	if (!IsValid(Table))
	{
		UE_LOG(
			LogBalhwajeomInvestigation,
			Error,
			TEXT("Required Investigation DataTable '%s' is not loaded."),
			TableName);
		return false;
	}

	if (Table->GetRowStruct() != RowType::StaticStruct())
	{
		UE_LOG(
			LogBalhwajeomInvestigation,
			Error,
			TEXT("Investigation DataTable '%s' uses the wrong row structure."),
			TableName);
		return false;
	}

	bool bIsValid = true;
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const RowType* Row = reinterpret_cast<const RowType*>(Pair.Value);
		const FName InternalID = GetInternalID(*Row);
		if (Pair.Key != InternalID)
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Error,
				TEXT("DataTable '%s' row '%s' has mismatched internal ID '%s'."),
				TableName,
				*Pair.Key.ToString(),
				*InternalID.ToString());
			bIsValid = false;
		}
	}

	return bIsValid;
}
}

void UBalhwajeomInvestigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfiguredDataTables();
	ValidateLoadedDataTables();
	InitializeDefaultWords();
	LoadPersistentPhotoGallery();
}

void UBalhwajeomInvestigationSubsystem::Deinitialize()
{
	EvidenceRuntimeStates.Empty();
	AcquiredWords.Empty();
	CapturedPhotos.Empty();
	KeywordDocumentStates.Empty();
	SentenceProgress.Empty();
	ClearLoadedDataTables();
	Super::Deinitialize();
}

void UBalhwajeomInvestigationSubsystem::LoadConfiguredDataTables()
{
	const UBalhwajeomInvestigationSettings* Settings =
		GetDefault<UBalhwajeomInvestigationSettings>();

	auto LoadTable = [](const TSoftObjectPtr<UDataTable>& TableReference, const TCHAR* SettingName)
		-> UDataTable*
	{
		if (TableReference.IsNull())
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Warning,
				TEXT("Investigation DataTable setting '%s' is not configured."),
				SettingName);
			return nullptr;
		}

		UDataTable* LoadedTable = TableReference.LoadSynchronous();
		if (!IsValid(LoadedTable))
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Error,
				TEXT("Failed to load Investigation DataTable '%s' from '%s'."),
				SettingName,
				*TableReference.ToSoftObjectPath().ToString());
		}

		return LoadedTable;
	};

	EvidenceDefinitionsTable = LoadTable(
		Settings->EvidenceDefinitionsTable,
		TEXT("EvidenceDefinitionsTable"));
	EvidenceStatesTable = LoadTable(
		Settings->EvidenceStatesTable,
		TEXT("EvidenceStatesTable"));
	WordsTable = LoadTable(Settings->WordsTable, TEXT("WordsTable"));
	PhotosTable = LoadTable(Settings->PhotosTable, TEXT("PhotosTable"));
	KeywordDocumentsTable = LoadTable(
		Settings->KeywordDocumentsTable,
		TEXT("KeywordDocumentsTable"));
	KeywordChoicesTable = LoadTable(
		Settings->KeywordChoicesTable,
		TEXT("KeywordChoicesTable"));
	SentencesTable = LoadTable(Settings->SentencesTable, TEXT("SentencesTable"));
	CharactersTable = LoadTable(Settings->CharactersTable, TEXT("CharactersTable"));
}

void UBalhwajeomInvestigationSubsystem::ClearLoadedDataTables()
{
	EvidenceDefinitionsTable = nullptr;
	EvidenceStatesTable = nullptr;
	WordsTable = nullptr;
	PhotosTable = nullptr;
	KeywordDocumentsTable = nullptr;
	KeywordChoicesTable = nullptr;
	SentencesTable = nullptr;
	CharactersTable = nullptr;
}

void UBalhwajeomInvestigationSubsystem::InitializeDefaultWords()
{
	if (!IsValid(WordsTable) || WordsTable->GetRowStruct() != FWordDefinition::StaticStruct())
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : WordsTable->GetRowMap())
	{
		const FWordDefinition* Definition =
			reinterpret_cast<const FWordDefinition*>(Pair.Value);
		if (!Definition->bUnlockedByDefault || AcquiredWords.Contains(Definition->WordID))
		{
			continue;
		}

		FAcquiredWordRecord Record;
		Record.WordID = Definition->WordID;
		Record.SourceType = EWordAcquisitionSource::Default;
		Record.AcquiredTime = FDateTime::UtcNow();
		AcquiredWords.Add(Record.WordID, Record);
	}
}

bool UBalhwajeomInvestigationSubsystem::ShouldPersistPhotoGallery() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	return World && World->IsGameWorld();
}

void UBalhwajeomInvestigationSubsystem::LoadPersistentPhotoGallery()
{
	if (!ShouldPersistPhotoGallery())
	{
		return;
	}
	if (!UGameplayStatics::DoesSaveGameExist(GetPhotoGallerySaveSlot(), 0))
	{
		return;
	}

	UBalhwajeomInvestigationSaveGame* SaveGame = Cast<UBalhwajeomInvestigationSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GetPhotoGallerySaveSlot(), 0));
	if (!SaveGame)
	{
		return;
	}

	const FString GalleryRoot = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Investigation"), TEXT("Photos")));
	for (const FCapturedPhotoRecord& Record : SaveGame->CapturedPhotos)
	{
		const FPhotoDefinition* PhotoDefinition = FindPhotoDefinition(Record.PhotoID);
		const FEvidenceStateDefinition* StateDefinition =
			FindEvidenceStateDefinition(Record.CapturedStateID);
		const FString AbsoluteImagePath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectSavedDir(), Record.ImageRelativePath));
		if (Record.PhotoID.IsNone() || Record.ObjectID.IsNone() ||
			!PhotoDefinition || !StateDefinition ||
			StateDefinition->ObjectID != Record.ObjectID ||
			StateDefinition->PhotoID != Record.PhotoID || !StateDefinition->bCanCapture ||
			!FPaths::IsUnderDirectory(AbsoluteImagePath, GalleryRoot) ||
			IFileManager::Get().FileSize(*AbsoluteImagePath) <= 0)
		{
			UE_LOG(LogBalhwajeomInvestigation, Warning,
				TEXT("Ignoring invalid persistent photo '%s' (%s)."),
				*Record.PhotoID.ToString(), *Record.ImageRelativePath);
			continue;
		}

		CapturedPhotos.FindOrAdd(Record.PhotoID) = Record;
		for (const FName WordID : PhotoDefinition->GrantedWordIDs)
		{
			AcquireWord(WordID, EWordAcquisitionSource::PhotoCapture, Record.PhotoID);
		}
	}

	UE_LOG(LogBalhwajeomInvestigation, Log,
		TEXT("Restored %d persistent photograph(s) from slot '%s'."),
		CapturedPhotos.Num(), *GetPhotoGallerySaveSlot());
}

bool UBalhwajeomInvestigationSubsystem::SavePersistentPhotoGallery() const
{
	if (!ShouldPersistPhotoGallery())
	{
		return true;
	}

	UBalhwajeomInvestigationSaveGame* SaveGame = Cast<UBalhwajeomInvestigationSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UBalhwajeomInvestigationSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	CapturedPhotos.GenerateValueArray(SaveGame->CapturedPhotos);
	SaveGame->CapturedPhotos.Sort([](const FCapturedPhotoRecord& A, const FCapturedPhotoRecord& B)
	{
		return A.PhotoID.LexicalLess(B.PhotoID);
	});
	return UGameplayStatics::SaveGameToSlot(SaveGame, GetPhotoGallerySaveSlot(), 0);
}

bool UBalhwajeomInvestigationSubsystem::ValidateLoadedDataTables() const
{
	bool bIsValid = true;

	bIsValid &= ValidateTableRowIDs<FEvidenceDefinition>(
		EvidenceDefinitionsTable,
		TEXT("EvidenceDefinitionsTable"),
		[](const FEvidenceDefinition& Row) { return Row.ObjectID; });
	bIsValid &= ValidateTableRowIDs<FEvidenceStateDefinition>(
		EvidenceStatesTable,
		TEXT("EvidenceStatesTable"),
		[](const FEvidenceStateDefinition& Row) { return Row.StateID; });
	bIsValid &= ValidateTableRowIDs<FWordDefinition>(
		WordsTable,
		TEXT("WordsTable"),
		[](const FWordDefinition& Row) { return Row.WordID; });
	bIsValid &= ValidateTableRowIDs<FPhotoDefinition>(
		PhotosTable,
		TEXT("PhotosTable"),
		[](const FPhotoDefinition& Row) { return Row.PhotoID; });
	bIsValid &= ValidateTableRowIDs<FKeywordDocumentDefinition>(
		KeywordDocumentsTable,
		TEXT("KeywordDocumentsTable"),
		[](const FKeywordDocumentDefinition& Row) { return Row.KeywordDocumentID; });
	bIsValid &= ValidateTableRowIDs<FKeywordChoiceDefinition>(
		KeywordChoicesTable,
		TEXT("KeywordChoicesTable"),
		[](const FKeywordChoiceDefinition& Row) { return Row.ChoiceID; });
	bIsValid &= ValidateTableRowIDs<FSentenceDefinition>(
		SentencesTable,
		TEXT("SentencesTable"),
		[](const FSentenceDefinition& Row) { return Row.SentenceID; });
	bIsValid &= ValidateTableRowIDs<FCharacterDefinition>(
		CharactersTable,
		TEXT("CharactersTable"),
		[](const FCharacterDefinition& Row) { return Row.CharacterID; });

	if (!bIsValid)
	{
		UE_LOG(LogBalhwajeomInvestigation, Error, TEXT("Investigation DataTable validation failed."));
		return false;
	}

	const FString Context(TEXT("Investigation cross-reference validation"));
	auto HasEvidenceObject = [this, &Context](FName ObjectID)
	{
		return !ObjectID.IsNone() &&
			EvidenceDefinitionsTable->FindRow<FEvidenceDefinition>(ObjectID, Context, false) != nullptr;
	};
	auto FindEvidenceState = [this, &Context](FName StateID)
	{
		return StateID.IsNone()
			? static_cast<const FEvidenceStateDefinition*>(nullptr)
			: EvidenceStatesTable->FindRow<FEvidenceStateDefinition>(StateID, Context, false);
	};
	auto HasWord = [this, &Context](FName WordID)
	{
		return !WordID.IsNone() && WordsTable->FindRow<FWordDefinition>(WordID, Context, false) != nullptr;
	};
	auto HasPhoto = [this, &Context](FName PhotoID)
	{
		return !PhotoID.IsNone() && PhotosTable->FindRow<FPhotoDefinition>(PhotoID, Context, false) != nullptr;
	};
	auto HasKeywordDocument = [this, &Context](FName DocumentID)
	{
		return !DocumentID.IsNone() &&
			KeywordDocumentsTable->FindRow<FKeywordDocumentDefinition>(DocumentID, Context, false) != nullptr;
	};
	auto HasCharacter = [this, &Context](FName CharacterID)
	{
		return !CharacterID.IsNone() &&
			CharactersTable->FindRow<FCharacterDefinition>(CharacterID, Context, false) != nullptr;
	};
	auto ReportInvalidReference = [&bIsValid](
		const TCHAR* OwnerType,
		FName OwnerID,
		const TCHAR* FieldName,
		FName ReferencedID)
	{
		UE_LOG(
			LogBalhwajeomInvestigation,
			Error,
			TEXT("%s '%s' has invalid %s '%s'."),
			OwnerType,
			*OwnerID.ToString(),
			FieldName,
			*ReferencedID.ToString());
		bIsValid = false;
	};

	for (const TPair<FName, uint8*>& Pair : WordsTable->GetRowMap())
	{
		const FWordDefinition* Word = reinterpret_cast<const FWordDefinition*>(Pair.Value);
		TSet<FName> SeenCharacterIDs;
		for (const FName CharacterID : Word->RelatedCharacterIDs)
		{
			if (!HasCharacter(CharacterID) || SeenCharacterIDs.Contains(CharacterID))
			{
				ReportInvalidReference(
					TEXT("WordDefinition"), Word->WordID, TEXT("RelatedCharacterIDs"), CharacterID);
			}
			SeenCharacterIDs.Add(CharacterID);
		}
	}

	for (const TPair<FName, uint8*>& Pair : EvidenceDefinitionsTable->GetRowMap())
	{
		const FEvidenceDefinition* Definition =
			reinterpret_cast<const FEvidenceDefinition*>(Pair.Value);
		const FEvidenceStateDefinition* InitialState = FindEvidenceState(Definition->InitialStateID);
		if (InitialState == nullptr || InitialState->ObjectID != Definition->ObjectID)
		{
			ReportInvalidReference(
				TEXT("EvidenceDefinition"),
				Definition->ObjectID,
				TEXT("InitialStateID"),
				Definition->InitialStateID);
		}
	}

	for (const TPair<FName, uint8*>& Pair : EvidenceStatesTable->GetRowMap())
	{
		const FEvidenceStateDefinition* State =
			reinterpret_cast<const FEvidenceStateDefinition*>(Pair.Value);

		if (!HasEvidenceObject(State->ObjectID))
		{
			ReportInvalidReference(TEXT("EvidenceState"), State->StateID, TEXT("ObjectID"), State->ObjectID);
		}

		if (State->InteractionBehavior == EEvidenceInteractionBehavior::ChangeState)
		{
			const FEvidenceStateDefinition* NextState = FindEvidenceState(State->NextStateID);
			if (NextState == nullptr || NextState->ObjectID != State->ObjectID)
			{
				ReportInvalidReference(
					TEXT("EvidenceState"), State->StateID, TEXT("NextStateID"), State->NextStateID);
			}
		}

		if (State->InteractionPresentation ==
			EEvidenceInteractionPresentation::KeywordSelectionWindow &&
			!HasKeywordDocument(State->KeywordDocumentID))
		{
			ReportInvalidReference(
				TEXT("EvidenceState"),
				State->StateID,
				TEXT("KeywordDocumentID"),
				State->KeywordDocumentID);
		}

		if (State->bCanCapture && !HasPhoto(State->PhotoID))
		{
			ReportInvalidReference(TEXT("EvidenceState"), State->StateID, TEXT("PhotoID"), State->PhotoID);
		}
	}

	for (const TPair<FName, uint8*>& Pair : PhotosTable->GetRowMap())
	{
		const FPhotoDefinition* Photo = reinterpret_cast<const FPhotoDefinition*>(Pair.Value);
		TSet<FName> GrantedWords;
		for (const FName WordID : Photo->GrantedWordIDs)
		{
			if (!HasWord(WordID) || GrantedWords.Contains(WordID))
			{
				ReportInvalidReference(
					TEXT("PhotoDefinition"), Photo->PhotoID, TEXT("GrantedWordIDs"), WordID);
			}
			GrantedWords.Add(WordID);
		}
		if (!Photo->PhotoSentenceID.IsNone())
		{
			const FSentenceDefinition* PhotoSentence = SentencesTable->FindRow<FSentenceDefinition>(
				Photo->PhotoSentenceID, Context, false);
			if (PhotoSentence == nullptr || PhotoSentence->SentenceType != ESentenceType::PhotoAnalysis)
			{
				ReportInvalidReference(
					TEXT("PhotoDefinition"), Photo->PhotoID, TEXT("PhotoSentenceID"), Photo->PhotoSentenceID);
			}
		}

		if (!Photo->EvidenceSentenceID.IsNone())
		{
			const FSentenceDefinition* EvidenceSentence = SentencesTable->FindRow<FSentenceDefinition>(
				Photo->EvidenceSentenceID, Context, false);
			if (EvidenceSentence == nullptr || EvidenceSentence->SentenceType != ESentenceType::Statement)
			{
				ReportInvalidReference(
					TEXT("PhotoDefinition"), Photo->PhotoID, TEXT("EvidenceSentenceID"), Photo->EvidenceSentenceID);
			}
		}

		if (!HasCharacter(Photo->CharacterID))
		{
			ReportInvalidReference(
				TEXT("PhotoDefinition"), Photo->PhotoID, TEXT("CharacterID"), Photo->CharacterID);
		}

		if (!Photo->WorldStoryCues.IsEmpty())
		{
			float PreviousStartTime = -1.0f;
			for (int32 CueIndex = 0; CueIndex < Photo->WorldStoryCues.Num(); ++CueIndex)
			{
				const FPhotoStoryCue& Cue = Photo->WorldStoryCues[CueIndex];
				const bool bInvalidFirstTime = CueIndex == 0 && !FMath::IsNearlyZero(Cue.StartTimeSeconds);
				const bool bInvalidOrder = Cue.StartTimeSeconds < PreviousStartTime;
				if (Cue.StartTimeSeconds < 0.0f || bInvalidFirstTime || bInvalidOrder)
				{
					UE_LOG(
						LogBalhwajeomInvestigation,
						Error,
						TEXT("PhotoDefinition '%s' has invalid WorldStoryCues[%d]. The first cue must start at 0, and times must be ascending. Empty or whitespace text is allowed."),
						*Photo->PhotoID.ToString(),
						CueIndex);
					bIsValid = false;
				}
				PreviousStartTime = Cue.StartTimeSeconds;
			}
		}
	}

	TMap<FName, TSet<int32>> SortOrdersByDocument;
	for (const TPair<FName, uint8*>& Pair : KeywordChoicesTable->GetRowMap())
	{
		const FKeywordChoiceDefinition* Choice =
			reinterpret_cast<const FKeywordChoiceDefinition*>(Pair.Value);
		if (!HasKeywordDocument(Choice->KeywordDocumentID))
		{
			ReportInvalidReference(
				TEXT("KeywordChoice"), Choice->ChoiceID, TEXT("KeywordDocumentID"), Choice->KeywordDocumentID);
		}
		if (!HasWord(Choice->GrantedWordID))
		{
			ReportInvalidReference(
				TEXT("KeywordChoice"), Choice->ChoiceID, TEXT("GrantedWordID"), Choice->GrantedWordID);
		}

		TSet<int32>& UsedSortOrders = SortOrdersByDocument.FindOrAdd(Choice->KeywordDocumentID);
		if (UsedSortOrders.Contains(Choice->SortOrder))
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Warning,
				TEXT("Keyword document '%s' contains duplicate SortOrder %d."),
				*Choice->KeywordDocumentID.ToString(),
				Choice->SortOrder);
		}
		UsedSortOrders.Add(Choice->SortOrder);
	}

	for (const TPair<FName, uint8*>& Pair : SentencesTable->GetRowMap())
	{
		const FSentenceDefinition* Sentence =
			reinterpret_cast<const FSentenceDefinition*>(Pair.Value);
		if (Sentence->SentenceType == ESentenceType::Statement && !HasCharacter(Sentence->CharacterID))
		{
			ReportInvalidReference(
				TEXT("Sentence"), Sentence->SentenceID, TEXT("CharacterID"), Sentence->CharacterID);
		}
		else if (!Sentence->CharacterID.IsNone() && !HasCharacter(Sentence->CharacterID))
		{
			ReportInvalidReference(
				TEXT("Sentence"), Sentence->SentenceID, TEXT("CharacterID"), Sentence->CharacterID);
		}
		TSet<int32> WordSlotIndices;
		for (const FSentenceWordSlot& Slot : Sentence->WordSlots)
		{
			if (Slot.SlotIndex < 0 || Slot.SlotIndex > 9 || WordSlotIndices.Contains(Slot.SlotIndex))
			{
				ReportInvalidReference(
					TEXT("Sentence"), Sentence->SentenceID, TEXT("WordSlotIndex"), FName(*FString::FromInt(Slot.SlotIndex)));
			}
			WordSlotIndices.Add(Slot.SlotIndex);
			const FWordDefinition* CorrectWord = Slot.CorrectWordID.IsNone()
				? nullptr
				: WordsTable->FindRow<FWordDefinition>(Slot.CorrectWordID, Context, false);
			if (!CorrectWord)
			{
				ReportInvalidReference(
					TEXT("Sentence"), Sentence->SentenceID, TEXT("CorrectWordID"), Slot.CorrectWordID);
			}
			else if (Sentence->SentenceType == ESentenceType::Statement &&
				!CorrectWord->RelatedCharacterIDs.Contains(Sentence->CharacterID))
			{
				ReportInvalidReference(
					TEXT("Sentence"),
					Sentence->SentenceID,
					TEXT("CorrectWordIDFolderAssignment"),
					Slot.CorrectWordID);
			}
		}

		TSet<int32> PhotoSlotIndices;
		for (const FSentencePhotoSlot& Slot : Sentence->PhotoSlots)
		{
			if (Slot.SlotIndex < 0 || Slot.SlotIndex > 1 || PhotoSlotIndices.Contains(Slot.SlotIndex))
			{
				ReportInvalidReference(
					TEXT("Sentence"), Sentence->SentenceID, TEXT("PhotoSlotIndex"), FName(*FString::FromInt(Slot.SlotIndex)));
			}
			PhotoSlotIndices.Add(Slot.SlotIndex);
			if (!HasPhoto(Slot.CorrectPhotoID))
			{
				ReportInvalidReference(
					TEXT("Sentence"), Sentence->SentenceID, TEXT("CorrectPhotoID"), Slot.CorrectPhotoID);
			}
		}

		if (Sentence->RequiredPhotoCount < 0 ||
			Sentence->RequiredPhotoCount > Sentence->PhotoSlots.Num())
		{
			ReportInvalidReference(
				TEXT("Sentence"),
				Sentence->SentenceID,
				TEXT("RequiredPhotoCount"),
				FName(*FString::FromInt(Sentence->RequiredPhotoCount)));
		}

		if (Sentence->ResultText.IsEmpty())
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Error,
				TEXT("Sentence '%s' has an empty ResultText."),
				*Sentence->SentenceID.ToString());
			bIsValid = false;
		}
	}

	// Exactly one Statement row per character (among characters that have any Statement rows at
	// all) should be flagged as that character's fixed tablet folder file; every other Statement
	// row is a photo-declaration entry only reached via a Photo's EvidenceSentenceID.
	TMap<FName, int32> StatementCountsByCharacter;
	TMap<FName, int32> FolderStatementCountsByCharacter;
	for (const TPair<FName, uint8*>& Pair : SentencesTable->GetRowMap())
	{
		const FSentenceDefinition* Sentence = reinterpret_cast<const FSentenceDefinition*>(Pair.Value);
		if (Sentence->SentenceType != ESentenceType::Statement)
		{
			continue;
		}
		++StatementCountsByCharacter.FindOrAdd(Sentence->CharacterID);
		if (Sentence->bIsFolderStatement)
		{
			++FolderStatementCountsByCharacter.FindOrAdd(Sentence->CharacterID);
		}
	}
	for (const TPair<FName, int32>& Pair : StatementCountsByCharacter)
	{
		const int32 FolderStatementCount = FolderStatementCountsByCharacter.FindRef(Pair.Key);
		if (FolderStatementCount != 1)
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Error,
				TEXT("Character '%s' has %d Statement rows with bIsFolderStatement set (expected exactly 1)."),
				*Pair.Key.ToString(),
				FolderStatementCount);
			bIsValid = false;
		}
	}

	if (bIsValid)
	{
		UE_LOG(LogBalhwajeomInvestigation, Log, TEXT("Investigation DataTable validation succeeded."));
	}
	else
	{
		UE_LOG(LogBalhwajeomInvestigation, Error, TEXT("Investigation DataTable validation failed."));
	}
	return bIsValid;
}

bool UBalhwajeomInvestigationSubsystem::GetEvidenceDefinition(
	FName ObjectID,
	FEvidenceDefinition& OutDefinition) const
{
	OutDefinition = FEvidenceDefinition{};

	const FEvidenceDefinition* Definition = FindEvidenceDefinition(ObjectID);
	if (Definition == nullptr)
	{
		return false;
	}

	OutDefinition = *Definition;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::GetEvidenceStateDefinition(
	FName StateID,
	FEvidenceStateDefinition& OutState) const
{
	OutState = FEvidenceStateDefinition{};

	const FEvidenceStateDefinition* Definition = FindEvidenceStateDefinition(StateID);
	if (Definition == nullptr)
	{
		return false;
	}

	OutState = *Definition;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::GetPhotoDefinition(
	FName PhotoID,
	FPhotoDefinition& OutDefinition) const
{
	OutDefinition = FPhotoDefinition{};

	const FPhotoDefinition* Definition = FindPhotoDefinition(PhotoID);
	if (Definition == nullptr)
	{
		return false;
	}

	OutDefinition = *Definition;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::GetCharacterDefinition(
	FName CharacterID,
	FCharacterDefinition& OutDefinition) const
{
	OutDefinition = FCharacterDefinition{};
	const FCharacterDefinition* Definition = FindCharacterDefinition(CharacterID);
	if (Definition == nullptr)
	{
		return false;
	}
	OutDefinition = *Definition;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::GetWordDefinition(
	FName WordID,
	FWordDefinition& OutDefinition) const
{
	OutDefinition = FWordDefinition{};
	const FWordDefinition* Definition = FindWordDefinition(WordID);
	if (Definition == nullptr)
	{
		return false;
	}
	OutDefinition = *Definition;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::GetSentenceDefinition(
	FName SentenceID,
	FSentenceDefinition& OutDefinition) const
{
	OutDefinition = FSentenceDefinition{};
	const FSentenceDefinition* Definition = FindSentenceDefinition(SentenceID);
	if (Definition == nullptr)
	{
		return false;
	}
	OutDefinition = *Definition;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::GetKeywordDocumentDefinition(
	FName KeywordDocumentID,
	FKeywordDocumentDefinition& OutDefinition) const
{
	OutDefinition = FKeywordDocumentDefinition{};
	const FKeywordDocumentDefinition* Definition =
		FindKeywordDocumentDefinition(KeywordDocumentID);
	if (Definition == nullptr)
	{
		return false;
	}
	OutDefinition = *Definition;
	return true;
}

void UBalhwajeomInvestigationSubsystem::GetKeywordChoicesForDocument(
	FName KeywordDocumentID,
	TArray<FKeywordChoiceDefinition>& OutChoices) const
{
	OutChoices.Reset();
	if (!IsValid(KeywordChoicesTable) ||
		KeywordChoicesTable->GetRowStruct() != FKeywordChoiceDefinition::StaticStruct())
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : KeywordChoicesTable->GetRowMap())
	{
		const FKeywordChoiceDefinition* Choice =
			reinterpret_cast<const FKeywordChoiceDefinition*>(Pair.Value);
		if (Choice->KeywordDocumentID == KeywordDocumentID)
		{
			OutChoices.Add(*Choice);
		}
	}
	OutChoices.Sort([](const FKeywordChoiceDefinition& A, const FKeywordChoiceDefinition& B)
	{
		return A.SortOrder == B.SortOrder
			? A.ChoiceID.LexicalLess(B.ChoiceID)
			: A.SortOrder < B.SortOrder;
	});
}

bool UBalhwajeomInvestigationSubsystem::SelectKeywordChoice(
	FName KeywordDocumentID,
	FName ChoiceID,
	EWordAcquisitionSource SourceType)
{
	if (FindKeywordDocumentDefinition(KeywordDocumentID) == nullptr)
	{
		return false;
	}
	const FKeywordChoiceDefinition* Choice = FindInvestigationRow<FKeywordChoiceDefinition>(
		KeywordChoicesTable, ChoiceID, TEXT("KeywordChoicesTable"));
	if (Choice == nullptr || Choice->KeywordDocumentID != KeywordDocumentID)
	{
		return false;
	}

	FKeywordDocumentRuntimeState& State = KeywordDocumentStates.FindOrAdd(KeywordDocumentID);
	State.KeywordDocumentID = KeywordDocumentID;
	if (State.SelectedChoiceIDs.Contains(ChoiceID))
	{
		return false;
	}
	State.SelectedChoiceIDs.Add(ChoiceID);
	State.bCompleted = true;
	return HasAcquiredWord(Choice->GrantedWordID) ||
		AcquireWord(Choice->GrantedWordID, SourceType, KeywordDocumentID);
}

const FEvidenceDefinition* UBalhwajeomInvestigationSubsystem::FindEvidenceDefinition(
	FName ObjectID) const
{
	return FindInvestigationRow<FEvidenceDefinition>(
		EvidenceDefinitionsTable,
		ObjectID,
		TEXT("EvidenceDefinitionsTable"));
}

const FEvidenceStateDefinition* UBalhwajeomInvestigationSubsystem::FindEvidenceStateDefinition(
	FName StateID) const
{
	return FindInvestigationRow<FEvidenceStateDefinition>(
		EvidenceStatesTable,
		StateID,
		TEXT("EvidenceStatesTable"));
}

const FWordDefinition* UBalhwajeomInvestigationSubsystem::FindWordDefinition(FName WordID) const
{
	return FindInvestigationRow<FWordDefinition>(WordsTable, WordID, TEXT("WordsTable"));
}

const FPhotoDefinition* UBalhwajeomInvestigationSubsystem::FindPhotoDefinition(FName PhotoID) const
{
	return FindInvestigationRow<FPhotoDefinition>(PhotosTable, PhotoID, TEXT("PhotosTable"));
}

const FKeywordDocumentDefinition*
UBalhwajeomInvestigationSubsystem::FindKeywordDocumentDefinition(FName KeywordDocumentID) const
{
	return FindInvestigationRow<FKeywordDocumentDefinition>(
		KeywordDocumentsTable,
		KeywordDocumentID,
		TEXT("KeywordDocumentsTable"));
}

const FSentenceDefinition* UBalhwajeomInvestigationSubsystem::FindSentenceDefinition(
	FName SentenceID) const
{
	return FindInvestigationRow<FSentenceDefinition>(
		SentencesTable,
		SentenceID,
		TEXT("SentencesTable"));
}

const FCharacterDefinition* UBalhwajeomInvestigationSubsystem::FindCharacterDefinition(
	FName CharacterID) const
{
	return FindInvestigationRow<FCharacterDefinition>(
		CharactersTable,
		CharacterID,
		TEXT("CharactersTable"));
}

bool UBalhwajeomInvestigationSubsystem::RegisterEvidenceActor(
	FGuid EvidenceInstanceID,
	FName ObjectID,
	FName& OutCurrentStateID)
{
	OutCurrentStateID = NAME_None;

	if (!EvidenceInstanceID.IsValid())
	{
		UE_LOG(LogBalhwajeomInvestigation, Warning, TEXT("Cannot register evidence with an invalid instance ID."));
		return false;
	}

	const FEvidenceDefinition* ObjectDefinition = FindEvidenceDefinition(ObjectID);
	if (ObjectDefinition == nullptr)
	{
		return false;
	}

	if (FEvidenceRuntimeState* ExistingState = EvidenceRuntimeStates.Find(EvidenceInstanceID))
	{
		if (ExistingState->ObjectID != ObjectID)
		{
			UE_LOG(
				LogBalhwajeomInvestigation,
				Error,
				TEXT("Evidence instance '%s' is already registered as ObjectID '%s', not '%s'."),
				*EvidenceInstanceID.ToString(),
				*ExistingState->ObjectID.ToString(),
				*ObjectID.ToString());
			return false;
		}

		OutCurrentStateID = ExistingState->CurrentStateID;
		return true;
	}

	const FEvidenceStateDefinition* InitialState =
		FindEvidenceStateDefinition(ObjectDefinition->InitialStateID);
	if (InitialState == nullptr || InitialState->ObjectID != ObjectID)
	{
		UE_LOG(
			LogBalhwajeomInvestigation,
			Error,
			TEXT("Evidence '%s' has an invalid initial state '%s'."),
			*ObjectID.ToString(),
			*ObjectDefinition->InitialStateID.ToString());
		return false;
	}

	FEvidenceRuntimeState NewState;
	NewState.EvidenceInstanceID = EvidenceInstanceID;
	NewState.ObjectID = ObjectID;
	NewState.CurrentStateID = InitialState->StateID;
	EvidenceRuntimeStates.Add(EvidenceInstanceID, NewState);

	OutCurrentStateID = NewState.CurrentStateID;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::BeginEvidenceInteraction(
	FGuid EvidenceInstanceID,
	FEvidenceInteractionViewData& OutViewData)
{
	OutViewData = FEvidenceInteractionViewData{};

	const FEvidenceRuntimeState* RuntimeState = EvidenceRuntimeStates.Find(EvidenceInstanceID);
	if (RuntimeState == nullptr)
	{
		return false;
	}

	const FEvidenceStateDefinition* StateDefinition =
		FindEvidenceStateDefinition(RuntimeState->CurrentStateID);
	if (StateDefinition == nullptr || StateDefinition->ObjectID != RuntimeState->ObjectID)
	{
		return false;
	}

	if (StateDefinition->InteractionBehavior == EEvidenceInteractionBehavior::None)
	{
		return false;
	}

	if (StateDefinition->InteractionBehavior == EEvidenceInteractionBehavior::Once &&
		RuntimeState->CompletedInteractionStateIDs.Contains(StateDefinition->StateID))
	{
		return false;
	}

	OutViewData.EvidenceInstanceID = EvidenceInstanceID;
	OutViewData.ObjectID = RuntimeState->ObjectID;
	OutViewData.StateID = StateDefinition->StateID;
	OutViewData.Behavior = StateDefinition->InteractionBehavior;
	OutViewData.Presentation = StateDefinition->InteractionPresentation;
	OutViewData.InteractionText = StateDefinition->InteractionText;
	OutViewData.KeywordDocumentID = StateDefinition->KeywordDocumentID;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::CompleteEvidenceInteraction(
	FGuid EvidenceInstanceID,
	FName ExpectedStateID)
{
	FEvidenceRuntimeState* RuntimeState = EvidenceRuntimeStates.Find(EvidenceInstanceID);
	if (RuntimeState == nullptr || RuntimeState->CurrentStateID != ExpectedStateID)
	{
		return false;
	}

	const FEvidenceStateDefinition* StateDefinition =
		FindEvidenceStateDefinition(RuntimeState->CurrentStateID);
	if (StateDefinition == nullptr || StateDefinition->ObjectID != RuntimeState->ObjectID)
	{
		return false;
	}

	switch (StateDefinition->InteractionBehavior)
	{
	case EEvidenceInteractionBehavior::Once:
		if (RuntimeState->CompletedInteractionStateIDs.Contains(ExpectedStateID))
		{
			return false;
		}
		RuntimeState->CompletedInteractionStateIDs.Add(ExpectedStateID);
		return true;

	case EEvidenceInteractionBehavior::Repeatable:
		return true;

	case EEvidenceInteractionBehavior::ChangeState:
	{
		const FEvidenceStateDefinition* NextState =
			FindEvidenceStateDefinition(StateDefinition->NextStateID);
		if (NextState == nullptr || NextState->ObjectID != RuntimeState->ObjectID)
		{
			return false;
		}

		const FName PreviousStateID = RuntimeState->CurrentStateID;
		RuntimeState->CurrentStateID = NextState->StateID;
		OnEvidenceStateChanged.Broadcast(
			EvidenceInstanceID,
			PreviousStateID,
			RuntimeState->CurrentStateID);
		return true;
	}

	case EEvidenceInteractionBehavior::None:
	default:
		return false;
	}
}

bool UBalhwajeomInvestigationSubsystem::AcquireWord(
	FName WordID,
	EWordAcquisitionSource SourceType,
	FName SourceID)
{
	if (AcquiredWords.Contains(WordID))
	{
		return false;
	}

	if (FindWordDefinition(WordID) == nullptr)
	{
		return false;
	}

	FAcquiredWordRecord Record;
	Record.WordID = WordID;
	Record.SourceType = SourceType;
	Record.SourceID = SourceID;
	Record.AcquiredTime = FDateTime::UtcNow();
	AcquiredWords.Add(WordID, Record);
	OnWordAcquired.Broadcast(Record);
	return true;
}

bool UBalhwajeomInvestigationSubsystem::HasAcquiredWord(FName WordID) const
{
	return AcquiredWords.Contains(WordID);
}

void UBalhwajeomInvestigationSubsystem::GetAcquiredWords(
	TArray<FAcquiredWordRecord>& OutWords) const
{
	AcquiredWords.GenerateValueArray(OutWords);
	OutWords.Sort([](const FAcquiredWordRecord& A, const FAcquiredWordRecord& B)
	{
		return A.AcquiredTime == B.AcquiredTime
			? A.WordID.LexicalLess(B.WordID)
			: A.AcquiredTime < B.AcquiredTime;
	});
}

void UBalhwajeomInvestigationSubsystem::GetAcquiredWordsForCharacter(
	const FName CharacterID,
	TArray<FAcquiredWordRecord>& OutWords) const
{
	OutWords.Reset();
	if (CharacterID.IsNone())
	{
		return;
	}

	for (const TPair<FName, FAcquiredWordRecord>& Pair : AcquiredWords)
	{
		const FWordDefinition* Definition = FindWordDefinition(Pair.Key);
		if (Definition && Definition->RelatedCharacterIDs.Contains(CharacterID))
		{
			OutWords.Add(Pair.Value);
		}
	}

	OutWords.Sort([](const FAcquiredWordRecord& A, const FAcquiredWordRecord& B)
	{
		return A.AcquiredTime == B.AcquiredTime
			? A.WordID.LexicalLess(B.WordID)
			: A.AcquiredTime < B.AcquiredTime;
	});
}

bool UBalhwajeomInvestigationSubsystem::HasCapturedPhoto(FName PhotoID) const
{
	return CapturedPhotos.Contains(PhotoID);
}

bool UBalhwajeomInvestigationSubsystem::RegisterCapturedPhoto(const FCapturedPhotoRecord& Record)
{
	if (Record.PhotoID.IsNone() ||
		!Record.EvidenceInstanceID.IsValid() ||
		CapturedPhotos.Contains(Record.PhotoID))
	{
		return false;
	}

	const FPhotoDefinition* PhotoDefinition = FindPhotoDefinition(Record.PhotoID);
	if (PhotoDefinition == nullptr)
	{
		return false;
	}

	const FEvidenceRuntimeState* RuntimeState =
		EvidenceRuntimeStates.Find(Record.EvidenceInstanceID);
	if (RuntimeState == nullptr ||
		RuntimeState->ObjectID != Record.ObjectID ||
		RuntimeState->CurrentStateID != Record.CapturedStateID)
	{
		return false;
	}

	const FEvidenceStateDefinition* CapturedState =
		FindEvidenceStateDefinition(Record.CapturedStateID);
	if (CapturedState == nullptr ||
		CapturedState->ObjectID != Record.ObjectID ||
		CapturedState->PhotoID != Record.PhotoID ||
		!CapturedState->bCanCapture)
	{
		return false;
	}

	FCapturedPhotoRecord StoredRecord = Record;
	if (StoredRecord.CapturedTime.GetTicks() == 0)
	{
		StoredRecord.CapturedTime = FDateTime::UtcNow();
	}

	CapturedPhotos.Add(StoredRecord.PhotoID, StoredRecord);
	if (!SavePersistentPhotoGallery())
	{
		CapturedPhotos.Remove(StoredRecord.PhotoID);
		UE_LOG(LogBalhwajeomInvestigation, Error,
			TEXT("Failed to persist captured photo '%s'."),
			*StoredRecord.PhotoID.ToString());
		return false;
	}
	for (const FName WordID : PhotoDefinition->GrantedWordIDs)
	{
		AcquireWord(WordID, EWordAcquisitionSource::PhotoCapture, StoredRecord.PhotoID);
	}
	OnPhotoCaptured.Broadcast(StoredRecord);
	return true;
}

void UBalhwajeomInvestigationSubsystem::GetCapturedPhotos(
	TArray<FCapturedPhotoRecord>& OutPhotos) const
{
	CapturedPhotos.GenerateValueArray(OutPhotos);
	OutPhotos.Sort([](const FCapturedPhotoRecord& A, const FCapturedPhotoRecord& B)
	{
		return A.CapturedTime == B.CapturedTime
			? A.PhotoID.LexicalLess(B.PhotoID)
			: A.CapturedTime < B.CapturedTime;
	});
}

bool UBalhwajeomInvestigationSubsystem::GetCapturedPhoto(
	FName PhotoID,
	FCapturedPhotoRecord& OutRecord) const
{
	OutRecord = FCapturedPhotoRecord{};
	const FCapturedPhotoRecord* Record = CapturedPhotos.Find(PhotoID);
	if (Record == nullptr)
	{
		return false;
	}
	OutRecord = *Record;
	return true;
}

bool UBalhwajeomInvestigationSubsystem::ValidateSentence(
	FName SentenceID,
	const FSentenceSubmission& Submission,
	FText& OutResultText)
{
	OutResultText = FText::GetEmpty();

	const FSentenceDefinition* Sentence = FindSentenceDefinition(SentenceID);
	if (Sentence == nullptr)
	{
		return false;
	}

	FSentenceRuntimeProgress& Progress = SentenceProgress.FindOrAdd(SentenceID);
	const bool bWasAlreadySolved = Progress.bSolved;
	Progress.SentenceID = SentenceID;
	Progress.SelectedWords = Submission.SubmittedWords;
	Progress.SelectedPhotos = Submission.SubmittedPhotos;

	TMap<int32, TArray<const FSentenceWordSlot*>> SlotsByGroup;
	for (const FSentenceWordSlot& CorrectSlot : Sentence->WordSlots)
	{
		SlotsByGroup.FindOrAdd(CorrectSlot.OrderGroup).Add(&CorrectSlot);
	}

	for (const TPair<int32, TArray<const FSentenceWordSlot*>>& GroupPair : SlotsByGroup)
	{
		if (GroupPair.Key == 0)
		{
			for (const FSentenceWordSlot* CorrectSlot : GroupPair.Value)
			{
				const FSubmittedWordSlot* SubmittedSlot = Submission.SubmittedWords.FindByPredicate(
					[CorrectSlot](const FSubmittedWordSlot& Candidate)
					{
						return Candidate.SlotIndex == CorrectSlot->SlotIndex;
					});

				if (SubmittedSlot == nullptr ||
					SubmittedSlot->WordID != CorrectSlot->CorrectWordID ||
					!AcquiredWords.Contains(SubmittedSlot->WordID))
				{
					return false;
				}
			}
			continue;
		}

		TSet<int32> GroupSlotIndices;
		TMap<FName, int32> RemainingRequiredCounts;
		for (const FSentenceWordSlot* CorrectSlot : GroupPair.Value)
		{
			GroupSlotIndices.Add(CorrectSlot->SlotIndex);
			++RemainingRequiredCounts.FindOrAdd(CorrectSlot->CorrectWordID);
		}

		int32 SubmittedCountInGroup = 0;
		for (const FSubmittedWordSlot& SubmittedSlot : Submission.SubmittedWords)
		{
			if (!GroupSlotIndices.Contains(SubmittedSlot.SlotIndex))
			{
				continue;
			}
			++SubmittedCountInGroup;

			int32* RemainingCount = RemainingRequiredCounts.Find(SubmittedSlot.WordID);
			if (RemainingCount == nullptr || *RemainingCount <= 0 ||
				!AcquiredWords.Contains(SubmittedSlot.WordID))
			{
				return false;
			}
			--(*RemainingCount);
		}

		if (SubmittedCountInGroup != GroupPair.Value.Num())
		{
			return false;
		}
	}

	int32 CorrectPhotoCount = 0;
	for (const FSentencePhotoSlot& CorrectSlot : Sentence->PhotoSlots)
	{
		const FSubmittedPhotoSlot* SubmittedSlot = Submission.SubmittedPhotos.FindByPredicate(
			[&CorrectSlot](const FSubmittedPhotoSlot& Candidate)
			{
				return Candidate.SlotIndex == CorrectSlot.SlotIndex;
			});

		if (SubmittedSlot == nullptr || !CapturedPhotos.Contains(SubmittedSlot->PhotoID))
		{
			continue;
		}

		// Photo evidence must have completed its own analysis sentence; a photo that is
		// merely captured (or has no analysis sentence, e.g. a story photo) is not eligible.
		const FPhotoDefinition* SubmittedPhoto = FindPhotoDefinition(SubmittedSlot->PhotoID);
		const FSentenceRuntimeProgress* AnalysisProgress =
			SubmittedPhoto != nullptr ? SentenceProgress.Find(SubmittedPhoto->PhotoSentenceID) : nullptr;

		if (AnalysisProgress == nullptr || !AnalysisProgress->bSolved)
		{
			continue;
		}

		if (SubmittedSlot->PhotoID != CorrectSlot.CorrectPhotoID)
		{
			// Wrong-but-completed evidence photo: surface that photo's own declaration sentence
			// (EvidenceSentenceID) so the player learns why it can't be used here, instead of a
			// generic failure. This does not solve the active Statement.
			if (SubmittedPhoto != nullptr && !SubmittedPhoto->EvidenceSentenceID.IsNone())
			{
				if (const FSentenceDefinition* Declaration =
						FindSentenceDefinition(SubmittedPhoto->EvidenceSentenceID))
				{
					FSentenceRuntimeProgress& DeclarationProgress =
						SentenceProgress.FindOrAdd(Declaration->SentenceID);
					DeclarationProgress.SentenceID = Declaration->SentenceID;
					DeclarationProgress.bSolved = true;
					OutResultText = Declaration->ResultText;
				}
			}
			return false;
		}

		++CorrectPhotoCount;
	}

	if (CorrectPhotoCount < Sentence->RequiredPhotoCount)
	{
		return false;
	}

	if (Sentence->ResultText.IsEmpty())
	{
		return false;
	}

	Progress.bSolved = true;

	OutResultText = Sentence->ResultText;
	if (!bWasAlreadySolved)
	{
		OnSentenceSolved.Broadcast(SentenceID);
	}
	return true;
}

bool UBalhwajeomInvestigationSubsystem::IsSentenceSolved(FName SentenceID) const
{
	const FSentenceRuntimeProgress* Progress = SentenceProgress.Find(SentenceID);
	return Progress != nullptr && Progress->bSolved;
}

void UBalhwajeomInvestigationSubsystem::GetStatementSentencesForCharacter(
	FName CharacterID,
	TArray<FSentenceDefinition>& OutSentences) const
{
	OutSentences.Reset();
	if (!IsValid(SentencesTable) || SentencesTable->GetRowStruct() != FSentenceDefinition::StaticStruct())
	{
		return;
	}
	for (const TPair<FName, uint8*>& Pair : SentencesTable->GetRowMap())
	{
		const FSentenceDefinition* Sentence = reinterpret_cast<const FSentenceDefinition*>(Pair.Value);
		if (Sentence->SentenceType == ESentenceType::Statement && Sentence->CharacterID == CharacterID &&
			Sentence->bIsFolderStatement)
		{
			OutSentences.Add(*Sentence);
		}
	}
	OutSentences.Sort([](const FSentenceDefinition& A, const FSentenceDefinition& B)
	{
		return A.SentenceID.LexicalLess(B.SentenceID);
	});
}

void UBalhwajeomInvestigationSubsystem::GetPhotosForCharacter(
	FName CharacterID,
	TArray<FPhotoDefinition>& OutPhotos) const
{
	OutPhotos.Reset();
	if (!IsValid(PhotosTable) || PhotosTable->GetRowStruct() != FPhotoDefinition::StaticStruct())
	{
		return;
	}
	for (const TPair<FName, uint8*>& Pair : PhotosTable->GetRowMap())
	{
		const FPhotoDefinition* Photo = reinterpret_cast<const FPhotoDefinition*>(Pair.Value);
		if (Photo->CharacterID == CharacterID)
		{
			OutPhotos.Add(*Photo);
		}
	}
	OutPhotos.Sort([](const FPhotoDefinition& A, const FPhotoDefinition& B)
	{
		return A.PhotoID.LexicalLess(B.PhotoID);
	});
}

void UBalhwajeomInvestigationSubsystem::GetAllCharacterDefinitions(
	TArray<FCharacterDefinition>& OutCharacters) const
{
	OutCharacters.Reset();
	if (!IsValid(CharactersTable) || CharactersTable->GetRowStruct() != FCharacterDefinition::StaticStruct())
	{
		return;
	}
	for (const TPair<FName, uint8*>& Pair : CharactersTable->GetRowMap())
	{
		const FCharacterDefinition* Character = reinterpret_cast<const FCharacterDefinition*>(Pair.Value);
		OutCharacters.Add(*Character);
	}
	OutCharacters.Sort([](const FCharacterDefinition& A, const FCharacterDefinition& B)
	{
		return A.FolderSortOrder == B.FolderSortOrder
			? A.CharacterID.LexicalLess(B.CharacterID)
			: A.FolderSortOrder < B.FolderSortOrder;
	});
}
