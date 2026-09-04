// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Interaction/InspectionComponent.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomEvidenceActor::ABalhwajeomEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EvidenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EvidenceMesh"));
	SetRootComponent(EvidenceMesh);
	EvidenceMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

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
