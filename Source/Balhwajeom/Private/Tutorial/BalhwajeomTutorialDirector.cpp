#include "Tutorial/BalhwajeomTutorialDirector.h"

#include "CameraSystem/BalhwajeomCameraPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/DoorInteractionComponent.h"
#include "Interaction/ItemInspectionIntegration.h"
#include "Interaction/InspectionComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"


ABalhwajeomTutorialDirector::ABalhwajeomTutorialDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}


ABalhwajeomTutorialDirector* ABalhwajeomTutorialDirector::GetTutorialDirector(
	const UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ABalhwajeomTutorialDirector> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}

	return nullptr;
}


float ABalhwajeomTutorialDirector::GetTutorialDimOpacity(const UObject* WorldContextObject)
{
	const ABalhwajeomTutorialDirector* Director = GetTutorialDirector(WorldContextObject);
	return Director ? Director->CalculateDimOpacity() : 0.0f;
}


EBalhwajeomTutorialHintTarget ABalhwajeomTutorialDirector::GetTutorialHintTarget(
	const UObject* WorldContextObject)
{
	const ABalhwajeomTutorialDirector* Director = GetTutorialDirector(WorldContextObject);
	const FBalhwajeomTutorialStep* Step = Director ? Director->GetCurrentStepPtr() : nullptr;

	// A mode that owns the whole screen also owns the icons, so stop highlighting them.
	if (!Step || Director->IsScreenOwnedByOtherMode())
	{
		return EBalhwajeomTutorialHintTarget::None;
	}

	return Step->HintTarget;
}


float ABalhwajeomTutorialDirector::CalculatePulseOpacity(
	float ElapsedSeconds,
	float PulsesPerSecond,
	float MinOpacity,
	float MaxOpacity)
{
	// Tolerating a swapped pair keeps a mis-authored level looking merely wrong rather
	// than leaving the highlight stuck invisible.
	const float Low = FMath::Clamp(FMath::Min(MinOpacity, MaxOpacity), 0.0f, 1.0f);
	const float High = FMath::Clamp(FMath::Max(MinOpacity, MaxOpacity), 0.0f, 1.0f);

	if (PulsesPerSecond <= 0.0f)
	{
		return High;
	}

	// Cosine, so the cycle begins at the bright end instead of halfway through.
	const float Phase = FMath::Cos(2.0f * UE_PI * PulsesPerSecond * ElapsedSeconds);

	return FMath::Lerp(Low, High, 0.5f + 0.5f * Phase);
}


float ABalhwajeomTutorialDirector::GetHighlightPulse() const
{
	const UWorld* World = GetWorld();
	const float Elapsed = World
		? static_cast<float>(World->GetTimeSeconds() - StepEnteredTimeSeconds)
		: 0.0f;

	return CalculatePulseOpacity(
		Elapsed,
		HighlightPulsesPerSecond,
		HighlightPulseMinOpacity,
		HighlightPulseMaxOpacity);
}


float ABalhwajeomTutorialDirector::GetTutorialHighlightPulse(
	const UObject* WorldContextObject)
{
	const ABalhwajeomTutorialDirector* Director = GetTutorialDirector(WorldContextObject);

	// Full brightness without a director, so a presenter can multiply by this
	// unconditionally and levels with no tutorial look untouched.
	return Director ? Director->GetHighlightPulse() : 1.0f;
}


void ABalhwajeomTutorialDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartOnBeginPlay)
	{
		StartFlow();
	}
}


void ABalhwajeomTutorialDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnsubscribeFromStoryState();

	Super::EndPlay(EndPlayReason);
}


UStoryStateSubsystem* ABalhwajeomTutorialDirector::GetStoryState() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<UStoryStateSubsystem>() : nullptr;
}


const FBalhwajeomTutorialStep* ABalhwajeomTutorialDirector::GetCurrentStepPtr() const
{
	if (!Flow || !Flow->Steps.IsValidIndex(CurrentStepIndex))
	{
		return nullptr;
	}

	return &Flow->Steps[CurrentStepIndex];
}


bool ABalhwajeomTutorialDirector::GetCurrentStep(FBalhwajeomTutorialStep& OutStep) const
{
	if (const FBalhwajeomTutorialStep* Step = GetCurrentStepPtr())
	{
		OutStep = *Step;
		return true;
	}

	OutStep = FBalhwajeomTutorialStep{};
	return false;
}


bool ABalhwajeomTutorialDirector::IsFlowActive() const
{
	return bFlowStarted && GetCurrentStepPtr() != nullptr;
}


FName ABalhwajeomTutorialDirector::GetCurrentStepID() const
{
	const FBalhwajeomTutorialStep* Step = GetCurrentStepPtr();
	return Step ? Step->StepID : NAME_None;
}


