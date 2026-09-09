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

	/** The character whose tablet folder this photo is displayed under. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FName CharacterID = NAME_None;

	/** Optional extra fill-in-blank sentence (usually SentenceType=PhotoAnalysis) that must also be
	 * solved before this photo counts as valid evidence in any statement's photo slot. Empty = none required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FName EvidenceSentenceID = NAME_None;

	/** Keywords granted once when this photo is successfully captured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	TArray<FName> GrantedWordIDs;

	/** World-locked 3D story lines shown after capture (and again when reopening the photo from the tablet), in order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	TArray<FText> WorldStoryLines;

	/** Narration voice played alongside WorldStoryLines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Audio")
	TSoftObjectPtr<USoundBase> StoryVoice;
};
