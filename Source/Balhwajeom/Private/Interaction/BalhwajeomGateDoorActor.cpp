#include "Interaction/BalhwajeomGateDoorActor.h"

#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Interaction/InspectionComponent.h"
#include "UObject/ConstructorHelpers.h"


ABalhwajeomGateDoorActor::ABalhwajeomGateDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	SetRootComponent(DoorMesh);

	DoorInteraction = CreateDefaultSubobject<UDoorInteractionComponent>(
		TEXT("DoorInteraction"));

	InspectionComponent = CreateDefaultSubobject<UInspectionComponent>(
		TEXT("InspectionComponent"));

	ObjectLabelWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ObjectLabelWidget"));
	ObjectLabelWidget->SetupAttachment(DoorMesh);
	ObjectLabelWidget->SetRelativeLocation(FVector::ZeroVector);
	ObjectLabelWidget->SetWidgetSpace(EWidgetSpace::Screen);
	ObjectLabelWidget->SetDrawAtDesiredSize(true);
	ObjectLabelWidget->SetVisibility(false);
	ObjectLabelWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The same label widget the evidence actors use, so a door reads identically in game.
	static ConstructorHelpers::FClassFinder<UUserWidget> ObjectLabelWidgetClass(
		TEXT("/Game/Balhwajeom/UI/Inspection/WBP_ObjectLabel"));
	if (ObjectLabelWidgetClass.Succeeded())
	{
		ObjectLabelWidget->SetWidgetClass(ObjectLabelWidgetClass.Class);
	}

	// Label text is authored per instance, which keeps localized strings out of source
	// encoding entirely -- this module's files are not consistently UTF-8.
}


void ABalhwajeomGateDoorActor::BeginPlay()
{
	Super::BeginPlay();

	if (InspectionComponent)
	{
		InspectionComponent->OnPlayerDistanceStateChanged.AddUniqueDynamic(
			this, &ThisClass::HandlePlayerDistanceStateChanged);
	}

	RefreshLabelForLockState();
}


FText ABalhwajeomGateDoorActor::ResolveCurrentLabel() const
{
	if (!DoorInteraction)
	{
		return FText::GetEmpty();
	}

	if (DoorInteraction->IsOpen() || DoorInteraction->IsOpening())
	{
		return OpenedLabel;
	}

	return DoorInteraction->IsUnlocked() ? UnlockedLabel : LockedLabel;
}


void ABalhwajeomGateDoorActor::RefreshLabelForLockState()
{
	if (!DoorInteraction || !InspectionComponent)
	{
		return;
	}

	const bool bUnlocked = DoorInteraction->IsUnlocked();
	const bool bOpen = DoorInteraction->IsOpen() || DoorInteraction->IsOpening();
	if (bHasLabelState && bUnlocked == bLastKnownUnlocked && bOpen == bLastKnownOpen)
	{
		return;
	}

	bHasLabelState = true;
	bLastKnownUnlocked = bUnlocked;
	bLastKnownOpen = bOpen;

	// Only the close-range label changes; the door stays unremarkable from a distance.
	InspectionComponent->NearLabel = ResolveCurrentLabel();
	ApplyInspectionDistanceState(LastInspectionDistanceState);
}


void ABalhwajeomGateDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The unlock condition is story state the door does not own, so there is nothing to
	// subscribe to that is cheaper than this throttled poll.
	RefreshLabelForLockState();

	if (ObjectLabelWidget && DoorMesh)
	{
		const FVector WorldOffset =
			DoorMesh->GetComponentTransform().TransformVectorNoScale(ObjectLabelOffset);
		ObjectLabelWidget->SetWorldLocation(DoorMesh->Bounds.Origin + WorldOffset);
	}
}


void ABalhwajeomGateDoorActor::HandlePlayerDistanceStateChanged(
	EPlayerInspectionDistanceState NewState)
{
	LastInspectionDistanceState = NewState;
	ApplyInspectionDistanceState(NewState);
}


void ABalhwajeomGateDoorActor::ApplyInspectionDistanceState(
	EPlayerInspectionDistanceState DistanceState)
{
	if (!InspectionComponent)
	{
		SetInspectionLabel(FText::GetEmpty(), false);
		return;
	}

	switch (DistanceState)
	{
	case EPlayerInspectionDistanceState::Far:
		SetInspectionLabel(InspectionComponent->FarLabel, true);
		break;

	case EPlayerInspectionDistanceState::Middle:
		SetInspectionLabel(InspectionComponent->MidLabel, true);
		break;

	case EPlayerInspectionDistanceState::Close:
		SetInspectionLabel(InspectionComponent->NearLabel, true);
		break;

	case EPlayerInspectionDistanceState::OutOfRange:
	default:
		SetInspectionLabel(FText::GetEmpty(), false);
		break;
	}
}


void ABalhwajeomGateDoorActor::SetInspectionLabel(
	const FText& LabelText,
	bool bVisible)
{
	if (!ObjectLabelWidget)
	{
		return;
	}

	const bool bShouldDisplay = bVisible && !LabelText.IsEmptyOrWhitespace();
	ObjectLabelWidget->SetVisibility(bShouldDisplay);
	if (!bShouldDisplay)
	{
		return;
	}

	ObjectLabelWidget->InitWidget();
	UUserWidget* LabelWidget = ObjectLabelWidget->GetUserWidgetObject();
	if (!IsValid(LabelWidget))
	{
		return;
	}

	UFunction* SetLabelTextFunction = LabelWidget->FindFunction(TEXT("SetLabelText"));
	if (!SetLabelTextFunction)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: WBP_ObjectLabel does not provide SetLabelText(FText)."),
			*GetName());
		return;
	}

	struct FSetLabelTextParameters
	{
		FText NewText;
	};

	FSetLabelTextParameters Parameters{ LabelText };
	LabelWidget->ProcessEvent(SetLabelTextFunction, &Parameters);
}
