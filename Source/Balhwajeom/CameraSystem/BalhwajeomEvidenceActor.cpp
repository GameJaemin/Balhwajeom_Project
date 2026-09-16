// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceActor.h"

#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SceneComponent.h"
#include "Components/SizeBox.h"
#include "Components/TextRenderComponent.h"
#include "Components/Image.h"
#include "Components/WidgetComponent.h"
#include "Interaction/InspectionComponent.h"
#include "ItemInspection/JMInspectableComponent.h"
#include "ItemInspection/JMItemInspectionData.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "CameraSystem/BalhwajeomEvidenceFocusGuideLayout.h"
#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "CameraSystem/PhotoWorldStoryActor.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/PlayerController.h"
#include "Investigation/BalhwajeomInvestigationSettings.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Story/StoryStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

ABalhwajeomEvidenceActor::ABalhwajeomEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EvidenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EvidenceMesh"));
	SetRootComponent(EvidenceMesh);
	EvidenceMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CameraTargetBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CameraTargetBounds"));
	CameraTargetBounds->SetupAttachment(EvidenceMesh);
	CameraTargetBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CameraTargetBounds->SetCollisionObjectType(ECC_WorldDynamic);
	CameraTargetBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	CameraTargetBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CameraTargetBounds->SetGenerateOverlapEvents(false);
	CameraTargetBounds->CanCharacterStepUpOn = ECB_No;

	InspectionComponent = CreateDefaultSubobject<UInspectionComponent>(TEXT("InspectionComponent"));
	ItemInspectionComponent = CreateDefaultSubobject<UJMInspectableComponent>(TEXT("ItemInspectionComponent"));
	ItemInspectionComponent->bInspectionEnabled = false;
	ItemInspectionComponent->bBlockPlayerInputDuringInspection = true;
	ItemInspectionComponent->bHideSourceActorDuringInspection = true;

	ObjectLabelWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ObjectLabelWidget"));
	ObjectLabelWidget->SetupAttachment(EvidenceMesh);
	ObjectLabelWidget->SetRelativeLocation(FVector::ZeroVector);
	ObjectLabelWidget->SetWidgetSpace(EWidgetSpace::Screen);
	ObjectLabelWidget->SetDrawAtDesiredSize(true);
	ObjectLabelWidget->SetVisibility(false);
	ObjectLabelWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<UUserWidget> ObjectLabelWidgetClass(
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel"));
	if (ObjectLabelWidgetClass.Succeeded())
	{
		ObjectLabelWidget->SetWidgetClass(ObjectLabelWidgetClass.Class);
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> PhotoRequiredIconAsset(
		TEXT("/Game/Balhwajeom/UI/Icons/T_EvidencePhotoRequired.T_EvidencePhotoRequired"));
	if (PhotoRequiredIconAsset.Succeeded())
	{
		PhotoRequiredIcon = PhotoRequiredIconAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> PhotoUnavailableIconAsset(
		TEXT("/Game/Balhwajeom/UI/Icons/DotIcon.DotIcon"));
	if (PhotoUnavailableIconAsset.Succeeded())
	{
		PhotoUnavailableIcon = PhotoUnavailableIconAsset.Object;
	}

	CameraFocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CameraFocusPoint"));
	CameraFocusPoint->SetupAttachment(EvidenceMesh);

	StateEffectAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("StateEffectAnchor"));
	StateEffectAnchor->SetupAttachment(EvidenceMesh);

	// Sits slightly above the object so a freshly placed actor shows readable text before the
	// designer positions it; +X points at the reader because the story widget faces its own +X.
	StoryAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("StoryAnchor"));
	StoryAnchor->SetupAttachment(EvidenceMesh);
	// Follow the evidence position, but keep the authored reading angle in world space.
	StoryAnchor->SetAbsolute(/*bNewAbsoluteLocation*/ false, /*bNewAbsoluteRotation*/ true,
		/*bNewAbsoluteScale*/ false);
	StoryAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));

#if WITH_EDITORONLY_DATA
	StateEffectAnchorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(
		TEXT("StateEffectAnchorArrow"));
	if (StateEffectAnchorArrow)
	{
		StateEffectAnchorArrow->SetupAttachment(StateEffectAnchor);
		StateEffectAnchorArrow->ArrowColor = FColor(255, 180, 60);
		StateEffectAnchorArrow->bIsScreenSizeScaled = true;
		StateEffectAnchorArrow->SetHiddenInGame(true);
	}

	StoryAnchorArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("StoryAnchorArrow"));
	if (StoryAnchorArrow)
	{
		StoryAnchorArrow->SetupAttachment(StoryAnchor);
		StoryAnchorArrow->ArrowColor = FColor(140, 200, 255);
		StoryAnchorArrow->bIsScreenSizeScaled = true;
		StoryAnchorArrow->SetHiddenInGame(true);
	}

	StoryAnchorPreviewText = CreateEditorOnlyDefaultSubobject<UTextRenderComponent>(
		TEXT("StoryAnchorPreviewText"));
	if (StoryAnchorPreviewText)
	{
		StoryAnchorPreviewText->SetupAttachment(StoryAnchor);
		StoryAnchorPreviewText->SetText(NSLOCTEXT(
			"BalhwajeomEvidence", "StoryAnchorPreview", "Story Anchor 예시 텍스트"));
		StoryAnchorPreviewText->SetHorizontalAlignment(EHTA_Center);
		StoryAnchorPreviewText->SetVerticalAlignment(EVRTA_TextCenter);
		StoryAnchorPreviewText->SetWorldSize(18.0f);
		StoryAnchorPreviewText->SetTextRenderColor(FColor(140, 220, 255));
		StoryAnchorPreviewText->SetHiddenInGame(true);
	}
