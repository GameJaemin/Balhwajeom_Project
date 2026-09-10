#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "MultiShadowText.generated.h"


USTRUCT(BlueprintType)
struct MULTISHADOWTEXT_API FMultiShadowLayer
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow")
	FVector2D Offset = FVector2D::ZeroVector;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shadow",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float Blur = 4.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shadow",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float Spread = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow")
	FLinearColor Color = FLinearColor::Black;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Shadow",
		meta = (
			ClampMin = "0.0",
			ClampMax = "1.0",
			UIMin = "0.0",
			UIMax = "1.0"
		)
	)
	float Opacity = 0.62f;
};


UCLASS(meta = (DisplayName = "Multi Shadow Text"))
class MULTISHADOWTEXT_API UMultiShadowTextWidget : public UWidget
{
	GENERATED_BODY()

public:

	UMultiShadowTextWidget();


	// ============================================================
	// TEXT
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	TEnumAsByte<ETextJustify::Type> Justification = ETextJustify::Left;


	// ============================================================
	// WRAPPING
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wrapping")
	bool bAutoWrapText = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Wrapping",
		meta = (ClampMin = "0.0", UIMin = "0.0")
	)
	float WrapTextAt = 0.0f;


	// ============================================================
	// SHADOWS
	// ============================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multi Shadow")
	TArray<FMultiShadowLayer> ShadowLayers;


protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void SynchronizeProperties() override;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;


#if WITH_EDITOR

	virtual const FText GetPaletteCategory() override;

#endif


private:

	TSharedPtr<class SOverlay> MyOverlay;

	void RebuildShadowWidgets();
};