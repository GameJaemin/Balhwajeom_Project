#include "Interaction/InspectionLabelPresentation.h"

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"


namespace BalhwajeomInspectionLabelPresentation
{
	void Calculate(
		float Distance,
		float MinimumPresentationDistance,
		float& OutScale,
		float& OutOpacity
	)
	{
		// Object labels use a screen-space WidgetComponent, so their desired size is
		// already expressed in screen pixels. Scaling the rendered widget down here
		// resamples the completed Slate output (including the font atlas), which makes
		// distant text look low-resolution. Reducing opacity at the same time further
		// weakens glyph contrast. Keep the final presentation pixel-aligned and let the
		// inspection distance state decide which label content is visible.
		(void)Distance;
		(void)MinimumPresentationDistance;
		OutScale = 1.0f;
		OutOpacity = 1.0f;
	}

	void ApplyToWidget(
		UWidget* LabelWidget,
		float Distance,
		float MinimumPresentationDistance
	)
	{
		if (!IsValid(LabelWidget))
		{
			return;
		}

		float Scale = 1.0f;
		float Opacity = 1.0f;
		Calculate(Distance, MinimumPresentationDistance, Scale, Opacity);

		LabelWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		LabelWidget->SetRenderScale(FVector2D(Scale, Scale));
		LabelWidget->SetRenderOpacity(Opacity);
	}

	void ApplyToActor(
		AActor* TargetActor,
		float Distance,
		float MinimumPresentationDistance
	)
	{
		if (!IsValid(TargetActor))
		{
			return;
		}

		static const FString ObjectLabelClassPath =
			TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel.WBP_ObjectLabel_C");
		TInlineComponentArray<UWidgetComponent*> WidgetComponents(TargetActor);
		for (UWidgetComponent* WidgetComponent : WidgetComponents)
		{
			const UClass* WidgetClass = WidgetComponent
				? WidgetComponent->GetWidgetClass()
				: nullptr;
			if (!WidgetComponent || !WidgetComponent->IsVisible() ||
				!WidgetClass || WidgetClass->GetPathName() != ObjectLabelClassPath)
			{
				continue;
			}

			WidgetComponent->InitWidget();
			ApplyToWidget(
				WidgetComponent->GetUserWidgetObject(),
				Distance,
				MinimumPresentationDistance
			);
		}
	}
}
