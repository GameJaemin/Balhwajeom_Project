#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo")
	FGameplayTagContainer PhotoTags;

	/** Optional sound played only when this evidence photo is registered for the first time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo|Audio")
	TSoftObjectPtr<USoundBase> CaptureSound;
};
