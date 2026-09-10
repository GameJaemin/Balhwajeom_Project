#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "BalhwajeomInvestigationSaveGame.generated.h"

/** Durable metadata for photographs stored in Saved/Investigation/Photos. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomInvestigationSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int32 DataVersion = 1;

	UPROPERTY(SaveGame)
	TArray<FCapturedPhotoRecord> CapturedPhotos;
};
