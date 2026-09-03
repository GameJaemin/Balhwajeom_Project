#include "Interaction/PlayerInteractionComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
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
	SetupInteractionInput();
}


void UPlayerInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInteractionInputInitialized)
	{
		SetupInteractionInput();
	}

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

	APlayerController* PlayerController =
		Cast<APlayerController>(PlayerActor->GetInstigatorController());

	if (!IsValid(PlayerController))
	{
		PlayerController = GetWorld()->GetFirstPlayerController();
	}

	if (!IsValid(PlayerController))
	{
		FocusedInspection = nullptr;
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	PlayerController->GetPlayerViewPoint(
		ViewLocation,
		ViewRotation
	);

	constexpr float FocusTraceDistance = 10000.0f;

	const FVector TraceEnd =
		ViewLocation +
		ViewRotation.Vector() * FocusTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerActor);

	FHitResult HitResult;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	UInspectionComponent* NewFocusedInspection = nullptr;

	if (bHit && IsValid(HitResult.GetActor()))
	{
		UInspectionComponent* HitInspection =
			HitResult.GetActor()->FindComponentByClass<UInspectionComponent>();

		NewFocusedInspection =
			ResolveFocusedInspection(HitInspection);
	}

	//FocusedInspection = NewFocusedInspection;
	SetFocusedInspection(NewFocusedInspection);
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

UInspectionComponent* UPlayerInteractionComponent::ResolveFocusedInspection(
	UInspectionComponent* HitInspection
) const
{
	if (!IsValid(HitInspection))
	{
		return nullptr;
	}

	const EPlayerInspectionDistanceState CurrentState =
		GetDistanceStateForInspectable(HitInspection);

	if (!CanInspectDistanceState(CurrentState))
	{
		return nullptr;
	}

	return HitInspection;
}


UInspectionComponent* UPlayerInteractionComponent::GetFocusedInspection() const
{
	return FocusedInspection;
}

void UPlayerInteractionComponent::SetFocusedInspection(
	UInspectionComponent* NewFocusedInspection
)
{
	FocusedInspection = NewFocusedInspection;
}


bool UPlayerInteractionComponent::TryInspect(
	FText& OutInspectionText
) 
{
	OutInspectionText = FText::GetEmpty();

	if (!IsValid(FocusedInspection))
	{
		return false;
	}

	const EPlayerInspectionDistanceState CurrentState =
		GetDistanceStateForInspectable(FocusedInspection);

	if (!CanInspectDistanceState(CurrentState))
	{
		return false;
	}

	OutInspectionText =
		FocusedInspection->InspectionText;

	return true;
}

bool UPlayerInteractionComponent::RequestInspect()
{
	FText InspectionText;

	if (!TryInspect(InspectionText))
	{
		return false;
	}

	OnInspectionSucceeded.Broadcast(InspectionText);

	return true;
}

bool UPlayerInteractionComponent::HandleInteractStarted()
{
	if (!IsValid(InteractAction.Get()))
	{
		return false;
	}

	return RequestInspect();
}

void UPlayerInteractionComponent::SetupInteractionInput()
{
	if (bInteractionInputInitialized)
	{
		return;
	}

	if (!IsValid(InteractionMappingContext.Get()) ||
		!IsValid(InteractAction.Get()))
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	if (!IsValid(OwnerPawn))
	{
		return;
	}

	APlayerController* PlayerController =
		Cast<APlayerController>(OwnerPawn->GetController());

	if (!IsValid(PlayerController) ||
		!PlayerController->IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer =
		PlayerController->GetLocalPlayer();

	if (!IsValid(LocalPlayer))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!IsValid(InputSubsystem))
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInputComponent =
		OwnerPawn->FindComponentByClass<UEnhancedInputComponent>();

	if (!IsValid(EnhancedInputComponent))
	{
		return;
	}

	InputSubsystem->AddMappingContext(
		InteractionMappingContext.Get(),
		InteractionMappingPriority
	);

	EnhancedInputComponent->BindAction(
		InteractAction.Get(),
		ETriggerEvent::Started,
		this,
		&UPlayerInteractionComponent::OnInteractActionStarted
	);

	bInteractionInputInitialized = true;
}


void UPlayerInteractionComponent::OnInteractActionStarted()
{
	HandleInteractStarted();
}