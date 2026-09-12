#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemInspection/JMItemInspectionTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "ItemInspectorSettings.generated.h"

class UJMItemInspectionWidgetBase;

UENUM(BlueprintType)
enum class EJMItemInspectionDuplicatePolicy : uint8
{
	RejectNewRequest,
	ReplaceCurrent
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Item Inspector", ToolTip = "Configure the reusable JM item inspection system."))
class ITEMINSPECTORRUNTIME_API UItemInspectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UItemInspectorSettings();

	static const UItemInspectorSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	TSoftClassPtr<UJMItemInspectionWidgetBase> DefaultInspectorWidgetClass;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	FIntPoint DefaultRenderTargetSize = FIntPoint(1024, 1024);

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection", meta = (ClampMin = "0.01"))
	float DefaultRotationSensitivity = 0.35f;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection", meta = (ClampMin = "0.01"))
	float DefaultZoomSpeed = 0.1f;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection", meta = (ClampMin = "5.0", ClampMax = "170.0", Units = "deg"))
	float DefaultPreviewFOV = 45.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	FLinearColor DefaultBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	bool bDefaultBlockPlayerInput = true;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	bool bDefaultPauseGame = false;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	EJMItemInspectionDuplicatePolicy DuplicatePolicy = EJMItemInspectionDuplicatePolicy::RejectNewRequest;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection")
	bool bDebugItemInspection = false;

	UPROPERTY(EditAnywhere, Config, Category = "Item Inspection|Transition")
	FJMItemInspectionTransitionSettings DefaultTransitionSettings;
};
