#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemInspection/JMItemInspectionTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "JMItemInspectionData.generated.h"

class UMaterialInterface;
class UStaticMesh;

UCLASS(BlueprintType)
class ITEMINSPECTORRUNTIME_API UJMItemInspectionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FText DisplayCategory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text", meta = (MultiLine = true))
	FText AdditionalInfo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	TSoftObjectPtr<UStaticMesh> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	FJMItemInspectionViewSettings ViewSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview|Transition")
	bool bOverrideTransitionSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview|Transition", meta = (EditCondition = "bOverrideTransitionSettings"))
	FJMItemInspectionTransitionSettings TransitionSettings;

	/** Optional dynamic UMG content rendered on the preview surface (notes, cards, screens). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	FJMItemInspectionSurfaceWidgetSettings SurfaceWidget;
};
