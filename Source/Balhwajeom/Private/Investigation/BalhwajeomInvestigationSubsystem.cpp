#include "Investigation/BalhwajeomInvestigationSubsystem.h"

#include "Engine/DataTable.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogBalhwajeomInvestigation, Log, All);

namespace
{
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
	SentencesTable = LoadTable(Settings->SentencesTable, TEXT("SentencesTable"));
	OutputTextsTable = LoadTable(Settings->OutputTextsTable, TEXT("OutputTextsTable"));
}

void UBalhwajeomInvestigationSubsystem::ClearLoadedDataTables()
{
	EvidenceDefinitionsTable = nullptr;
	EvidenceStatesTable = nullptr;
	WordsTable = nullptr;
	PhotosTable = nullptr;
	KeywordDocumentsTable = nullptr;
	SentencesTable = nullptr;
	OutputTextsTable = nullptr;
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
	bIsValid &= ValidateTableRowIDs<FSentenceDefinition>(
		SentencesTable,
		TEXT("SentencesTable"),
		[](const FSentenceDefinition& Row) { return Row.SentenceID; });
	bIsValid &= ValidateTableRowIDs<FOutputTextDefinition>(
		OutputTextsTable,
		TEXT("OutputTextsTable"),
		[](const FOutputTextDefinition& Row) { return Row.TextID; });

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
	auto HasSentence = [this, &Context](FName SentenceID)
	{
		return !SentenceID.IsNone() &&
			SentencesTable->FindRow<FSentenceDefinition>(SentenceID, Context, false) != nullptr;
	};
	auto HasOutputText = [this, &Context](FName TextID)
	{
		return !TextID.IsNone() &&
			OutputTextsTable->FindRow<FOutputTextDefinition>(TextID, Context, false) != nullptr;
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
		if (!Photo->PhotoSentenceID.IsNone() && !HasSentence(Photo->PhotoSentenceID))
		{
			ReportInvalidReference(
				TEXT("PhotoDefinition"),
				Photo->PhotoID,
				TEXT("PhotoSentenceID"),
				Photo->PhotoSentenceID);
		}
	}

	for (const TPair<FName, uint8*>& Pair : KeywordDocumentsTable->GetRowMap())
	{
		const FKeywordDocumentDefinition* Document =
			reinterpret_cast<const FKeywordDocumentDefinition*>(Pair.Value);
		TSet<FName> ChoiceIDs;
		for (const FKeywordChoiceDefinition& Choice : Document->KeywordChoices)
		{
			if (Choice.ChoiceID.IsNone() || ChoiceIDs.Contains(Choice.ChoiceID))
			{
				ReportInvalidReference(
					TEXT("KeywordDocument"),
					Document->KeywordDocumentID,
					TEXT("ChoiceID"),
					Choice.ChoiceID);
			}
			ChoiceIDs.Add(Choice.ChoiceID);

			if (!HasWord(Choice.GrantedWordID))
			{
				ReportInvalidReference(
					TEXT("KeywordDocument"),
					Document->KeywordDocumentID,
					TEXT("GrantedWordID"),
					Choice.GrantedWordID);
			}
		}
	}

	for (const TPair<FName, uint8*>& Pair : SentencesTable->GetRowMap())
	{
		const FSentenceDefinition* Sentence =
			reinterpret_cast<const FSentenceDefinition*>(Pair.Value);
		TSet<int32> WordSlotIndices;
		for (const FSentenceWordSlot& Slot : Sentence->WordSlots)
		{
			if (Slot.SlotIndex < 0 || Slot.SlotIndex > 4 || WordSlotIndices.Contains(Slot.SlotIndex))
			{
				ReportInvalidReference(
					TEXT("Sentence"), Sentence->SentenceID, TEXT("WordSlotIndex"), FName(*FString::FromInt(Slot.SlotIndex)));
			}
			WordSlotIndices.Add(Slot.SlotIndex);
			if (!HasWord(Slot.CorrectWordID))
			{
				ReportInvalidReference(
					TEXT("Sentence"), Sentence->SentenceID, TEXT("CorrectWordID"), Slot.CorrectWordID);
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

		if (!Sentence->ResultTextID.IsNone() && !HasOutputText(Sentence->ResultTextID))
		{
			ReportInvalidReference(
				TEXT("Sentence"), Sentence->SentenceID, TEXT("ResultTextID"), Sentence->ResultTextID);
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

const FOutputTextDefinition* UBalhwajeomInvestigationSubsystem::FindOutputTextDefinition(
	FName TextID) const
{
	return FindInvestigationRow<FOutputTextDefinition>(
		OutputTextsTable,
		TextID,
		TEXT("OutputTextsTable"));
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

	if (FindPhotoDefinition(Record.PhotoID) == nullptr)
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
	OnPhotoCaptured.Broadcast(StoredRecord);
	return true;
}

bool UBalhwajeomInvestigationSubsystem::ValidateSentence(
	FName SentenceID,
	const FSentenceSubmission& Submission,
	FName& OutResultTextID)
{
	OutResultTextID = NAME_None;

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

	for (const FSentenceWordSlot& CorrectSlot : Sentence->WordSlots)
	{
		const FSubmittedWordSlot* SubmittedSlot = Submission.SubmittedWords.FindByPredicate(
			[&CorrectSlot](const FSubmittedWordSlot& Candidate)
			{
				return Candidate.SlotIndex == CorrectSlot.SlotIndex;
			});

		if (SubmittedSlot == nullptr ||
			SubmittedSlot->WordID != CorrectSlot.CorrectWordID ||
			!AcquiredWords.Contains(SubmittedSlot->WordID))
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

		if (SubmittedSlot != nullptr &&
			SubmittedSlot->PhotoID == CorrectSlot.CorrectPhotoID &&
			CapturedPhotos.Contains(SubmittedSlot->PhotoID))
		{
			++CorrectPhotoCount;
		}
	}

	if (CorrectPhotoCount < Sentence->RequiredPhotoCount)
	{
		return false;
	}

	if (!Sentence->ResultTextID.IsNone() &&
		FindOutputTextDefinition(Sentence->ResultTextID) == nullptr)
	{
		return false;
	}

	Progress.bSolved = true;

	OutResultTextID = Sentence->ResultTextID;
	if (!bWasAlreadySolved)
	{
		OnSentenceSolved.Broadcast(SentenceID, OutResultTextID);
	}
	return true;
}