#endif

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMesh.Succeeded())
	{
		EvidenceMesh->SetStaticMesh(DefaultMesh.Object);
		EvidenceMesh->SetRelativeScale3D(FVector(0.5f));
	}
}

void ABalhwajeomEvidenceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FitCameraTargetBoundsToMesh();
}

void ABalhwajeomEvidenceActor::FitCameraTargetBoundsToMesh()
{
	if (!EvidenceMesh || !CameraTargetBounds)
	{
		return;
	}
	FVector LocalMin;
	FVector LocalMax;
	EvidenceMesh->GetLocalBounds(LocalMin, LocalMax);
	const FVector LocalExtent = (LocalMax - LocalMin) * 0.5f;
	CameraTargetBounds->SetRelativeLocation((LocalMin + LocalMax) * 0.5f);
	CameraTargetBounds->SetBoxExtent(FVector(
		FMath::Max(LocalExtent.X, 5.0f),
		FMath::Max(LocalExtent.Y, 5.0f),
		FMath::Max(LocalExtent.Z, 5.0f)));
}

void ABalhwajeomEvidenceActor::UpdateObjectLabelPlacement()
{
	if (!ObjectLabelWidget)
	{
		return;
	}
	// Bounds.Origin is the visible mesh center even when the mesh asset's pivot is off-center.
	const FVector WorldOffset = EvidenceMesh
		? EvidenceMesh->GetComponentTransform().TransformVectorNoScale(ObjectLabelOffset)
		: GetActorTransform().TransformVectorNoScale(ObjectLabelOffset);
	const FVector LabelCenter = EvidenceMesh ? EvidenceMesh->Bounds.Origin : GetActorLocation();
	ObjectLabelWidget->SetWorldLocation(LabelCenter + WorldOffset);
}

void ABalhwajeomEvidenceActor::ApplyStateVisuals(
	const FEvidenceStateDefinition& State,
	bool bInitialApply)
{
	bool bMeshPresentationChanged = false;

	// The mesh is applied on a load too, otherwise a restored state shows the wrong object.
	if (EvidenceMesh && !State.StateMesh.IsNull())
	{
		// Synchronous because the swap has to land in the same frame as the state change; these
		// are small props, not streamed geometry.
		UStaticMesh* StateMesh = State.StateMesh.LoadSynchronous();
		if (StateMesh && EvidenceMesh->GetStaticMesh() != StateMesh)
		{
			const UStaticMesh* PreviousMesh = EvidenceMesh->GetStaticMesh();
			EvidenceMesh->SetStaticMesh(StateMesh);
			// Per-instance material overrides are indexed by slot, so the placement's overrides
			// survive the swap and hide the new mesh's own materials -- the '_e' memory look
			// never appears on any object whose placement overrode a material.
			EvidenceMesh->EmptyOverrideMaterials();
			ReportStateMeshPivotShift(State, PreviousMesh, StateMesh);
			bMeshPresentationChanged = true;
		}
		else if (!StateMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s: state '%s' could not load StateMesh '%s'."),
				*GetName(), *State.StateID.ToString(), *State.StateMesh.ToString());
		}
	}

	// Applied on every state, not just on a swap, so leaving an offset state puts the mesh back.
	if (EvidenceMesh && bEvidenceMeshBaselineCaptured)
	{
		// The offset is authored in the mesh's own space, the space the pivot difference was
		// measured in, so it has to be rotated and scaled into the placement before it is added.
		const FVector DesiredLocation = EvidenceMeshBaselineRelativeLocation +
			EvidenceMesh->GetRelativeTransform().TransformVector(State.StateMeshOffset);
		if (!EvidenceMesh->GetRelativeLocation().Equals(DesiredLocation))
		{
			EvidenceMesh->SetRelativeLocation(DesiredLocation);
			bMeshPresentationChanged = true;
		}
	}

	if (bMeshPresentationChanged)
	{
		// Photo focus and interaction traces run against this volume, so it has to follow.
		FitCameraTargetBoundsToMesh();
		UpdateObjectLabelPlacement();
	}

	if (ActiveStateEffect.IsValid())
	{
		ActiveStateEffect->Deactivate();
	}
	ActiveStateEffect.Reset();

	// A load restoring an already-advanced state must not replay the transition burst.
	if (bInitialApply || State.StateEffect.IsNull() || !EvidenceMesh)
	{
		return;
	}

	UNiagaraSystem* Effect = State.StateEffect.LoadSynchronous();
	if (!Effect)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: state '%s' could not load StateEffect '%s'."),
			*GetName(), *State.StateID.ToString(), *State.StateEffect.ToString());
		return;
	}

	ActiveStateEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
		Effect,
		StateEffectAnchor,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy*/ true);

}

void ABalhwajeomEvidenceActor::ReportStateMeshPivotShift(
	const FEvidenceStateDefinition& State,
	const UStaticMesh* PreviousMesh,
	const UStaticMesh* NewMesh) const
{
	// An authored offset means somebody already knows the pivots differ.
	if (!PreviousMesh || !NewMesh || !State.StateMeshOffset.IsNearlyZero())
	{
		return;
	}

	const FVector PivotShift =
		NewMesh->GetBounds().Origin - PreviousMesh->GetBounds().Origin;
	// Variants exported from one source share a pivot, so a real difference means the new mesh
	// will sit somewhere else entirely. That reads as "the mesh is broken", so name it here
	// instead of leaving it to be found by eye.
	if (PivotShift.Size() <= 1.0f)
	{
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("%s: state '%s' swaps to a mesh sitting %s from the previous pivot, so it will not ")
		TEXT("land where the object was placed. Re-export it on the same pivot, or set that ")
		TEXT("state's StateMeshOffset to %s in DT_EvidenceStates."),
		*GetName(),
		*State.StateID.ToString(),
		*PivotShift.ToString(),
		*(-PivotShift).ToString());
}

