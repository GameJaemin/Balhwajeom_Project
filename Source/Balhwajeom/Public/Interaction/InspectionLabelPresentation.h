#pragma once

#include "CoreMinimal.h"

class UWidget;
class AActor;


namespace BalhwajeomInspectionLabelPresentation
{
	BALHWAJEOM_API void Calculate(
		float Distance,
		float MinimumPresentationDistance,
		float& OutScale,
		float& OutOpacity
	);

	BALHWAJEOM_API void ApplyToWidget(
		UWidget* LabelWidget,
		float Distance,
		float MinimumPresentationDistance
	);

	BALHWAJEOM_API void ApplyToActor(
		AActor* TargetActor,
		float Distance,
		float MinimumPresentationDistance
	);
}
