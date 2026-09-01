// Copyright Epic Games, Inc. All Rights Reserved.

#include "BalhwajeomEvidenceActor.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABalhwajeomEvidenceActor::ABalhwajeomEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EvidenceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EvidenceMesh"));
	SetRootComponent(EvidenceMesh);
	EvidenceMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

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
