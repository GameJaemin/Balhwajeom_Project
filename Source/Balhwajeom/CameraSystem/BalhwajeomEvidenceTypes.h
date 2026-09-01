// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomEvidenceTypes.generated.h"

/** Designer-authored data stored when an evidence photo succeeds. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FBalhwajeomEvidenceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FName EvidenceID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FText EvidenceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
	bool bAlreadyCollected = false;
};
