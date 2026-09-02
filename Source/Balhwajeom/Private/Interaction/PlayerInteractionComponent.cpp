#include "Interaction/PlayerInteractionComponent.h"

#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Interaction/InspectionComponent.h"


UPlayerInteractionComponent::UPlayerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


EPlayerInspectionDistanceState UPlayerInteractionComponent::ClassifyDistance(
	float Distance,
	float CloseDistance,
	float MiddleDistance,
	float MaxDisplayDistance
)
{
	if (Distance <= CloseDistance)
	{
		return EPlayerInspectionDistanceState::Close;
	}

	if (Distance <= MiddleDistance)
	{
		return EPlayerInspectionDistanceState::Middle;
	}

	if (Distance <= MaxDisplayDistance)
	{
		return EPlayerInspectionDistanceState::Far;
	}

	return EPlayerInspectionDistanceState::OutOfRange;
}


EPlayerInspectionDistanceState UPlayerInteractionComponent::ClassifyDistanceBetweenPoints(
	const FVector& PlayerLocation,
	const FVector& TargetBoundsCenter,
	float CloseDistance,
	float MiddleDistance,
	float MaxDisplayDistance
)
{
	const float Distance = FVector::Distance(
		PlayerLocation,
		TargetBoundsCenter
	);

	return ClassifyDistance(
		Distance,
		CloseDistance,
		MiddleDistance,
		MaxDisplayDistance
	);
}


bool UPlayerInteractionComponent::CanInspectDistanceState(
	EPlayerInspectionDistanceState DistanceState
)
{
	return DistanceState == EPlayerInspectionDistanceState::Close;
}


bool UPlayerInteractionComponent::UpdateDistanceStateForInspectable(
	UInspectionComponent* InspectionComponent,
	const FVector& PlayerLocation,
	const FVector& TargetBoundsCenter
)
{
	if (!IsValid(InspectionComponent))
	{
		return false;
	}

	const EPlayerInspectionDistanceState NewDistanceState =
		ClassifyDistanceBetweenPoints(
			PlayerLocation,
			TargetBoundsCenter,
			InspectionComponent->CloseDistance,
			InspectionComponent->MiddleDistance,
			InspectionComponent->MaxDisplayDistance
		);

	const EPlayerInspectionDistanceState* PreviousDistanceState =
		DistanceStates.Find(InspectionComponent);

	const bool bStateChanged =
		PreviousDistanceState == nullptr ||
		*PreviousDistanceState != NewDistanceState;

	DistanceStates.FindOrAdd(InspectionComponent) = NewDistanceState;

	if (bStateChanged)
	{
		InspectionComponent->NotifyPlayerDistanceStateChanged(
			NewDistanceState
		);
	}

	return bStateChanged;
}

EPlayerInspectionDistanceState UPlayerInteractionComponent::GetDistanceStateForInspectable(
	UInspectionComponent* InspectionComponent
) const
{
	if (!IsValid(InspectionComponent))
	{
		return EPlayerInspectionDistanceState::OutOfRange;
	}

	const EPlayerInspectionDistanceState* FoundState =
		DistanceStates.Find(InspectionComponent);

	if (FoundState != nullptr)
	{
		return *FoundState;
	}

	return EPlayerInspectionDistanceState::OutOfRange;
}


void UPlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshInspectableObjects();
}


void UPlayerInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* PlayerActor = GetOwner();

	if (!IsValid(PlayerActor))
	{
		return;
	}

	const FVector PlayerLocation = PlayerActor->GetActorLocation();

	for (const TObjectPtr<UInspectionComponent>& InspectionPtr : InspectableObjects)
	{
		UInspectionComponent* InspectionComponent = InspectionPtr.Get();

		if (!IsValid(InspectionComponent))
		{
			continue;
		}

		AActor* TargetActor = InspectionComponent->GetOwner();

		if (!IsValid(TargetActor) || TargetActor == PlayerActor)
		{
			continue;
		}

		FVector BoundsOrigin;
		FVector BoundsExtent;

		TargetActor->GetActorBounds(
			true,
			BoundsOrigin,
			BoundsExtent
		);

		UpdateDistanceStateForInspectable(
			InspectionComponent,
			PlayerLocation,
			BoundsOrigin
		);
	}
}


void UPlayerInteractionComponent::RefreshInspectableObjects()
{
	InspectableObjects.Reset();
	DistanceStates.Reset();

	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;

		if (!IsValid(Actor))
		{
			continue;
		}

		UInspectionComponent* InspectionComponent =
			Actor->FindComponentByClass<UInspectionComponent>();

		if (IsValid(InspectionComponent))
		{
			InspectableObjects.Add(InspectionComponent);
		}
	}
}


int32 UPlayerInteractionComponent::GetInspectableObjectCount() const
{
	return InspectableObjects.Num();
}