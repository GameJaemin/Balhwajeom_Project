#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Investigation/InvestigationEnums.h"
#include "EvidenceDefinitions.generated.h"

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FEvidenceDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence")
	FName ObjectID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence")
	FText ObjectName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence")
	FName InitialStateID = NAME_None;
};

USTRUCT(BlueprintType)
struct BALHWAJEOM_API FEvidenceStateDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence State")
	FName StateID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence State")
	FName ObjectID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence State")
	FText StateName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EEvidenceInteractionBehavior InteractionBehavior = EEvidenceInteractionBehavior::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	EEvidenceInteractionPresentation InteractionPresentation = EEvidenceInteractionPresentation::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName NextStateID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (MultiLine = "true"))
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName KeywordDocumentID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection")
	FText FarLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection")
	FText MidLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection", meta = (MultiLine = "true"))
	FText NearLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	bool bCanCapture = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	FName PhotoID = NAME_None;

	/** Added to the photo camera's global minimum focus/capture distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Focus", meta = (Units = "cm"))
	float MinimumFocusDistanceOffset = 0.0f;

	/** Added to the photo camera's global maximum focus/capture distance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Focus", meta = (Units = "cm"))
	float MaximumFocusDistanceOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Legacy", meta = (ClampMin = "1.0", DeprecatedProperty, DeprecationMessage = "Focus distance is now owned by the photo camera. Use MinimumFocusDistanceOffset and MaximumFocusDistanceOffset for per-state variation."))
	float PreferredFocusDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Legacy", meta = (ClampMin = "0.0", DeprecatedProperty, DeprecationMessage = "Focus distance is now owned by the photo camera. Use MinimumFocusDistanceOffset and MaximumFocusDistanceOffset for per-state variation."))
	float FocusDistanceTolerance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Legacy", meta = (DeprecatedProperty, DeprecationMessage = "Zoom no longer changes focus or capture distance."))
	bool bScaleFocusDistanceWithZoom = true;
};
