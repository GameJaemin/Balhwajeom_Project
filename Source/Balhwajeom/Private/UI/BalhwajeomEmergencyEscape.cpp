#include "UI/BalhwajeomEmergencyEscape.h"


int32 BalhwajeomEmergencyEscape::ResolveNearestDoorIndex(
	const FVector& PlayerLocation,
	const TArray<FVector>& DoorLocations)
{
	int32 NearestIndex = INDEX_NONE;
	double NearestDistanceSquared = 0.0;

	for (int32 Index = 0; Index < DoorLocations.Num(); ++Index)
	{
		const double DistanceSquared = FVector::DistSquared(PlayerLocation, DoorLocations[Index]);
		// Strictly less than, so an exact tie keeps the earlier index and the destination
		// does not change between runs with a different actor iteration order.
		if (NearestIndex == INDEX_NONE || DistanceSquared < NearestDistanceSquared)
		{
			NearestIndex = Index;
			NearestDistanceSquared = DistanceSquared;
		}
	}

	return NearestIndex;
}


FTransform BalhwajeomEmergencyEscape::ResolveEscapeTransform(
	const FTransform& DoorTransform,
	const FVector& LocalOffset,
	const float LocalYaw)
{
	const FVector EscapeLocation = DoorTransform.TransformPosition(LocalOffset);

	// Yaw only, and taken from the door rather than the actor's full rotation: a door placed
	// with any pitch or roll must still leave the character capsule upright.
	FRotator EscapeRotation = FRotator::ZeroRotator;
	EscapeRotation.Yaw = FMath::UnwindDegrees(DoorTransform.Rotator().Yaw + LocalYaw);

	return FTransform(EscapeRotation, EscapeLocation);
}