void ABalhwajeomEvidenceActor::BeginPlay()
{
	Super::BeginPlay();

	bActorBaselineHidden = IsHidden();
	bActorBaselineCollisionEnabled = GetActorEnableCollision();
	if (EvidenceMesh)
	{
		// Captured before any state applies, so StateMeshOffset always measures from the
		// placement the level author sees in the editor.
		EvidenceMeshBaselineRelativeLocation = EvidenceMesh->GetRelativeLocation();
		bEvidenceMeshBaselineCaptured = true;
	}
	FitCameraTargetBoundsToMesh();
	if (CameraTargetBounds)
	{
		CameraTargetBoundsBaselineCollisionEnabled =
			CameraTargetBounds->GetCollisionEnabled();
		CameraTargetBoundsBaselineVisibilityResponse =
			CameraTargetBounds->GetCollisionResponseToChannel(ECC_Visibility);
		bCameraTargetBoundsBaselineCaptured = true;
	}

	if (ObjectLabelWidget)
	{
		UpdateObjectLabelPlacement();
		ObjectLabelWidget->SetVisibility(false);
	}

	if (InspectionComponent)
	{
		InspectionComponent->OnPlayerDistanceStateChanged.AddDynamic(
			this,
			&ABalhwajeomEvidenceActor::HandlePlayerDistanceStateChanged);
	}
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		Investigation->OnPhotoCaptured.AddUniqueDynamic(
			this, &ABalhwajeomEvidenceActor::HandlePhotoCaptured);
		Investigation->OnPhotoGalleryReset.AddUniqueDynamic(
			this, &ABalhwajeomEvidenceActor::HandlePhotoGalleryReset);
	}
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UStoryStateSubsystem* StoryState =
				GameInstance->GetSubsystem<UStoryStateSubsystem>())
			{
				StoryState->OnStateTagAdded.AddUniqueDynamic(
					this, &ThisClass::HandleStoryStateTagChanged);
				StoryState->OnStateTagRemoved.AddUniqueDynamic(
					this, &ThisClass::HandleStoryStateTagChanged);
			}
		}
	}

	// bInspectionEnabled is derived state that every ConfigureItemInspection() rewrites.
	// The component is visible in the details panel, so it is the flag designers reach for;
	// read the authored tick before anything overwrites it instead of silently dropping it.
	bAuthoredItemInspectionEnabled =
		ItemInspectionComponent && ItemInspectionComponent->bInspectionEnabled;
	if (bAuthoredItemInspectionEnabled && !bEnable3DInspection)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: ItemInspectionComponent 'Inspection Enabled' is ticked while 'Enable 3D Inspection' is off. ")
			TEXT("Honouring it, but 'Enable 3D Inspection' on the Evidence Actor is the switch this system reads."),
			*GetName());
	}

	RegisterWithInvestigationSystem();

	// Registration bails out on a missing ObjectID or DT_EvidenceStates row, and it owns the
	// only other path to ConfigureItemInspection(). Without this the inspector would have no
	// InspectionData, and CanInspect() would refuse every F press without saying why.
	ConfigureItemInspection();
}

void ABalhwajeomEvidenceActor::ConfigureItemInspection()
{
	if (!ItemInspectionComponent)
	{
		return;
	}

	const bool bInspectionRequested =
		bEnable3DInspection || bAuthoredItemInspectionEnabled;
	const bool bProgressionAllowsInspection =
		bProgressionAvailable && !bProgressionCleared && !bProgressionRemovalPending;
	ItemInspectionComponent->bInspectionEnabled = ShouldEnable3DInspectionForState(
		bInspectionRequested,
		bProgressionAllowsInspection,
		bCurrentStateDisables3DInspection);
	ItemInspectionComponent->InspectionData = nullptr;
	RuntimeItemInspectionData = nullptr;
	if (!ItemInspectionComponent->bInspectionEnabled)
	{
		return;
	}

	if (!EvidenceMesh || !EvidenceMesh->GetStaticMesh())
	{
		// CanInspect() needs InspectionData as well as the flag, so leaving the flag on
		// would only turn a missing mesh into another silent "F does nothing".
		UE_LOG(LogTemp, Warning,
			TEXT("%s: 3D inspection is enabled but EvidenceMesh has no Static Mesh to show."),
			*GetName());
		ItemInspectionComponent->bInspectionEnabled = false;
		return;
	}

	RuntimeItemInspectionData = ItemInspectionData
		? DuplicateObject<UJMItemInspectionData>(ItemInspectionData, this)
		: NewObject<UJMItemInspectionData>(this);
	if (!RuntimeItemInspectionData)
	{
		ItemInspectionComponent->bInspectionEnabled = false;
		return;
	}

	if (RuntimeItemInspectionData->ItemId.IsNone())
	{
		RuntimeItemInspectionData->ItemId = ObjectID.IsNone() ? EvidenceData.EvidenceID : ObjectID;
	}
	if (RuntimeItemInspectionData->DisplayName.IsEmpty())
	{
		RuntimeItemInspectionData->DisplayName = EvidenceData.EvidenceName;
	}
	if (RuntimeItemInspectionData->DisplayCategory.IsEmpty())
	{
		RuntimeItemInspectionData->DisplayCategory = NSLOCTEXT("Balhwajeom", "EvidenceInspectionCategory", "Evidence");
	}
	if (RuntimeItemInspectionData->Description.IsEmpty() && InspectionComponent)
	{
		RuntimeItemInspectionData->Description = InspectionComponent->InspectionText;
	}
	if (RuntimeItemInspectionData->PreviewMesh.IsNull())
	{
		RuntimeItemInspectionData->PreviewMesh = EvidenceMesh->GetStaticMesh();
	}

	ItemInspectionComponent->InspectionData = RuntimeItemInspectionData;
}

void ABalhwajeomEvidenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		Investigation->OnEvidenceStateChanged.RemoveDynamic(
			this, &ABalhwajeomEvidenceActor::HandleEvidenceStateChanged);
		Investigation->OnPhotoCaptured.RemoveDynamic(
			this, &ABalhwajeomEvidenceActor::HandlePhotoCaptured);
		Investigation->OnPhotoGalleryReset.RemoveDynamic(
			this, &ABalhwajeomEvidenceActor::HandlePhotoGalleryReset);
	}
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UStoryStateSubsystem* StoryState =
				GameInstance->GetSubsystem<UStoryStateSubsystem>())
			{
				StoryState->OnStateTagAdded.RemoveDynamic(
					this, &ThisClass::HandleStoryStateTagChanged);
				StoryState->OnStateTagRemoved.RemoveDynamic(
					this, &ThisClass::HandleStoryStateTagChanged);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ABalhwajeomEvidenceActor::ConfigureInvestigationObject(FName InObjectID)
{
	ObjectID = InObjectID;
	EvidenceData.EvidenceID = InObjectID;
}

bool ABalhwajeomEvidenceActor::RequestInvestigationInteraction(FText& OutDisplayText)
{
	OutDisplayText = FText::GetEmpty();
	if (!bProgressionAvailable || bProgressionCleared || bProgressionRemovalPending)
	{
		return false;
	}
	// Do not let rapid interaction restart the presentation. StopStory fades the old actor, so
	// replacing it here used to leave the fading caption underneath the newly spawned caption.
	// The weak pointer becomes invalid automatically after the final cue and fade have completed.
	if (ActiveWorldStory.IsValid())
	{
		return false;
	}
	if (CanClearForProgression())
	{
		bProgressionRemovalPending = true;
		BeginProgressionRemoval();
		return true;
	}
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceInteractionViewData ViewData;
	if (!Investigation || !Investigation->BeginEvidenceInteraction(EvidenceInstanceID, ViewData))
	{
		// A WorldStory state that refuses interaction is almost always a data mistake, and the
		// silent failure is indistinguishable from "F did nothing", so name the cause here.
		LogBlockedWorldStory(TEXT("its InteractionBehavior is None or its Once interaction is already spent"));
		return false;
	}

	OutDisplayText = ViewData.InteractionText;
	ABalhwajeomCameraPlayerController* ModalController = nullptr;
	TSubclassOf<UUserWidget> ModalContentClass;
	FText ModalDocumentText = ViewData.InteractionText;
	if (ViewData.Presentation == EEvidenceInteractionPresentation::ModalWidget)
	{
		ModalController = Cast<ABalhwajeomCameraPlayerController>(
			GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr);
		ModalContentClass = ViewData.InteractionWidgetClass.LoadSynchronous();
		if (!ModalController || ModalController->IsInteractionModalOpen() || !ModalContentClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("%s: state '%s' could not prepare its interaction modal '%s'."),
				*GetName(),
				*ViewData.StateID.ToString(),
				*ViewData.InteractionWidgetClass.ToString());
			return false;
		}
		FKeywordDocumentDefinition Document;
		if (Investigation->GetKeywordDocumentDefinition(
			ViewData.KeywordDocumentID, Document))
		{
			ModalDocumentText = Document.DocumentText;
		}
		// The dedicated modal owns the presentation; suppress the legacy inspection message.
		OutDisplayText = FText::GetEmpty();
	}
	if (ViewData.Presentation == EEvidenceInteractionPresentation::KeywordSelectionWindow)
	{
		FKeywordDocumentDefinition Document;
		if (Investigation->GetKeywordDocumentDefinition(ViewData.KeywordDocumentID, Document))
		{
			OutDisplayText = Document.DocumentText;
		}
		TArray<FKeywordChoiceDefinition> Choices;
		Investigation->GetKeywordChoicesForDocument(ViewData.KeywordDocumentID, Choices);
		if (Choices.Num() == 1)
		{
			Investigation->SelectKeywordChoice(
				ViewData.KeywordDocumentID,
				Choices[0].ChoiceID,
				EWordAcquisitionSource::EvidenceInteraction);
		}
	}
	else if (ViewData.Presentation == EEvidenceInteractionPresentation::WorldStory)
	{
		// The cues are presented as world text, so the 2D inspection popup stays silent.
		OutDisplayText = FText::GetEmpty();
	}

	TArray<FName> NewlyGrantedWordIDs;
	if (!Investigation->CompleteEvidenceInteractionWithGrantedWords(
		EvidenceInstanceID, ViewData.StateID, NewlyGrantedWordIDs))
	{
		LogBlockedWorldStory(TEXT("its InteractionBehavior is ChangeState but NextStateID is empty or points at another object"));
		return false;
	}

	if (ViewData.Presentation == EEvidenceInteractionPresentation::ModalWidget)
	{
		TArray<FText> NewlyGrantedKeywordTexts;
		for (const FName WordID : NewlyGrantedWordIDs)
		{
			FWordDefinition Word;
			if (Investigation->GetWordDefinition(WordID, Word))
			{
				NewlyGrantedKeywordTexts.Add(Word.DisplayWord);
			}
		}
		if (!ModalController->ShowInteractionModal(
			ModalContentClass, ModalDocumentText, NewlyGrantedKeywordTexts))
		{
			UE_LOG(LogTemp, Error,
				TEXT("%s: interaction completed but state '%s' failed to open its modal."),
				*GetName(), *ViewData.StateID.ToString());
		}
	}

	if (ViewData.Presentation == EEvidenceInteractionPresentation::WorldStory)
	{
		// Uses the state we interacted with, which matters when the interaction also changed state.
		PlayWorldStoryForState(ViewData.StateID);
	}
	return true;
}

