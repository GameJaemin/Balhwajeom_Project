#include "Tutorial/BalhwajeomTutorialOverlayPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Intro/BalhwajeomIntroFlowActor.h"
#include "Story/StoryStateSubsystem.h"
#include "Story/StoryStateTags.h"
#include "Tutorial/BalhwajeomTutorialOverlayInput.h"
#include "Tutorial/BalhwajeomTutorialOverlayQueue.h"
#include "Tutorial/BalhwajeomTutorialOverlayTriggers.h"
#include "Tutorial/BalhwajeomTutorialOverlayWidget.h"
#include "Tutorial/TutorialOverlayDefinitions.h"

namespace
{
	const TCHAR* DefaultOverlayTablePath =
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_TutorialOverlay.DT_TutorialOverlay");
	const TCHAR* DefaultOverlayWidgetPath =
		TEXT("/Game/Balhwajeom/UI/HUD/WBP_TutorialOverlay.WBP_TutorialOverlay_C");
	const TCHAR* OverlayContext = TEXT("BalhwajeomTutorialOverlayPresenter");
}


UBalhwajeomTutorialOverlayPresenter::UBalhwajeomTutorialOverlayPresenter()
{
	PrimaryComponentTick.bCanEverTick = true;
	OverlayTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(DefaultOverlayTablePath));
	OverlayWidgetClass = TSoftClassPtr<UBalhwajeomTutorialOverlayWidget>(
		FSoftObjectPath(DefaultOverlayWidgetPath));
}


void UBalhwajeomTutorialOverlayPresenter::BeginPlay()
{
	Super::BeginPlay();

	const APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	if (!OwningController || !OwningController->IsLocalController())
	{
		SetComponentTickEnabled(false);
		return;
	}

	LoadedOverlayTable = OverlayTable.LoadSynchronous();
	if (!LoadedOverlayTable)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Tutorial overlays are disabled: %s could not be loaded."),
			*OverlayTable.ToString());
		SetComponentTickEnabled(false);
		return;
	}

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UStoryStateSubsystem* StoryState =
			GameInstance->GetSubsystem<UStoryStateSubsystem>())
		{
			StoryState->OnStateTagAdded.AddDynamic(this, &ThisClass::HandleStateTagAdded);
		}
	}
}


void UBalhwajeomTutorialOverlayPresenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UStoryStateSubsystem* StoryState =
			GameInstance->GetSubsystem<UStoryStateSubsystem>())
		{
			StoryState->OnStateTagAdded.RemoveDynamic(this, &ThisClass::HandleStateTagAdded);
		}
	}

	// Leaving the level mid-overlay must not leave the player unable to move.
	ApplyInputBlock(false);
	if (OverlayWidget)
	{
		OverlayWidget->OnFadeOutFinished.RemoveAll(this);
		OverlayWidget->RemoveFromParent();
		OverlayWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}


void UBalhwajeomTutorialOverlayPresenter::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	StartWithoutIntroIfNeeded();
}


void UBalhwajeomTutorialOverlayPresenter::StartWithoutIntroIfNeeded()
{
	if (bCheckedForIntroActor)
	{
		return;
	}
	bCheckedForIntroActor = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Playing a level straight from the editor skips the intro, and with it the actor that
	// announces gameplay has started. Without this, no tutorial screen would ever appear
	// during ordinary iteration on the level.
	for (TActorIterator<ABalhwajeomIntroFlowActor> It(World); It; ++It)
	{
		return;
	}

	BalhwajeomTutorialOverlayTriggers::Set(
		this, BalhwajeomGameplayTags::Tutorial_Trigger_GameplayStarted);
}


void UBalhwajeomTutorialOverlayPresenter::HandleStateTagAdded(FGameplayTag StateTag)
{
	CollectDueRows();
	if (!IsOverlayOnScreen())
	{
		ShowNextQueuedRow();
	}
}


void UBalhwajeomTutorialOverlayPresenter::CollectDueRows()
{
	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UStoryStateSubsystem* StoryState = GameInstance
		? GameInstance->GetSubsystem<UStoryStateSubsystem>()
		: nullptr;
	if (!LoadedOverlayTable || !StoryState)
	{
		return;
	}

	TArray<BalhwajeomTutorialOverlayQueue::FCandidate> AllRows;
	for (const TPair<FName, uint8*>& Row : LoadedOverlayTable->GetRowMap())
	{
		AllRows.Add({ Row.Key, reinterpret_cast<const FTutorialOverlayDefinition*>(Row.Value) });
	}

	// The row map has no order of its own, and the tutorial is a sequence.
	AllRows.Sort([](const BalhwajeomTutorialOverlayQueue::FCandidate& Left,
		const BalhwajeomTutorialOverlayQueue::FCandidate& Right)
	{
		return Left.RowName.LexicalLess(Right.RowName);
	});

	TArray<FName> Excluded = QueuedRowNames;
	if (!ActiveRowName.IsNone())
	{
		Excluded.Add(ActiveRowName);
	}

	for (const BalhwajeomTutorialOverlayQueue::FCandidate& Due :
		BalhwajeomTutorialOverlayQueue::SelectDueRows(
			AllRows, StoryState->GetCurrentStateTags(), Excluded))
	{
		QueuedRowNames.Add(Due.RowName);
	}
}


