#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "TutorialOverlayDefinitions.generated.h"


class UTexture2D;


/** Data-driven content and one-shot gating for a full-screen tutorial overlay. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FTutorialOverlayDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Kept identical to the Row Name, as in every other investigation DataTable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Overlay")
	FName OverlayID = NAME_None;

	/** Every tag in this container must be present before the overlay may open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Overlay|Condition")
	FGameplayTagContainer RequiredTags;

	/** Optional images shown by the overlay in authored order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Overlay|Content")
	TArray<TSoftObjectPtr<UTexture2D>> Images;

	/** Single-line heading above the explanation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Overlay|Content")
	FText OverlayTitle;

	/** Player-facing tutorial copy. Embedded newlines are preserved. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Overlay|Content",
		meta = (MultiLine = "true"))
	FText OverlayText;

	/** Added to StoryState when the player closes the overlay to prevent it reopening. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial Overlay|Condition")
	FGameplayTag CompletionTag;
};
