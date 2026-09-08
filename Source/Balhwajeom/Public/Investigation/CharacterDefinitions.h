#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CharacterDefinitions.generated.h"

/** A person with their own tablet folder (name, sort order). Puzzle content lives on FSentenceDefinition/FPhotoDefinition, which reference this by CharacterID. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FCharacterDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FName CharacterID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	FText FolderName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	int32 FolderSortOrder = 0;
};
