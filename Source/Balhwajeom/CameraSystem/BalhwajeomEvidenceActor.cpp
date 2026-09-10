// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Interaction/InspectionComponent.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Engine/GameInstance.h"

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

	ObjectLabelWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ObjectLabelWidget"));
	ObjectLabelWidget->SetupAttachment(EvidenceMesh);
	ObjectLabelWidget->SetRelativeLocation(FVector::ZeroVector);
	ObjectLabelWidget->SetWidgetSpace(EWidgetSpace::Screen);
	ObjectLabelWidget->SetDrawAtDesiredSize(true);
	ObjectLabelWidget->SetVisibility(false);
	ObjectLabelWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<UUserWidget> ObjectLabelWidgetClass(
		TEXT("/Game/Balhwajeom/Blueprints/UI/WBP_ObjectLabel"));
	if (ObjectLabelWidgetClass.Succeeded())
	{
		ObjectLabelWidget->SetWidgetClass(ObjectLabelWidgetClass.Class);
	}

	CameraFocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CameraFocusPoint"));
	CameraFocusPoint->SetupAttachment(EvidenceMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMesh.Succeeded())
	{
		EvidenceMesh->SetStaticMesh(DefaultMesh.Object);
		EvidenceMesh->SetRelativeScale3D(FVector(0.5f));
	}
}

void ABalhwajeomEvidenceActor::BeginPlay()
{
	Super::BeginPlay();

	if (EvidenceMesh && CameraTargetBounds)
	{
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

	if (ObjectLabelWidget)
	{
		// Bounds.Origin is the visible mesh center even when the mesh asset's pivot is off-center.
		const FVector WorldOffset = EvidenceMesh
			? EvidenceMesh->GetComponentTransform().TransformVectorNoScale(ObjectLabelOffset)
			: GetActorTransform().TransformVectorNoScale(ObjectLabelOffset);
		const FVector LabelCenter = EvidenceMesh ? EvidenceMesh->Bounds.Origin : GetActorLocation();
		ObjectLabelWidget->SetWorldLocation(LabelCenter + WorldOffset);
		ObjectLabelWidget->SetVisibility(false);
	}

	if (InspectionComponent)
	{
		InspectionComponent->OnPlayerDistanceStateChanged.AddDynamic(
			this,
			&ABalhwajeomEvidenceActor::HandlePlayerDistanceStateChanged);
	}

	RegisterWithInvestigationSystem();
}

void ABalhwajeomEvidenceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		Investigation->OnEvidenceStateChanged.RemoveDynamic(
			this, &ABalhwajeomEvidenceActor::HandleEvidenceStateChanged);
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
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceInteractionViewData ViewData;
	if (!Investigation || !Investigation->BeginEvidenceInteraction(EvidenceInstanceID, ViewData))
	{
		return false;
	}

	OutDisplayText = ViewData.InteractionText;
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
	return Investigation->CompleteEvidenceInteraction(EvidenceInstanceID, ViewData.StateID);
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
	ApplyInvestigationState(CurrentStateID);
}

void ABalhwajeomEvidenceActor::HandleEvidenceStateChanged(
	FGuid ChangedInstanceID, FName PreviousStateID, FName NewStateID)
{
	if (ChangedInstanceID == EvidenceInstanceID)
	{
		ApplyInvestigationState(NewStateID);
	}
}

void ABalhwajeomEvidenceActor::ApplyInvestigationState(FName StateID)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FEvidenceStateDefinition State;
	if (!Investigation || !Investigation->GetEvidenceStateDefinition(StateID, State))
	{
		return;
	}
	CurrentStateID = State.StateID;
	bCanBeCaptured = State.bCanCapture;
	EvidenceData.bAlreadyCollected = !State.PhotoID.IsNone() &&
		Investigation->HasCapturedPhoto(State.PhotoID);
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
	}
}

void ABalhwajeomEvidenceActor::HandlePlayerDistanceStateChanged(
	EPlayerInspectionDistanceState NewState)
{
	LastInspectionDistanceState = NewState;
	ApplyInspectionDistanceState(NewState);
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

void ABalhwajeomEvidenceActor::SetInspectionLabel(
	const FText& LabelText,
	bool bVisible)
{
	if (!ObjectLabelWidget)
	{
		return;
	}

	const bool bShouldDisplay = bVisible && !LabelText.IsEmptyOrWhitespace();
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

	const FText StatusText = EvidenceData.bAlreadyCollected
		? FText::FromString(TEXT("V"))
		: FText::FromString(TEXT("?"));

	FText DisplayText;
	if (LastInspectionDistanceState == EPlayerInspectionDistanceState::Far) {
		DisplayText = FText::Format(
			NSLOCTEXT(
				"Evidence",
				"InspectionLabelWithStatus",
				"{0}"
			),
			StatusText
		);
	}
	else {
		DisplayText = FText::Format(
			NSLOCTEXT(
				"Evidence",
				"InspectionLabelWithStatus",
				"{0}  {1}"
			),
			StatusText,
			LabelText
		);
	}

	FSetLabelTextParameters Parameters{ DisplayText };
	LabelWidget->ProcessEvent(SetLabelTextFunction, &Parameters);
}

void ABalhwajeomEvidenceActor::MarkAsCollected()
{
	EvidenceData.bAlreadyCollected = true;
}

bool ABalhwajeomEvidenceActor::RequestCameraTargetInfo_Implementation(
	FBalhwajeomCameraTargetInfo& OutInfo) const
{
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
	MarkAsCollected();
}
