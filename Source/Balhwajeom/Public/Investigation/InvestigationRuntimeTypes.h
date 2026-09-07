#pragma once

#include "CoreMinimal.h"
#include "Investigation/InvestigationEnums.h"
#include "InvestigationRuntimeTypes.generated.h"

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FEvidenceRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Evidence")
	FGuid EvidenceInstanceID;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Evidence")
	FName ObjectID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Evidence")
	FName CurrentStateID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Evidence")
	TSet<FName> CompletedInteractionStateIDs;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FAcquiredWordRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Word")
	FName WordID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Word")
	EWordAcquisitionSource SourceType = EWordAcquisitionSource::Default;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Word")
	FName SourceID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Word")
	FDateTime AcquiredTime;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FCapturedPhotoRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	FName PhotoID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	FName ObjectID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	FGuid EvidenceInstanceID;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	FName CapturedStateID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	FString ImageRelativePath;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	FDateTime CapturedTime;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Photo")
	bool bViewedInTablet = false;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FEvidenceInteractionViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	FGuid EvidenceInstanceID;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	FName ObjectID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	FName StateID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	EEvidenceInteractionBehavior Behavior = EEvidenceInteractionBehavior::None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	EEvidenceInteractionPresentation Presentation = EEvidenceInteractionPresentation::None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	FText InteractionText;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Interaction")
	FName KeywordDocumentID = NAME_None;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSubmittedWordSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation|Sentence")
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation|Sentence")
	FName WordID = NAME_None;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSubmittedPhotoSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation|Sentence")
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation|Sentence")
	FName PhotoID = NAME_None;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSentenceSubmission
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation|Sentence")
	TArray<FSubmittedWordSlot> SubmittedWords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigation|Sentence")
	TArray<FSubmittedPhotoSlot> SubmittedPhotos;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FKeywordDocumentRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Keyword Document")
	FName KeywordDocumentID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Keyword Document")
	TSet<FName> SelectedChoiceIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Keyword Document")
	bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSentenceRuntimeProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Sentence")
	FName SentenceID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Sentence")
	TArray<FSubmittedWordSlot> SelectedWords;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Sentence")
	TArray<FSubmittedPhotoSlot> SelectedPhotos;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Sentence")
	bool bSolved = false;

	UPROPERTY(BlueprintReadOnly, Category = "Investigation|Sentence")
	bool bResultViewed = false;
};
