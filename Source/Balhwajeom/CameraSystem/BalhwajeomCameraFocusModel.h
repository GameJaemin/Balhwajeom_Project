#pragma once

#include "CoreMinimal.h"

struct BALHWAJEOM_API FBalhwajeomFocusRange
{
	float Minimum = 0.0f;
	float Maximum = 0.0f;

	bool Contains(const float Distance) const
	{
		return Distance >= Minimum && Distance <= Maximum;
	}
};

struct BALHWAJEOM_API FBalhwajeomFocusRegion
{
	float FocalDistance = 0.0f;
	float SharpNear = 0.0f;
	float SharpFar = 0.0f;
};

struct BALHWAJEOM_API FBalhwajeomScreenFrameMetrics
{
	/** Fraction of the target's projected rectangle that remains inside the viewport. */
	float VisibleFraction = 0.0f;

	/** Fraction of the viewport occupied by the visible target rectangle. */
	float ScreenOccupancyRatio = 0.0f;
};

struct BALHWAJEOM_API FBalhwajeomFocusGraceState
{
	float MissElapsed = 0.0f;

	void OnStrictTargetFound()
	{
		MissElapsed = 0.0f;
	}

	bool ShouldRetainAfterMiss(const float DeltaSeconds, const float GracePeriod)
	{
		MissElapsed += FMath::Max(0.0f, DeltaSeconds);
		return MissElapsed + UE_KINDA_SMALL_NUMBER < FMath::Max(0.0f, GracePeriod);
	}
};

class BALHWAJEOM_API FBalhwajeomCameraFocusModel
{
public:
	static float CalculateVisualFocusDepth(
		const FVector& CameraLocation,
		const FVector& CameraForward,
		const FVector& FocusLocation);

	static bool CalculateEffectiveRange(
		float BaseMinimum,
		float BaseMaximum,
		float MinimumOffset,
		float MaximumOffset,
		FBalhwajeomFocusRange& OutRange);

	static FBalhwajeomFocusRegion CalculateFocusedRegion(
		float TargetDistance,
		float BlurStartDistance);

	static FBalhwajeomFocusRegion CalculateDefaultRegion(
		float MinimumDistance,
		float MaximumDistance);

	static FBalhwajeomFocusRegion InterpolateRegion(
		const FBalhwajeomFocusRegion& Current,
		const FBalhwajeomFocusRegion& Desired,
		float DeltaTime,
		float Speed);

	static float CalculateBlurStrength(
		float SceneDepth,
		const FBalhwajeomFocusRegion& FocusRegion,
		float BlurTransitionDistance,
		float MaximumBlurStrength);

	static bool CalculateScreenFrameMetrics(
		const FVector2D& ScreenMin,
		const FVector2D& ScreenMax,
		const FVector2D& ViewportSize,
		FBalhwajeomScreenFrameMetrics& OutMetrics);

	static bool IsScreenOccupancySufficient(
		float ScreenOccupancyRatio,
		float GlobalMinimumRatio,
		float OverrideMinimumRatio,
		float& OutRequiredRatio);
};