void ABalhwajeomEvidenceActor::LogBlockedWorldStory(const TCHAR* Reason) const
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceStateDefinition State;
	if (!Investigation || !Investigation->GetEvidenceStateDefinition(CurrentStateID, State) ||
		State.InteractionPresentation != EEvidenceInteractionPresentation::WorldStory)
	{
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("%s: state '%s' is set to WorldStory but cannot present it because %s."),
		*GetName(), *CurrentStateID.ToString(), Reason);
}

bool ABalhwajeomEvidenceActor::PlayWorldStory()
{
	return bProgressionAvailable && !bProgressionCleared &&
		!bProgressionRemovalPending && PlayWorldStoryForState(CurrentStateID);
}

void ABalhwajeomEvidenceActor::StopWorldStory()
{
	if (ActiveWorldStory.IsValid())
	{
		ActiveWorldStory->StopStory();
	}
	ActiveWorldStory.Reset();
}

FTransform ABalhwajeomEvidenceActor::GetWorldStoryTransform() const
{
	FTransform AnchorTransform = StoryAnchor
		? StoryAnchor->GetComponentTransform()
		: GetActorTransform();
	// The story widget carries its own scale, so a scaled evidence actor must not shrink the text.
	AnchorTransform.SetScale3D(FVector::OneVector);

	const APlayerController* PlayerController = GetWorld()
		? GetWorld()->GetFirstPlayerController()
		: nullptr;
	if (!bStoryFacesPlayer || !PlayerController)
	{
		return AnchorTransform;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	FRotator StoryRotation = AnchorTransform.Rotator();
	StoryRotation.Yaw = (ViewLocation - AnchorTransform.GetLocation()).Rotation().Yaw;
	AnchorTransform.SetRotation(StoryRotation.Quaternion());
	return AnchorTransform;
}

bool ABalhwajeomEvidenceActor::PlayWorldStoryForState(FName StateID)
{
	if (!bProgressionAvailable || bProgressionCleared || bProgressionRemovalPending)
	{
		return false;
	}
	if (ActiveWorldStory.IsValid())
	{
		return false;
	}

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceStateDefinition State;
	if (!Investigation || !Investigation->GetEvidenceStateDefinition(StateID, State))
	{
		return false;
	}
	if (State.PhotoID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: state '%s' asks for a world story but has no PhotoID."),
			*GetName(), *StateID.ToString());
		return false;
	}

	FPhotoDefinition Photo;
	if (!Investigation->GetPhotoDefinition(State.PhotoID, Photo))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s could not resolve PhotoID '%s' for a world story."),
			*GetName(), *State.PhotoID.ToString());
		return false;
	}

	ActiveWorldStory = APhotoWorldStoryActor::SpawnAndStart(
		GetWorld(),
		StoryActorClass,
		GetWorldStoryTransform(),
		Photo,
		this);
	if (!ActiveWorldStory.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: PhotoID '%s' has no WorldStoryCues to present."),
			*GetName(), *State.PhotoID.ToString());
		return false;
	}

	// Evidence interaction is blocked while the photo camera is raised, so this presentation is
	// always read in the third-person view and needs the larger font straight away.
	ActiveWorldStory->TransitionToThirdPersonScale();

	// A Repeatable world story never changes state, so this is what lets progression
	// gates ask "has the player heard this one yet?".
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UStoryStateSubsystem* StoryState =
				GameInstance->GetSubsystem<UStoryStateSubsystem>())
			{
				StoryState->AddEvidenceStoryPlayedTag(StateID);
				// Object-scoped as well, so a gate can ask "heard this one yet?" without
				// caring whether it played before or after the photo.
				StoryState->AddEvidenceStoryHeardTag(ObjectID);
			}
		}
	}

	return true;
}

bool ABalhwajeomEvidenceActor::CanRequestInvestigationInteraction() const
{
	if (!bProgressionAvailable || bProgressionCleared || bProgressionRemovalPending ||
		ActiveWorldStory.IsValid())
	{
		return false;
	}
	if (CanClearForProgression())
	{
		return true;
	}

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceInteractionViewData ViewData;
	return Investigation &&
		Investigation->BeginEvidenceInteraction(EvidenceInstanceID, ViewData);
}

FText ABalhwajeomEvidenceActor::GetInteractionPromptText() const
{
	const UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceStateDefinition State;
	return Investigation && Investigation->GetEvidenceStateDefinition(CurrentStateID, State)
		? State.InteractionPromptText
		: FText::GetEmpty();
}

UBalhwajeomInvestigationSubsystem* ABalhwajeomEvidenceActor::GetInvestigationSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
}

