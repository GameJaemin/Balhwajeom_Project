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
	ClearAutoAdvanceTimer();
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
	if (UWorld* World = GetWorld())
	{
		StepEnteredTimeSeconds = World->GetTimeSeconds();

		ClearAutoAdvanceTimer();
		if (Step.AutoAdvanceAfterSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				AutoAdvanceTimerHandle,
				this,
				&ThisClass::HandleAutoAdvanceTimer,
				Step.AutoAdvanceAfterSeconds,
				false);
		}
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


void ABalhwajeomTutorialDirector::HandleAutoAdvanceTimer()
{
	AdvanceStep();
}


void ABalhwajeomTutorialDirector::ClearAutoAdvanceTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	}
}


void ABalhwajeomTutorialDirector::FinishFlow()
{
	ClearAutoAdvanceTimer();
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
	ClearAutoAdvanceTimer();
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
