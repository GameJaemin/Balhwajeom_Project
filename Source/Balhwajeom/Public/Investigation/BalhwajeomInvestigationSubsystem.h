#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Investigation/CharacterDefinitions.h"
#include "Investigation/EvidenceDefinitions.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Investigation/PhotoDefinitions.h"
#include "Investigation/SentenceDefinitions.h"
#include "Investigation/WordDefinitions.h"
#include "BalhwajeomInvestigationSubsystem.generated.h"

class UDataTable;
struct FInvestigationSubsystemTestAccessor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnEvidenceStateChanged,
	FGuid, EvidenceInstanceID,
	FName, PreviousStateID,
	FName, NewStateID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWordAcquired,
	const FAcquiredWordRecord&, WordRecord);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnPhotoCaptured,
	const FCapturedPhotoRecord&, PhotoRecord);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSentenceSolved,
	FName, SentenceID);

/** Central access point for investigation definitions and mutable play-session state. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomInvestigationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
	bool GetEvidenceDefinition(FName ObjectID, FEvidenceDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
	bool GetEvidenceStateDefinition(FName StateID, FEvidenceStateDefinition& OutState) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
	bool GetPhotoDefinition(FName PhotoID, FPhotoDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
	bool GetCharacterDefinition(FName CharacterID, FCharacterDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
	bool GetWordDefinition(FName WordID, FWordDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Definitions")
	bool GetSentenceDefinition(FName SentenceID, FSentenceDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Keyword Documents")
	bool GetKeywordDocumentDefinition(FName KeywordDocumentID, FKeywordDocumentDefinition& OutDefinition) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Keyword Documents")
	void GetKeywordChoicesForDocument(FName KeywordDocumentID, TArray<FKeywordChoiceDefinition>& OutChoices) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Keyword Documents")
	bool SelectKeywordChoice(FName KeywordDocumentID, FName ChoiceID, EWordAcquisitionSource SourceType);

	UFUNCTION(BlueprintCallable, Category = "Investigation|Evidence")
	bool RegisterEvidenceActor(
		FGuid EvidenceInstanceID,
		FName ObjectID,
		FName& OutCurrentStateID);

	UFUNCTION(BlueprintCallable, Category = "Investigation|Interaction")
	bool BeginEvidenceInteraction(
		FGuid EvidenceInstanceID,
		FEvidenceInteractionViewData& OutViewData);

	UFUNCTION(BlueprintCallable, Category = "Investigation|Interaction")
	bool CompleteEvidenceInteraction(
		FGuid EvidenceInstanceID,
		FName ExpectedStateID);

	UFUNCTION(BlueprintCallable, Category = "Investigation|Words")
	bool AcquireWord(FName WordID, EWordAcquisitionSource SourceType, FName SourceID);

	UFUNCTION(BlueprintPure, Category = "Investigation|Words")
	bool HasAcquiredWord(FName WordID) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Words")
	void GetAcquiredWords(TArray<FAcquiredWordRecord>& OutWords) const;

	UFUNCTION(BlueprintPure, Category = "Investigation|Photos")
	bool HasCapturedPhoto(FName PhotoID) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Photos")
	bool RegisterCapturedPhoto(const FCapturedPhotoRecord& Record);

	UFUNCTION(BlueprintCallable, Category = "Investigation|Photos")
	void GetCapturedPhotos(TArray<FCapturedPhotoRecord>& OutPhotos) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Sentences")
	bool ValidateSentence(
		FName SentenceID,
		const FSentenceSubmission& Submission,
		FText& OutResultText);

	UFUNCTION(BlueprintPure, Category = "Investigation|Sentences")
	bool IsSentenceSolved(FName SentenceID) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Folders")
	void GetStatementSentencesForCharacter(FName CharacterID, TArray<FSentenceDefinition>& OutSentences) const;

	UFUNCTION(BlueprintCallable, Category = "Investigation|Folders")
	void GetPhotosForCharacter(FName CharacterID, TArray<FPhotoDefinition>& OutPhotos) const;

	UPROPERTY(BlueprintAssignable, Category = "Investigation|Events")
	FOnEvidenceStateChanged OnEvidenceStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Investigation|Events")
	FOnWordAcquired OnWordAcquired;

	UPROPERTY(BlueprintAssignable, Category = "Investigation|Events")
	FOnPhotoCaptured OnPhotoCaptured;

	UPROPERTY(BlueprintAssignable, Category = "Investigation|Events")
	FOnSentenceSolved OnSentenceSolved;

private:
	friend struct FInvestigationSubsystemTestAccessor;

	void LoadConfiguredDataTables();
	void ClearLoadedDataTables();
	void InitializeDefaultWords();
	void LoadPersistentPhotoGallery();
	bool SavePersistentPhotoGallery() const;
	bool ShouldPersistPhotoGallery() const;
	bool ValidateLoadedDataTables() const;

	const FEvidenceDefinition* FindEvidenceDefinition(FName ObjectID) const;
	const FEvidenceStateDefinition* FindEvidenceStateDefinition(FName StateID) const;
	const FWordDefinition* FindWordDefinition(FName WordID) const;
	const FPhotoDefinition* FindPhotoDefinition(FName PhotoID) const;
	const FKeywordDocumentDefinition* FindKeywordDocumentDefinition(FName KeywordDocumentID) const;
	const FSentenceDefinition* FindSentenceDefinition(FName SentenceID) const;
	const FCharacterDefinition* FindCharacterDefinition(FName CharacterID) const;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> EvidenceDefinitionsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> EvidenceStatesTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> WordsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> PhotosTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> KeywordDocumentsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> KeywordChoicesTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> SentencesTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CharactersTable;

	UPROPERTY(Transient)
	TMap<FGuid, FEvidenceRuntimeState> EvidenceRuntimeStates;

	UPROPERTY(Transient)
	TMap<FName, FAcquiredWordRecord> AcquiredWords;

	UPROPERTY(Transient)
	TMap<FName, FCapturedPhotoRecord> CapturedPhotos;

	UPROPERTY(Transient)
	TMap<FName, FKeywordDocumentRuntimeState> KeywordDocumentStates;

	UPROPERTY(Transient)
	TMap<FName, FSentenceRuntimeProgress> SentenceProgress;
};
