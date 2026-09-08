#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WordDefinitions.generated.h"

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FWordDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Word")
	FName WordID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Word")
	FText DisplayWord;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Word", meta = (MultiLine = "true"))
	FText Description;

	/** Character folders where this word is visible and can be used as a statement candidate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Word")
	TArray<FName> RelatedCharacterIDs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Word")
	bool bUnlockedByDefault = false;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FKeywordChoiceDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FName ChoiceID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FName KeywordDocumentID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FName GrantedWordID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FKeywordDocumentDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Document")
	FName KeywordDocumentID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Document", meta = (MultiLine = "true"))
	FText DocumentText;

	/** Migration-only copy of the pre-redesign nested choices. New content belongs in DT_KeywordChoices. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use DT_KeywordChoices."))
	TArray<FKeywordChoiceDefinition> KeywordChoices;

	/** Retained only so existing DataTable assets deserialize during the one-time migration. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Keyword documents remain open in the final prototype."))
	bool bCloseAfterSelection = false;

};