void ABalhwajeomTutorialDirector::SubscribeToStoryState()
{
	if (bSubscribed)
	{
		return;
	}

	UStoryStateSubsystem* StoryState = GetStoryState();
	if (!StoryState)
	{
		return;
	}

	// Both directions matter: a query can start matching because a tag arrived or
	// because one it excluded went away.
	StoryState->OnStateTagAdded.AddUniqueDynamic(this, &ThisClass::HandleStateTagChanged);
	StoryState->OnStateTagRemoved.AddUniqueDynamic(this, &ThisClass::HandleStateTagChanged);
	bSubscribed = true;
}


void ABalhwajeomTutorialDirector::UnsubscribeFromStoryState()
{
	if (!bSubscribed)
	{
		return;
	}

	if (UStoryStateSubsystem* StoryState = GetStoryState())
	{
		StoryState->OnStateTagAdded.RemoveDynamic(this, &ThisClass::HandleStateTagChanged);
		StoryState->OnStateTagRemoved.RemoveDynamic(this, &ThisClass::HandleStateTagChanged);
	}

	bSubscribed = false;
}


void ABalhwajeomTutorialDirector::SetFlow(UBalhwajeomTutorialFlow* InFlow)
{
	if (bFlowStarted)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: the flow is already running, so SetFlow was ignored."),
			*GetName());
		return;
	}

	Flow = InFlow;
}


void ABalhwajeomTutorialDirector::StartFlow()
{
	if (bFlowStarted)
	{
		return;
	}

	if (!Flow)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no Flow assigned, nothing to run."), *GetName());
		return;
	}

	UStoryStateSubsystem* StoryState = GetStoryState();
	if (!StoryState)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: UStoryStateSubsystem is unavailable, so the flow cannot run."),
			*GetName());
		return;
	}

	bFlowStarted = true;

	for (const FGameplayTag& LockTag : Flow->LocksOnStart)
	{
		StoryState->AddStateTag(LockTag);
	}

	SubscribeToStoryState();
	EnterStep(0);
}


void ABalhwajeomTutorialDirector::EnterStep(int32 StepIndex)
{
	if (!Flow)
	{
		return;
	}

	if (!Flow->Steps.IsValidIndex(StepIndex))
	{
		CurrentStepIndex = INDEX_NONE;
		FinishFlow();
		return;
	}

	CurrentStepIndex = StepIndex;
	const FBalhwajeomTutorialStep& Step = Flow->Steps[StepIndex];

	// Restarting the clock makes each new highlight begin at its brightest.
	if (const UWorld* World = GetWorld())
	{
		StepEnteredTimeSeconds = World->GetTimeSeconds();
	}

	UStoryStateSubsystem* StoryState = GetStoryState();
	if (StoryState)
	{
		TGuardValue<bool> EnteringGuard(bIsEnteringStep, true);

		if (Step.StageTag.IsValid() && Flow->StageRootTag.IsValid())
		{
			StoryState->SetExclusiveStateTag(Flow->StageRootTag, Step.StageTag);
		}

		// Removals first: a step that unlocks an ability and grants a tag in the same
		// entry should not be able to observe both the old lock and the new grant.
		for (const FGameplayTag& RemovedTag : Step.RemoveOnEnter)
		{
			StoryState->RemoveStateTag(RemovedTag);
		}
		for (const FGameplayTag& GrantedTag : Step.GrantOnEnter)
		{
			StoryState->AddStateTag(GrantedTag);
		}
	}

	OnStepChanged.Broadcast(Step);

	// Fast-forward: a step whose condition already holds completes immediately.
	EvaluateCurrentStep();
}


void ABalhwajeomTutorialDirector::EvaluateCurrentStep()
{
	if (bIsEnteringStep)
	{
		bPendingEvaluation = true;
		return;
	}

	do
	{
		bPendingEvaluation = false;

		const FBalhwajeomTutorialStep* Step = GetCurrentStepPtr();
		if (!Step || !Step->HasCompletionCondition() || !IsStepSatisfied(*Step))
		{
			return;
		}

		EnterStep(CurrentStepIndex + 1);
	}
	while (bPendingEvaluation);
}


bool ABalhwajeomTutorialDirector::IsStepSatisfied(
	const FBalhwajeomTutorialStep& Step) const
{
	const UStoryStateSubsystem* StoryState = GetStoryState();
	if (!StoryState)
	{
		return false;
	}

	if (!Step.CompleteWhenAllTags.IsEmpty() &&
		!StoryState->HasAllStateTags(Step.CompleteWhenAllTags))
	{
		return false;
	}

	if (!Step.CompleteWhenAnyTags.IsEmpty() &&
		!StoryState->HasAnyStateTags(Step.CompleteWhenAnyTags))
	{
		return false;
	}

	return Step.CompleteWhen.IsEmpty() ||
		StoryState->MatchesStateQuery(Step.CompleteWhen);
}


