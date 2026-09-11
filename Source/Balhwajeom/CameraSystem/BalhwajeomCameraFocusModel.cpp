#include "CameraSystem/BalhwajeomCameraFocusModel.h"

float FBalhwajeomCameraFocusModel::CalculateVisualFocusDepth(
	const FVector& CameraLocation,
	const FVector& CameraForward,
	const FVector& FocusLocation)
{
	return FMath::Max(0.0f, FVector::DotProduct(
		FocusLocation - CameraLocation,
		CameraForward.GetSafeNormal()));
}

bool FBalhwajeomCameraFocusModel::CalculateEffectiveRange(
	const float BaseMinimum,
	const float BaseMaximum,
	const float MinimumOffset,
	const float MaximumOffset,
	FBalhwajeomFocusRange& OutRange)
{
	OutRange.Minimum = FMath::Max(0.0f, BaseMinimum + MinimumOffset);
	OutRange.Maximum = FMath::Max(0.0f, BaseMaximum + MaximumOffset);
	return OutRange.Minimum <= OutRange.Maximum;
}

FBalhwajeomFocusRegion FBalhwajeomCameraFocusModel::CalculateFocusedRegion(
	const float TargetDistance,
	const float BlurStartDistance)
{
	const float SafeFocalDistance = FMath::Max(0.0f, TargetDistance);
	const float SafeHalfWidth = FMath::Max(0.0f, BlurStartDistance);

	FBalhwajeomFocusRegion Result;
	Result.FocalDistance = SafeFocalDistance;
	Result.SharpNear = FMath::Max(0.0f, SafeFocalDistance - SafeHalfWidth);
	Result.SharpFar = SafeFocalDistance + SafeHalfWidth;
	return Result;
}

FBalhwajeomFocusRegion FBalhwajeomCameraFocusModel::CalculateDefaultRegion(
	const float MinimumDistance,
	const float MaximumDistance)
{
	FBalhwajeomFocusRegion Result;
	Result.SharpNear = FMath::Max(0.0f, MinimumDistance);
	Result.SharpFar = FMath::Max(Result.SharpNear, MaximumDistance);
	Result.FocalDistance = (Result.SharpNear + Result.SharpFar) * 0.5f;
	return Result;
}

FBalhwajeomFocusRegion FBalhwajeomCameraFocusModel::InterpolateRegion(
	const FBalhwajeomFocusRegion& Current,
	const FBalhwajeomFocusRegion& Desired,
	const float DeltaTime,
	const float Speed)
{
	if (DeltaTime <= 0.0f || Speed <= 0.0f)
	{
		return Desired;
	}

	const float CurrentHalfWidth = FMath::Max(
		0.0f, (Current.SharpFar - Current.SharpNear) * 0.5f);
	const float DesiredHalfWidth = FMath::Max(
		0.0f, (Desired.SharpFar - Desired.SharpNear) * 0.5f);

	FBalhwajeomFocusRegion Result;
	Result.FocalDistance = FMath::FInterpTo(
		Current.FocalDistance, Desired.FocalDistance, DeltaTime, Speed);
	const float HalfWidth = FMath::FInterpTo(
		CurrentHalfWidth, DesiredHalfWidth, DeltaTime, Speed);
	Result.SharpNear = FMath::Max(0.0f, Result.FocalDistance - HalfWidth);
	Result.SharpFar = Result.FocalDistance + HalfWidth;
	return Result;
}

float FBalhwajeomCameraFocusModel::CalculateBlurStrength(
	const float SceneDepth,
	const FBalhwajeomFocusRegion& FocusRegion,
	const float BlurTransitionDistance,
	const float MaximumBlurStrength)
{
	const float SafeMaximum = FMath::Clamp(MaximumBlurStrength, 0.0f, 1.0f);
	const float DistanceOutsideSharpRegion = FMath::Max3(
		FocusRegion.SharpNear - SceneDepth,
		SceneDepth - FocusRegion.SharpFar,
		0.0f);

	if (DistanceOutsideSharpRegion <= 0.0f)
	{
		return 0.0f;
	}

	if (BlurTransitionDistance <= UE_KINDA_SMALL_NUMBER)
	{
		return SafeMaximum;
	}

	const float Progress = FMath::Clamp(
		DistanceOutsideSharpRegion / BlurTransitionDistance,
		0.0f,
		1.0f);
	return Progress * Progress * SafeMaximum;
}
