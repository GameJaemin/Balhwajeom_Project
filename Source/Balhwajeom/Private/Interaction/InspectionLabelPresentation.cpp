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
		constexpr float ClosestScale = 1.0f;
		constexpr float FarthestScale = 0.55f;
		constexpr float ClosestOpacity = 1.0f;
		constexpr float FarthestOpacity = 0.7f;
		constexpr float EaseExponent = 2.0f;

		const float DistanceRatio = MinimumPresentationDistance > KINDA_SMALL_NUMBER
			? FMath::Clamp(Distance / MinimumPresentationDistance, 0.0f, 1.0f)
			: 1.0f;
		const float EasedRatio = FMath::InterpEaseInOut(
			0.0f,
			1.0f,
			DistanceRatio,
			EaseExponent
		);

		OutScale = FMath::Lerp(ClosestScale, FarthestScale, EasedRatio);
		OutOpacity = FMath::Lerp(ClosestOpacity, FarthestOpacity, EasedRatio);
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