void ABalhwajeomEvidenceActor::RegisterWithInvestigationSystem()
{
	if (ObjectID.IsNone())
	{
		ObjectID = EvidenceData.EvidenceID;
	}
	if (ObjectID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no investigation ObjectID."), *GetName());
		return;
	}
	if (!EvidenceInstanceID.IsValid())
	{
		EvidenceInstanceID = FGuid::NewGuid();
	}
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || !Investigation->RegisterEvidenceActor(EvidenceInstanceID, ObjectID, CurrentStateID))
	{
		UE_LOG(LogTemp, Error, TEXT("%s failed to register ObjectID '%s'."), *GetName(), *ObjectID.ToString());
		return;
	}
	Investigation->OnEvidenceStateChanged.AddUniqueDynamic(
		this, &ABalhwajeomEvidenceActor::HandleEvidenceStateChanged);
	ApplyInvestigationState(CurrentStateID, /*bInitialApply*/ true);
}

void ABalhwajeomEvidenceActor::HandleEvidenceStateChanged(
	FGuid ChangedInstanceID, FName PreviousStateID, FName NewStateID)
{
	if (ChangedInstanceID == EvidenceInstanceID)
	{
		ApplyInvestigationState(NewStateID);
		ApplyInspectionDistanceState(LastInspectionDistanceState);
	}
}

void ABalhwajeomEvidenceActor::ApplyInvestigationState(FName StateID, bool bInitialApply)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceStateDefinition State;
	if (!Investigation || !Investigation->GetEvidenceStateDefinition(StateID, State))
	{
		return;
	}
	const FName PreviousStateID = CurrentStateID;
	CurrentStateID = State.StateID;
	bCurrentStateDisables3DInspection = State.bDisable3DInspection;
	bCanBeCaptured = State.bCanCapture;
	EvidenceData.bAlreadyCollected = !State.PhotoID.IsNone() &&
		Investigation->HasCapturedPhoto(State.PhotoID);
	MinimumFocusDistanceOffset = State.MinimumFocusDistanceOffset;
	MaximumFocusDistanceOffset = State.MaximumFocusDistanceOffset;
	MinimumCaptureScreenOccupancyRatioOverride = State.MinimumCaptureScreenOccupancyRatioOverride;
	PreferredFocusDistanceAt1x = State.PreferredFocusDistance;
	FocusDistanceToleranceAt1x = State.FocusDistanceTolerance;
	bScaleFocusDistanceWithZoom = State.bScaleFocusDistanceWithZoom;
	if (InspectionComponent)
	{
		InspectionComponent->FarLabel = State.FarLabel;
		InspectionComponent->MidLabel = State.MidLabel;
		InspectionComponent->NearLabel = State.NearLabel;
		InspectionComponent->InspectionText = State.InteractionText.IsEmpty()
			? State.NearLabel : State.InteractionText;
	}
	FEvidenceDefinition ObjectDefinition;
	if (Investigation->GetEvidenceDefinition(ObjectID, ObjectDefinition))
	{
		EvidenceData.EvidenceID = ObjectID;
		EvidenceData.EvidenceName = ObjectDefinition.ObjectName;
		const UBalhwajeomInvestigationSettings* Settings =
			GetDefault<UBalhwajeomInvestigationSettings>();
		RequiredActivationTag = Settings
			? Settings->ResolveChapter01RequiredActivationTag(
				ObjectID, ObjectDefinition.RequiredActivationTag)
			: ObjectDefinition.RequiredActivationTag;
		ClearRequiredTag = ObjectDefinition.ClearRequiredTag;
		GrantedTagOnClear = ObjectDefinition.GrantedTagOnClear;
	}
	ApplyStateVisuals(State, bInitialApply);
	// After the swap, so the rotating inspector captures the state's mesh and the
	// progression gate can disable every interaction surface together.
	RefreshProgressionAvailability();
	RefreshProgressionClearedState();

	// Last, so Blueprint reacts to a fully applied state. Use it for anything the DataTable
	// columns cannot express, such as swapping a whole child actor or driving a material.
	OnEvidenceStateApplied(PreviousStateID, CurrentStateID, bInitialApply);
}

void ABalhwajeomEvidenceActor::HandlePlayerDistanceStateChanged(
	EPlayerInspectionDistanceState NewState)
{
	LastInspectionDistanceState = NewState;
	ApplyInspectionDistanceState(NewState);
}

void ABalhwajeomEvidenceActor::HandleStoryStateTagChanged(FGameplayTag StateTag)
{
	if (StateTag == RequiredActivationTag)
	{
		RefreshProgressionAvailability();
	}
	if (StateTag == GrantedTagOnClear)
	{
		RefreshProgressionClearedState();
	}
}

bool ABalhwajeomEvidenceActor::CanClearForProgression() const
{
	if (!ClearRequiredTag.IsValid() || !GrantedTagOnClear.IsValid())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UStoryStateSubsystem* StoryState = GameInstance
		? GameInstance->GetSubsystem<UStoryStateSubsystem>()
		: nullptr;
	return StoryState && StoryState->HasStateTagExact(ClearRequiredTag);
}

void ABalhwajeomEvidenceActor::BeginProgressionRemoval_Implementation()
{
	FinalizeProgressionRemoval();
}

void ABalhwajeomEvidenceActor::FinalizeProgressionRemoval()
{
	if (bProgressionCleared)
	{
		return;
	}

	bProgressionRemovalPending = false;
	bProgressionCleared = true;
	SetInspectionLabel(FText::GetEmpty(), false);
	StopWorldStory();
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	ConfigureItemInspection();

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (UStoryStateSubsystem* StoryState = GameInstance
		? GameInstance->GetSubsystem<UStoryStateSubsystem>()
		: nullptr)
	{
		StoryState->AddStateTag(GrantedTagOnClear);
	}
}

