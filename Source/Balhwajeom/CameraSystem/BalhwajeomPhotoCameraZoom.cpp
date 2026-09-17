#include "BalhwajeomPhotoCameraZoom.h"

#include "Math/UnrealMathUtility.h"


float BalhwajeomPhotoCameraZoom::ResolveZoomAlpha(
	const float FieldOfView,
	const float MinFieldOfView,
	const float MaxFieldOfView)
{
	const float Narrowest = FMath::Min(MinFieldOfView, MaxFieldOfView);
	const float Widest = FMath::Max(MinFieldOfView, MaxFieldOfView);
	const float Range = Widest - Narrowest;
	if (Range <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp((Widest - FieldOfView) / Range, 0.0f, 1.0f);
}


float BalhwajeomPhotoCameraZoom::ResolveZoomBarPosition(
	const float ZoomAlpha,
	const float MinZoomPosition,
	const float MaxZoomPosition)
{
	return FMath::Lerp(
		MinZoomPosition,
		MaxZoomPosition,
		FMath::Clamp(ZoomAlpha, 0.0f, 1.0f));
}