void ABalhwajeomTutorialDirector::HandleStateTagChanged(FGameplayTag StateTag)
{
	EvaluateCurrentStep();
}


void ABalhwajeomTutorialDirector::AdvanceStep()
{
	if (!bFlowStarted)
	{
		return;
	}

	EnterStep(CurrentStepIndex + 1);
}


void ABalhwajeomTutorialDirector::FinishFlow()
{
	CurrentStepIndex = INDEX_NONE;
	UnsubscribeFromStoryState();

	if (Flow && Flow->bReleaseLocksOnFinish)
	{
		if (UStoryStateSubsystem* StoryState = GetStoryState())
		{
			for (const FGameplayTag& LockTag : Flow->LocksOnStart)
			{
				StoryState->RemoveStateTag(LockTag);
			}
		}
	}

	OnFlowFinished.Broadcast();
}


void ABalhwajeomTutorialDirector::AbortFlow()
{
	// Releases the locks even when the flow opted out of automatic release, because
	// aborting is the recovery path and leaving the player locked out is never correct.
	CurrentStepIndex = INDEX_NONE;
	UnsubscribeFromStoryState();

	if (Flow)
	{
		if (UStoryStateSubsystem* StoryState = GetStoryState())
		{
			for (const FGameplayTag& LockTag : Flow->LocksOnStart)
			{
				StoryState->RemoveStateTag(LockTag);
			}
		}
	}

	OnFlowFinished.Broadcast();
}


void ABalhwajeomTutorialDirector::SetLockActive(FGameplayTag LockTag, bool bActive)
{
	UStoryStateSubsystem* StoryState = GetStoryState();
	if (!StoryState || !LockTag.IsValid())
	{
		return;
	}

	if (bActive)
	{
		StoryState->AddStateTag(LockTag);
	}
	else
	{
		StoryState->RemoveStateTag(LockTag);
	}
}


bool ABalhwajeomTutorialDirector::IsScreenOwnedByOtherMode() const
{
	const UStoryStateSubsystem* StoryState = GetStoryState();
	if (!StoryState)
	{
		return false;
	}

	// Fail open: only an explicitly non-exploration mode suppresses the dim. Camera mode,
	// tablet and every future mode set their tag on entry, so this cannot leave a dim
	// hanging over a full-screen mode, and an unset mode during startup still dims.
	if (StoryState->HasStateTag(BalhwajeomGameplayTags::Runtime_Player_Mode) &&
		!StoryState->HasStateTagExact(BalhwajeomGameplayTags::Runtime_Player_Mode_Exploration))
	{
		return true;
	}

	// The 3D item inspector is modal but has no runtime mode tag of its own.
	const APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(this, 0);

	return IsValid(PlayerController) && BalhwajeomItemInspection::IsOpen(PlayerController);
}


float ABalhwajeomTutorialDirector::CalculateDimOpacity() const
{
	const FBalhwajeomTutorialStep* Step = GetCurrentStepPtr();
	if (!Step || Step->DimMode == EBalhwajeomTutorialDimMode::Off)
	{
		return 0.0f;
	}

	if (IsScreenOwnedByOtherMode())
	{
		return 0.0f;
	}

	// Door feedback is WBP_Check, not part of the evidence-focus tutorial. Never dim
	// the screen merely because the player is looking at an interactable door.
	const APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(this, 0);
	const APawn* PlayerPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
	const UPlayerInteractionComponent* PlayerInteraction = IsValid(PlayerPawn)
		? PlayerPawn->FindComponentByClass<UPlayerInteractionComponent>()
		: nullptr;
	const UInspectionComponent* FocusedInspection = IsValid(PlayerInteraction)
		? PlayerInteraction->GetFocusedInspection()
		: nullptr;
	const AActor* FocusedActor = IsValid(FocusedInspection)
		? FocusedInspection->GetOwner()
		: nullptr;
	if (IsValid(FocusedActor) &&
		FocusedActor->FindComponentByClass<UDoorInteractionComponent>())
	{
		return 0.0f;
	}

	const float StepOpacity = FMath::Clamp(Step->DimOpacity, 0.0f, 1.0f);
	if (Step->DimMode == EBalhwajeomTutorialDimMode::Always)
	{
		return StepOpacity;
	}

	// FollowInteractPrompt: ride the prompt's own fade so the dim and the [F] text
	// appear and disappear together, with no second fade to keep in sync.
	const ABalhwajeomCameraPlayerController* CameraPlayerController =
		Cast<ABalhwajeomCameraPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0));
	if (!IsValid(CameraPlayerController))
	{
		return 0.0f;
	}

	return StepOpacity * FMath::Clamp(
		CameraPlayerController->GetInteractionPromptAlpha(), 0.0f, 1.0f);
}
