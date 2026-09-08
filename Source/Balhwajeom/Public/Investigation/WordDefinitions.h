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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Word")
	bool bUnlockedByDefault = false;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FKeywordChoiceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FName ChoiceID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Choice")
	FName GrantedWordID = NAME_None;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FKeywordDocumentDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Document")
	FName KeywordDocumentID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Document", meta = (MultiLine = "true"))
	FText DocumentText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Document")
	TArray<FKeywordChoiceDefinition> KeywordChoices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keyword Document")
	bool bCloseAfterSelection = true;
};
