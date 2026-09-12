#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JMItemInspectionSurfaceWidgetBase.generated.h"

class UTextBlock;

/**
 * Base widget rendered on an inspected mesh surface.
 * A Blueprint may bind a TextBlock named SurfaceContentText, or leave its tree
 * empty and use the native readable fallback.
 */
UCLASS(BlueprintType, Blueprintable)
class ITEMINSPECTORRUNTIME_API UJMItemInspectionSurfaceWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "JM Gameplay|Item Inspection|Surface Widget")
	void SetSurfaceContent(const FText& InContent);

	UFUNCTION(BlueprintPure, Category = "JM Gameplay|Item Inspection|Surface Widget")
	FText GetSurfaceContent() const { return SurfaceContent; }

	UFUNCTION(BlueprintImplementableEvent, Category = "JM Gameplay|Item Inspection|Surface Widget")
	void OnSurfaceContentChanged(const FText& InContent);

protected:
	void BuildDefaultWidgetTreeIfNeeded();
	void ApplySurfaceContent();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "JM Gameplay|Item Inspection|Surface Widget")
	TObjectPtr<UTextBlock> SurfaceContentText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "JM Gameplay|Item Inspection|Surface Widget")
	FText SurfaceContent;
};
