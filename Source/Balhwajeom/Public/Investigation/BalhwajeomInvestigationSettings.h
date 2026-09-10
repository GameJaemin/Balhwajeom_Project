#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BalhwajeomInvestigationSettings.generated.h"

class UDataTable;

/** Project-wide DataTable references used by the investigation system. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Investigation"))
class BALHWAJEOM_API UBalhwajeomInvestigationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> EvidenceDefinitionsTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> EvidenceStatesTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> WordsTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> PhotosTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> KeywordDocumentsTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> KeywordChoicesTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> SentencesTable;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> CharactersTable;
};