void ABalhwajeomEvidenceActor::RefreshProgressionAvailability()
{
	bool bShouldBeAvailable = !RequiredActivationTag.IsValid();
	if (!bShouldBeAvailable)
	{
		const UWorld* World = GetWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UStoryStateSubsystem* StoryState = GameInstance
			? GameInstance->GetSubsystem<UStoryStateSubsystem>()
			: nullptr;
		bShouldBeAvailable = StoryState &&
			StoryState->HasStateTagExact(RequiredActivationTag);
	}

	bProgressionAvailable = bShouldBeAvailable;
	if (CameraTargetBounds && bCameraTargetBoundsBaselineCaptured)
	{
		CameraTargetBounds->SetCollisionEnabled(
			bProgressionAvailable
				? CameraTargetBoundsBaselineCollisionEnabled
				: ECollisionEnabled::NoCollision);
		CameraTargetBounds->SetCollisionResponseToChannel(
			ECC_Visibility,
			bProgressionAvailable
				? CameraTargetBoundsBaselineVisibilityResponse
				: ECR_Ignore);
	}

	if (!bProgressionAvailable)
	{
		SetInspectionLabel(FText::GetEmpty(), false);
		StopWorldStory();
	}

	ConfigureItemInspection();
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

void ABalhwajeomEvidenceActor::RefreshProgressionClearedState()
{
	bool bShouldBeCleared = false;
	if (GrantedTagOnClear.IsValid())
	{
		const UWorld* World = GetWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UStoryStateSubsystem* StoryState = GameInstance
			? GameInstance->GetSubsystem<UStoryStateSubsystem>()
			: nullptr;
		bShouldBeCleared = StoryState &&
			StoryState->HasStateTagExact(GrantedTagOnClear);
	}

	bProgressionCleared = bShouldBeCleared;
	if (bProgressionCleared)
	{
		bProgressionRemovalPending = false;
		SetInspectionLabel(FText::GetEmpty(), false);
		StopWorldStory();
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		ConfigureItemInspection();
		return;
	}

	SetActorHiddenInGame(bActorBaselineHidden);
	SetActorEnableCollision(bActorBaselineCollisionEnabled);
	RefreshProgressionAvailability();
}

void ABalhwajeomEvidenceActor::SetInspectionLabelSuppressed(bool bSuppressed)
{
	bInspectionLabelSuppressed = bSuppressed;
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

void ABalhwajeomEvidenceActor::ApplyInspectionDistanceState(
	EPlayerInspectionDistanceState DistanceState)
{
	if (bInspectionLabelSuppressed)
	{
		SetInspectionLabel(FText::GetEmpty(), false);
		return;
	}
	if (!bProgressionAvailable || bProgressionCleared || bProgressionRemovalPending)
	{
		SetInspectionLabel(FText::GetEmpty(), false);
		return;
	}

	if (!InspectionComponent)
	{
		SetInspectionLabel(FText::GetEmpty(), false);
		return;
	}

	switch (DistanceState)
	{
	case EPlayerInspectionDistanceState::Far:
		SetInspectionLabel(InspectionComponent->FarLabel, true);
		break;

	case EPlayerInspectionDistanceState::Middle:
		SetInspectionLabel(InspectionComponent->MidLabel, true);
		break;

	case EPlayerInspectionDistanceState::Close:
		SetInspectionLabel(InspectionComponent->NearLabel, true);
		break;

	case EPlayerInspectionDistanceState::OutOfRange:
	default:
		SetInspectionLabel(FText::GetEmpty(), false);
		break;
	}
}

void ABalhwajeomEvidenceActor::HandlePhotoCaptured(
	const FCapturedPhotoRecord& PhotoRecord)
{
	if (!bProgressionAvailable || bProgressionCleared ||
		bProgressionRemovalPending || !EvidenceInstanceID.IsValid() ||
		PhotoRecord.EvidenceInstanceID != EvidenceInstanceID)
	{
		return;
	}

	MarkAsCollected();
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		// No-op unless the captured state names a PostCaptureStateID.
		Investigation->AdvanceEvidenceStateAfterCapture(EvidenceInstanceID);
	}
}

void ABalhwajeomEvidenceActor::HandlePhotoGalleryReset()
{
	EvidenceData.bAlreadyCollected = false;
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

FText ABalhwajeomEvidenceActor::FormatInspectionLabel(
	EPlayerInspectionDistanceState DistanceState,
	const FText& LabelText)
{
	if ((DistanceState == EPlayerInspectionDistanceState::Middle ||
		 DistanceState == EPlayerInspectionDistanceState::Close) &&
		!LabelText.IsEmptyOrWhitespace())
	{
		return LabelText;
	}

	return FText::GetEmpty();
}

bool ABalhwajeomEvidenceActor::ShouldDisplayInspectionLabel(
	EPlayerInspectionDistanceState DistanceState,
	const FText& LabelText)
{
	if (DistanceState == EPlayerInspectionDistanceState::Far)
	{
		return true;
	}

	return (DistanceState == EPlayerInspectionDistanceState::Middle ||
			DistanceState == EPlayerInspectionDistanceState::Close) &&
		!LabelText.IsEmptyOrWhitespace();
}

bool ABalhwajeomEvidenceActor::ShouldEnable3DInspectionForState(
	const bool bInspectionRequested,
	const bool bProgressionAllowsInspection,
	const bool bStateDisablesInspection)
{
	return bInspectionRequested
		&& bProgressionAllowsInspection
		&& !bStateDisablesInspection;
}

void ABalhwajeomEvidenceActor::SetInspectionLabel(
	const FText& LabelText,
	bool bVisible)
{
	if (!ObjectLabelWidget)
	{
		return;
	}

	const FText DisplayText = FormatInspectionLabel(
		LastInspectionDistanceState,
		LabelText);
	const bool bShouldDisplay =
		bVisible && ShouldDisplayInspectionLabel(LastInspectionDistanceState, LabelText);
	ObjectLabelWidget->SetVisibility(bShouldDisplay);
	if (!bShouldDisplay)
	{
		return;
	}

	ObjectLabelWidget->InitWidget();
	UUserWidget* LabelWidget = ObjectLabelWidget->GetUserWidgetObject();
	if (!IsValid(LabelWidget))
	{
		return;
	}

	if (UImage* StatusImage = Cast<UImage>(LabelWidget->GetWidgetFromName(TEXT("UseCamera"))))
	{
		const bool bUsePhotoRequiredIcon =
			BalhwajeomEvidenceFocusGuideLayout::ShouldUsePhotoRequiredIcon(
				bCanBeCaptured,
				EvidenceData.bAlreadyCollected);
		UTexture2D* StatusTexture = bUsePhotoRequiredIcon
			? PhotoRequiredIcon.Get()
			: PhotoUnavailableIcon.Get();
		const double LabelPositionX = bUsePhotoRequiredIcon ? 40.0 : 22.0;
		if (StatusTexture)
		{
			StatusImage->SetBrushFromTexture(StatusTexture, false);
		}

		if (USizeBox* LabelContainer = Cast<USizeBox>(
			LabelWidget->GetWidgetFromName(TEXT("LabelContainer"))))
		{
			if (UCanvasPanelSlot* LabelContainerSlot =
				Cast<UCanvasPanelSlot>(LabelContainer->Slot))
			{
				FVector2D Position = LabelContainerSlot->GetPosition();
				Position.X = LabelPositionX;
				LabelContainerSlot->SetPosition(Position);
			}
		}
	}

	UFunction* SetLabelTextFunction = LabelWidget->FindFunction(TEXT("SetLabelText"));
	if (!SetLabelTextFunction)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: WBP_ObjectLabel does not provide SetLabelText(FText)."),
			*GetName());
		return;
	}

	struct FSetLabelTextParameters
	{
		FText NewText;
	};

	FSetLabelTextParameters Parameters{ DisplayText };
	LabelWidget->ProcessEvent(SetLabelTextFunction, &Parameters);
}

