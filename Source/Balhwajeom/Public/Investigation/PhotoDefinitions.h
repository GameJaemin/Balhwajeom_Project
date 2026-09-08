#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Investigation/InvestigationEnums.h"
#include "PhotoDefinitions.generated.h"

class USoundBase;

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FPhotoDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FName PhotoID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FText PhotoName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	EPhotoDescriptionSource DescriptionSource = EPhotoDescriptionSource::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo", meta = (MultiLine = "true"))
	FText CustomDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FName PhotoSentenceID = NAME_None;

	/** The Statement-type sentence whose CharacterID/FolderName this photo is displayed under in the tablet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FName StatementSentenceID = NAME_None;

	/** Keywords granted once when this photo is successfully captured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	TArray<FName> GrantedWordIDs;

	/** World-locked 3D story text shown after capture, and again when reopening the photo from the tablet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo", meta = (MultiLine = "true"))
	FText WorldStoryText;

	/** Narration voice played alongside WorldStoryText. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Audio")
	TSoftObjectPtr<USoundBase> StoryVoice;
};
