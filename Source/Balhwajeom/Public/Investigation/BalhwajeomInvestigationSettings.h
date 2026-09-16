#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "BalhwajeomInvestigationSettings.generated.h"

class UDataTable;

/** Project-wide DataTable references used by the investigation system. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Investigation"))
class BALHWAJEOM_API UBalhwajeomInvestigationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	/** Applies the centrally configured chapter 01 phase gate for an ObjectID. */
	UFUNCTION(BlueprintPure, Category = "Investigation|Chapter 01 Phase Progression")
	FGameplayTag ResolveChapter01RequiredActivationTag(
		FName ObjectID,
		FGameplayTag AuthoredActivationTag) const;

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

	/** Global typewriter speed shared by every DT_Photos WorldStoryCue. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Photo World Story",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float WorldStoryCharactersPerSecond = 20.0f;

	/** Enables chapter 01 phase completion, obstacle unlocks, and ObjectID activation gates. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Chapter 01 Phase Progression")
	bool bEnableChapter01PhaseSystem = true;

	/** Evidence that becomes interactable after the room 2 tutorial is complete. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Chapter 01 Phase Progression",
		meta = (EditCondition = "bEnableChapter01PhaseSystem"))
	TArray<FName> Phase01ObjectIDs;

	/** Evidence that becomes interactable after the phase 01 obstacle is cleared. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Chapter 01 Phase Progression",
		meta = (EditCondition = "bEnableChapter01PhaseSystem"))
	TArray<FName> Phase02ObjectIDs;

	/** Evidence that becomes interactable after the phase 02 obstacle is cleared. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Chapter 01 Phase Progression",
		meta = (EditCondition = "bEnableChapter01PhaseSystem"))
	TArray<FName> Phase03ObjectIDs;
};
