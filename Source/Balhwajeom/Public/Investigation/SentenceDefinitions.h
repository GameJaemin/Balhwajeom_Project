#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Investigation/InvestigationEnums.h"
#include "SentenceDefinitions.generated.h"

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSentenceWordSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (ClampMin = "0", ClampMax = "4"))
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	FName CorrectWordID = NAME_None;

	/** 0 = this slot's position is fixed. Slots sharing the same non-zero group may be filled in any order among themselves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	int32 OrderGroup = 0;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSentencePhotoSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (ClampMin = "0", ClampMax = "1"))
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	FName CorrectPhotoID = NAME_None;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FSentenceDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	FName SentenceID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	ESentenceType SentenceType = ESentenceType::PhotoAnalysis;

	/** The person this Statement-type sentence belongs to, for tablet folder placement. Empty for PhotoAnalysis rows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	FName CharacterID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (MultiLine = "true"))
	FText SentenceTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	TArray<FSentenceWordSlot> WordSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	TArray<FSentencePhotoSlot> PhotoSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (ClampMin = "0", ClampMax = "2"))
	int32 RequiredPhotoCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence")
	FName ResultTextID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (MultiLine = "true"))
	FText ResultText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sentence", meta = (MultiLine = "true"))
	FText DesignerNote;
};