void ABalhwajeomEvidenceActor::MarkAsCollected()
{
	EvidenceData.bAlreadyCollected = true;
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}

bool ABalhwajeomEvidenceActor::RequestCameraTargetInfo_Implementation(
	FBalhwajeomCameraTargetInfo& OutInfo) const
{
	if (!bProgressionAvailable || bProgressionCleared || bProgressionRemovalPending)
	{
		return false;
	}

	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		FEvidenceStateDefinition State;
		if (Investigation->GetEvidenceStateDefinition(CurrentStateID, State))
		{
			OutInfo.EvidenceInstanceID = EvidenceInstanceID;
			OutInfo.ObjectID = ObjectID;
			OutInfo.StateID = CurrentStateID;
			OutInfo.PhotoID = State.PhotoID;
			OutInfo.bCanCapture = State.bCanCapture;
			OutInfo.MinimumFocusDistanceOffset = State.MinimumFocusDistanceOffset;
			OutInfo.MaximumFocusDistanceOffset = State.MaximumFocusDistanceOffset;
			OutInfo.MinimumCaptureScreenOccupancyRatioOverride =
				State.MinimumCaptureScreenOccupancyRatioOverride;
			OutInfo.PreferredFocusDistance = State.PreferredFocusDistance;
			OutInfo.FocusDistanceTolerance = State.FocusDistanceTolerance;
			OutInfo.bScaleFocusDistanceWithZoom = State.bScaleFocusDistanceWithZoom;
			OutInfo.EvidenceData = EvidenceData;
			return true;
		}
	}
	OutInfo.EvidenceData = EvidenceData;
	OutInfo.InformationStages = CameraInformationStages;
	OutInfo.bCanBeCaptured = bCanBeCaptured;
	OutInfo.MinimumFocusDistanceOffset = MinimumFocusDistanceOffset;
	OutInfo.MaximumFocusDistanceOffset = MaximumFocusDistanceOffset;
	OutInfo.MinimumCaptureScreenOccupancyRatioOverride =
		MinimumCaptureScreenOccupancyRatioOverride;
	OutInfo.PreferredFocusDistanceAt1x = PreferredFocusDistanceAt1x;
	OutInfo.FocusDistanceToleranceAt1x = FocusDistanceToleranceAt1x;
	OutInfo.bScaleFocusDistanceWithZoom = bScaleFocusDistanceWithZoom;
	return true;
}

FVector ABalhwajeomEvidenceActor::RequestCameraFocusLocation_Implementation() const
{
	return CameraFocusPoint ? CameraFocusPoint->GetComponentLocation() : GetActorLocation();
}

UPrimitiveComponent* ABalhwajeomEvidenceActor::RequestCameraFramingComponent_Implementation() const
{
	return EvidenceMesh;
}

void ABalhwajeomEvidenceActor::NotifyCameraCaptureSucceeded_Implementation()
{
	if (bProgressionAvailable && !bProgressionCleared && !bProgressionRemovalPending)
	{
		MarkAsCollected();
	}
}
