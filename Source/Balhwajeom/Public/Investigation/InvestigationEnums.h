#pragma once

#include "CoreMinimal.h"
#include "InvestigationEnums.generated.h"

UENUM(BlueprintType)
enum class EEvidenceInteractionBehavior : uint8
{
	None,
	Once,
	Repeatable,
	ChangeState
};

UENUM(BlueprintType)
enum class EEvidenceInteractionPresentation : uint8
{
	None,
	SimpleText,
	KeywordSelectionWindow,
	/** Spawns the photo's WorldStoryCues as world-locked 3D text at the actor's StoryAnchor. */
	WorldStory
};

UENUM(BlueprintType)
enum class EPhotoDescriptionSource : uint8
{
	None,
	NearLabel,
	InteractionText,
	Custom
};

UENUM(BlueprintType)
enum class ESentenceType : uint8
{
	PhotoAnalysis,
	Statement
};

UENUM(BlueprintType)
enum class EWordAcquisitionSource : uint8
{
	Default,
	EvidenceInteraction,
	PhotoCapture,
	Browser,
	Messenger
};
