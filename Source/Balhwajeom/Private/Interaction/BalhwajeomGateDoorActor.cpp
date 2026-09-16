#include "Interaction/BalhwajeomGateDoorActor.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Interaction/InspectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Story/StoryStateSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"


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

	// WBP_Check owns its layout. Its message is refreshed from the ordered stage data
	// every time a locked interaction is requested.
	static ConstructorHelpers::FClassFinder<UUserWidget> LockedFeedbackClass(
		TEXT("/Game/Balhwajeom/UI/HUD/WBP_Check"));
	if (LockedFeedbackClass.Succeeded())
	{
		LockedFeedbackWidgetClass = LockedFeedbackClass.Class;
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
	if (DoorInteraction)
	{
		DoorInteraction->OnLockedInteractionRequested.AddUniqueDynamic(
			this, &ThisClass::HandleLockedInteractionRequested);
	}

	RefreshLabelForLockState();
}


void ABalhwajeomGateDoorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LockedFeedbackTimerHandle);
	}
	if (LockedFeedbackWidget)
	{
		LockedFeedbackWidget->RemoveFromParent();
		LockedFeedbackWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}


void ABalhwajeomGateDoorActor::HandleLockedInteractionRequested()
{
	const TSubclassOf<UUserWidget> FeedbackClass = ResolveLockedFeedbackWidgetClass();
	if (!FeedbackClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: locked interaction feedback has no widget class. Assign WBP_Check."),
			*GetName());
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return;
	}

	if (!LockedFeedbackWidget)
	{
		LockedFeedbackWidget = CreateWidget<UUserWidget>(
			PlayerController, FeedbackClass);
		if (!LockedFeedbackWidget)
		{
			return;
		}
		LockedFeedbackWidget->AddToViewport(FMath::Max(LockedFeedbackZOrder, 1100));
	}
	if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld()))
	{
		FGameViewportWidgetSlot Slot = ViewportSubsystem->GetWidgetSlot(LockedFeedbackWidget);
		// Older room instances may have serialized the previous value (30). Never let
		// that stale instance value put this message under the tutorial UI.
		Slot.ZOrder = FMath::Max(LockedFeedbackZOrder, 1100);
		ViewportSubsystem->SetWidgetSlot(LockedFeedbackWidget, Slot);
	}

	ApplyLockedFeedbackMessage(ResolveLockedFeedbackMessage());
	LockedFeedbackWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LockedFeedbackTimerHandle,
			this,
			&ThisClass::HideLockedFeedback,
			FMath::Max(0.1f, LockedFeedbackDisplayDuration),
			false);
	}
}


FText ABalhwajeomGateDoorActor::ResolveLockedFeedbackMessage() const
{
	if (LockedFeedbackStages.IsEmpty())
	{
		return FText::GetEmpty();
	}

	FGameplayTagContainer CurrentStateTags;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UStoryStateSubsystem* StoryState =
			GameInstance->GetSubsystem<UStoryStateSubsystem>())
		{
			CurrentStateTags = StoryState->GetCurrentStateTags();
		}
	}

	return GateDoorLockedFeedback::ResolveFirstIncompleteMessage(
		LockedFeedbackStages, CurrentStateTags);
}


void ABalhwajeomGateDoorActor::ApplyLockedFeedbackMessage(const FText& Message)
{
	if (!LockedFeedbackWidget || Message.IsEmptyOrWhitespace())
	{
		// No configured stages preserves the text authored in legacy WBP_Check assets.
		return;
	}

	if (UFunction* SetMessageTextFunction =
		LockedFeedbackWidget->FindFunction(TEXT("SetMessageText")))
	{
		struct FSetMessageTextParameters
		{
			FText NewText;
		};

		FSetMessageTextParameters Parameters{ Message };
		LockedFeedbackWidget->ProcessEvent(SetMessageTextFunction, &Parameters);
		return;
	}

	// The shipped WBP_Check predates SetMessageText and exposes its TextBlock as
	// "Text". MessageText is also supported for a future, explicitly named widget.
	static const FName CandidateNames[] = { TEXT("MessageText"), TEXT("Text") };
	for (const FName CandidateName : CandidateNames)
	{
		UWidget* MessageTarget = LockedFeedbackWidget->GetWidgetFromName(CandidateName);
		if (UTextBlock* MessageText = Cast<UTextBlock>(MessageTarget))
		{
			MessageText->SetText(Message);
			return;
		}

		if (MessageTarget)
		{
			if (UFunction* SetTextFunction = MessageTarget->FindFunction(TEXT("SetText")))
			{
				struct FSetTextParameters
				{
					FText InText;
				};

				FSetTextParameters Parameters{ Message };
				MessageTarget->ProcessEvent(SetTextFunction, &Parameters);
				return;
			}
		}
	}

	// Legacy WBP_Check currently has a single generated TextBlock name. Resolve that
	// sole text target without coupling runtime code to the generated numeric suffix.
	if (LockedFeedbackWidget->WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		LockedFeedbackWidget->WidgetTree->GetAllWidgets(AllWidgets);
		UTextBlock* SoleTextBlock = nullptr;
		for (UWidget* Widget : AllWidgets)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				if (SoleTextBlock)
				{
					SoleTextBlock = nullptr;
					break;
				}
				SoleTextBlock = TextBlock;
			}
		}
		if (SoleTextBlock)
		{
			SoleTextBlock->SetText(Message);
			return;
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s: WBP_Check has no unambiguous message text target."),
		*GetName());
}


TSubclassOf<UUserWidget> ABalhwajeomGateDoorActor::ResolveLockedFeedbackWidgetClass() const
{
	if (LockedFeedbackWidgetClass)
	{
		return LockedFeedbackWidgetClass;
	}

	// A newly created, unsaved WBP_Check still has a generated class in editor memory.
	// Resolve it by name so PIE does not depend on when this actor's CDO was constructed.
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UUserWidget::StaticClass()) &&
			It->GetName().Equals(TEXT("WBP_Check_C"), ESearchCase::IgnoreCase))
		{
			return *It;
		}
	}

	// FClassFinder in the constructor cannot see a widget created later during the same
	// editor session, so try the conventional path again at interaction time.
	const FString ConventionalPackage = TEXT("/Game/Balhwajeom/UI/HUD/WBP_Check");
	if (FPackageName::DoesPackageExist(ConventionalPackage))
	{
		if (UClass* LoadedClass = LoadClass<UUserWidget>(
			nullptr, TEXT("/Game/Balhwajeom/UI/HUD/WBP_Check.WBP_Check_C")))
		{
			return LoadedClass;
		}
	}

	return nullptr;
}


void ABalhwajeomGateDoorActor::HideLockedFeedback()
{
	if (LockedFeedbackWidget)
	{
		LockedFeedbackWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
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


void ABalhwajeomGateDoorActor::CheckDoorFullyOpenedBroadcast()
{
	if (bHasBroadcastDoorOpened || !DoorInteraction || !DoorInteraction->IsOpen())
	{
		return;
	}
	bHasBroadcastDoorOpened = true;
	OnDoorFullyOpened.Broadcast();
}


void ABalhwajeomGateDoorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The unlock condition is story state the door does not own, so there is nothing to
	// subscribe to that is cheaper than this throttled poll.
	RefreshLabelForLockState();
	CheckDoorFullyOpenedBroadcast();

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
