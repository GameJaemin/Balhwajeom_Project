#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Investigation/InvestigationEnums.h"
#include "PhotoDefinitions.generated.h"

class USoundBase;

/** One world-story caption and the time at which it replaces the previous caption. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FPhotoStoryCue
{
	GENERATED_BODY()

	/** Text displayed for this cue. Embedded newlines and whitespace are preserved; empty text can intentionally clear the caption. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Story", meta = (MultiLine = "true"))
	FText Text;

	/** Seconds from the start of StoryVoice. The first cue must start at zero. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Story",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float StartTimeSeconds = 0.0f;
};

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

	/** Declaration sentence (SentenceType=Statement) explaining whether this specific completed photo
	 * is valid evidence. When this photo is submitted to a statement's photo slot but isn't the
	 * correct evidence, this sentence's ResultText is shown to explain why -- and this sentence
	 * itself is marked solved. Empty = no per-photo explanation is shown on a wrong submission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FName EvidenceSentenceID = NAME_None;

	/** Keywords granted once when this photo is successfully captured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	TArray<FName> GrantedWordIDs;

	/** Legacy untimed story lines. Kept while existing DT_Photos assets migrate to WorldStoryCues. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	TArray<FText> WorldStoryLines;

	/** Timed world-locked captions shown after capture, in ascending StartTimeSeconds order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Story")
	TArray<FPhotoStoryCue> WorldStoryCues;

	/** Optional narration voice played alongside WorldStoryCues. Cues also play as a timed text-only story when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Audio")
	TSoftObjectPtr<USoundBase> StoryVoice;
};
