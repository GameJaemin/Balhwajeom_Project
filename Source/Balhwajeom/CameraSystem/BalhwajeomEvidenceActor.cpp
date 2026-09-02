// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomEvidenceActor::ABalhwajeomEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EvidenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EvidenceMesh"));
	SetRootComponent(EvidenceMesh);
	EvidenceMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

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

void ABalhwajeomEvidenceActor::NotifyCameraCaptureSucceeded_Implementation()
{
	MarkAsCollected();
}
