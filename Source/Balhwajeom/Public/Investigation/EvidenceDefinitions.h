#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Investigation/InvestigationEnums.h"
#include "EvidenceDefinitions.generated.h"

class UNiagaraSystem;
class UStaticMesh;
class UUserWidget;

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

	/** Story state tag that must be active before this evidence can be inspected or photographed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence|Progression")
	FGameplayTag RequiredActivationTag;

	/** When active, F interaction clears this object instead of running its normal interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence|Progression")
	FGameplayTag ClearRequiredTag;

	/** Story state tag granted when this object finishes its progression removal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Evidence|Progression")
	FGameplayTag GrantedTagOnClear;
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

	/** Full-screen content opened when InteractionPresentation is ModalWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	/** Keywords granted once when this interaction successfully completes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TArray<FName> GrantedWordIDs;

	/**
	 * Static mesh shown while this state is active, i.e. the "object" the evidence turns into.
	 * Empty keeps whatever mesh the actor was placed with.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UStaticMesh> StateMesh;

	/**
	 * Local offset applied to the evidence mesh while this state is active, on top of the
	 * placement authored in the level. Swapping StateMesh keeps the component transform, so a
	 * mesh exported on a different pivot lands somewhere else; this nudges it back without
	 * touching the art or the other states. Zero means "use the authored placement", which is
	 * what every state wants once its meshes share a pivot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (Units = "cm"))
	FVector StateMeshOffset = FVector::ZeroVector;

	/**
	 * Niagara system played once when this state is entered. It is deliberately skipped when a
	 * level load restores an already-advanced state, so a one-shot burst does not replay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UNiagaraSystem> StateEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection")
	FText FarLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection")
	FText MidLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection", meta = (MultiLine = "true"))
	FText NearLabel;

	/**
	 * Prevents this state from opening the rotating 3D inspector even when the
	 * owning Evidence Actor has Enable 3D Inspection checked. Enable this on a
	 * PostCaptureStateID destination when the photographed object should no
	 * longer open the 3D view.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inspection|3D")
	bool bDisable3DInspection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	bool bCanCapture = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	FName PhotoID = NAME_None;

	/**
	 * State entered once this state's photo is captured. Empty leaves the object in this state.
	 * The capture presentation skips its own world story when this is set, because the destination
	 * state is what presents the story from then on.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	FName PostCaptureStateID = NAME_None;

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