void UBalhwajeomTutorialOverlayPresenter::ShowNextQueuedRow()
{
	if (QueuedRowNames.IsEmpty() || IsOverlayOnScreen() || !LoadedOverlayTable)
	{
		return;
	}

	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	if (!OwningController)
	{
		return;
	}

	const FName RowName = QueuedRowNames[0];
	QueuedRowNames.RemoveAt(0);

	const FTutorialOverlayDefinition* Definition =
		LoadedOverlayTable->FindRow<FTutorialOverlayDefinition>(RowName, OverlayContext, false);
	if (!Definition)
	{
		return;
	}

	if (!OverlayWidget)
	{
		TSubclassOf<UBalhwajeomTutorialOverlayWidget> WidgetClass =
			OverlayWidgetClass.LoadSynchronous();
		if (!WidgetClass)
		{
			WidgetClass = UBalhwajeomTutorialOverlayWidget::StaticClass();
		}
		OverlayWidget = CreateWidget<UBalhwajeomTutorialOverlayWidget>(
			OwningController, WidgetClass);
	}
	if (!OverlayWidget)
	{
		return;
	}

	ActiveRowName = RowName;
	ActiveCompletionTag = Definition->CompletionTag;

	OverlayWidget->OnFadeOutFinished.RemoveAll(this);
	OverlayWidget->OnFadeOutFinished.AddUObject(this, &ThisClass::HandleFadeOutFinished);
	OverlayWidget->SetInputLockSeconds(InputLockSeconds);
	OverlayWidget->Present(*Definition);
	OverlayWidget->AddToViewport(ViewportZOrder);
	OverlayWidget->BeginFadeIn();

	ApplyInputBlock(true);
}


void UBalhwajeomTutorialOverlayPresenter::RequestDismiss()
{
	if (!OverlayWidget || !OverlayWidget->CanAcceptDismissInput())
	{
		return;
	}

	OverlayWidget->BeginFadeOut();
}


void UBalhwajeomTutorialOverlayPresenter::HandleFadeOutFinished()
{
	FinishCurrentOverlay();

	// Marking the overlay as seen can make the next row due, so collect before showing.
	CollectDueRows();
	ShowNextQueuedRow();
}


void UBalhwajeomTutorialOverlayPresenter::FinishCurrentOverlay()
{
	const FGameplayTag CompletionTag = ActiveCompletionTag;
	ActiveCompletionTag = FGameplayTag();
	ActiveRowName = NAME_None;

	if (OverlayWidget)
	{
		OverlayWidget->OnFadeOutFinished.RemoveAll(this);
		OverlayWidget->RemoveFromParent();
	}
	ApplyInputBlock(false);

	// Recorded only once the screen is actually gone, so a row cannot be marked seen by a
	// dismissal that never finished playing out.
	if (CompletionTag.IsValid())
	{
		BalhwajeomTutorialOverlayTriggers::Set(this, CompletionTag);
	}
}


bool UBalhwajeomTutorialOverlayPresenter::IsOverlayOnScreen() const
{
	return IsValid(OverlayWidget) && OverlayWidget->IsInViewport();
}


void UBalhwajeomTutorialOverlayPresenter::ApplyInputBlock(const bool bBlocked)
{
	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	if (!OwningController || bBlocked == bInputBlockApplied)
	{
		return;
	}
	bInputBlockApplied = bBlocked;

	if (bBlocked)
	{
		if (FSlateApplication::IsInitialized())
		{
			InputProcessor = MakeShared<FTutorialOverlayInputProcessor>(this);
			FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
		}

		// Remembered rather than assumed: the tablet and the camera each leave their own
		// values behind, and restoring a guess would strand the player in the wrong mode.
		bRestoreMoveInput = !OwningController->IsMoveInputIgnored();
		bRestoreLookInput = !OwningController->IsLookInputIgnored();
		OwningController->SetIgnoreMoveInput(true);
		OwningController->SetIgnoreLookInput(true);

		// Anything already held goes down as released now, so the overlay starts from a
		// clean input state instead of inheriting a key the player is still leaning on.
		OwningController->FlushPressedKeys();
		return;
	}

	if (InputProcessor.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
		}
		InputProcessor.Reset();
	}

	// Flushed again on the way out so the key that dismissed the overlay cannot arrive in
	// gameplay as a fresh press once input is live again.
	OwningController->FlushPressedKeys();
	if (bRestoreMoveInput)
	{
		OwningController->SetIgnoreMoveInput(false);
	}
	if (bRestoreLookInput)
	{
		OwningController->SetIgnoreLookInput(false);
	}
	bRestoreMoveInput = false;
	bRestoreLookInput = false;
}
