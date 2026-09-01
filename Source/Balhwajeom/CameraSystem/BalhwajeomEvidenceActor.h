// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BalhwajeomEvidenceTypes.h"
#include "BalhwajeomEvidenceActor.generated.h"

class UStaticMeshComponent;

/** A simple Blueprint-placeable object that can be discovered with the camera trace. */
UCLASS(Blueprintable)
class BALHWAJEOM_API ABalhwajeomEvidenceActor : public AActor
{
	GENERATED_BODY()

public:
	ABalhwajeomEvidenceActor();

	UFUNCTION(BlueprintPure, Category = "Evidence")
	FBalhwajeomEvidenceData GetEvidenceData() const { return EvidenceData; }

	UFUNCTION(BlueprintCallable, Category = "Evidence")
	void MarkAsCollected();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence")
	TObjectPtr<UStaticMeshComponent> EvidenceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evidence")
	FBalhwajeomEvidenceData EvidenceData;
};
