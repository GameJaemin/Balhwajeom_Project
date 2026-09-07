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
	KeywordSelectionWindow
};

UENUM(BlueprintType)
enum class EPhotoDescriptionSource : uint8
{
	None,
	ObservationText,
	InteractionText,
	Custom
};

UENUM(BlueprintType)
enum class EWordCategory : uint8
{
	Object,
	State,
	Target,
	Action,
	Place,
	Time,
	Etc
};

UENUM(BlueprintType)
enum class ESentenceType : uint8
{
	PhotoAnalysis,
	Statement
};

UENUM(BlueprintType)
enum class EOutputTextType : uint8
{
	Answer,
	Statement,
	Dialogue,
	Etc
};

UENUM(BlueprintType)
enum class EWordAcquisitionSource : uint8
{
	Default,
	EvidenceInteraction,
	Browser,
	Messenger
};
