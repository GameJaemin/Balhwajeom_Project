#include "Investigation/BalhwajeomInvestigationProgressNotifier.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Investigation/BalhwajeomInvestigationProgress.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Investigation/InvestigationRuntimeTypes.h"
#include "Investigation/PhotoDefinitions.h"
#include "UI/BalhwajeomNotificationPresenter.h"


UBalhwajeomInvestigationProgressNotifier::UBalhwajeomInvestigationProgressNotifier()
{
	PrimaryComponentTick.bCanEverTick = false;
	ReadyForStatementText = NSLOCTEXT(
		"Balhwajeom",
		"ReadyForStatement",
		"태블릿 속 진술서를 확인할 때가 온 것 같다..!");
}

UBalhwajeomInvestigationSubsystem*
UBalhwajeomInvestigationProgressNotifier::GetInvestigationSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
}

void UBalhwajeomInvestigationProgressNotifier::BeginPlay()
{
	Super::BeginPlay();

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation)
	{
		return;
	}

	// The two events that can complete the set. Evidence being photographed cannot finish it
	// on its own -- a captured photo still has its analysis sentence to solve.
	Investigation->OnWordAcquired.AddUniqueDynamic(this, &ThisClass::HandleWordAcquired);
	Investigation->OnSentenceSolved.AddUniqueDynamic(this, &ThisClass::HandleSentenceSolved);

	// Deliberately no evaluation here. A save loaded into an already-complete investigation
	// would otherwise announce itself on the opening frame, which reads as a bug rather than
	// a cue: the line is about the moment the last piece lands.
	bAnnounced = IsReadyForStatement();
}

void UBalhwajeomInvestigationProgressNotifier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem())
	{
		Investigation->OnWordAcquired.RemoveDynamic(this, &ThisClass::HandleWordAcquired);
		Investigation->OnSentenceSolved.RemoveDynamic(this, &ThisClass::HandleSentenceSolved);
	}
	Super::EndPlay(EndPlayReason);
}

void UBalhwajeomInvestigationProgressNotifier::HandleWordAcquired(
	const FAcquiredWordRecord& WordRecord)
{
	EvaluateProgress();
}

void UBalhwajeomInvestigationProgressNotifier::HandleSentenceSolved(const FName SentenceID)
{
	EvaluateProgress();
}

bool UBalhwajeomInvestigationProgressNotifier::IsReadyForStatement() const
{
	const UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation)
	{
		return false;
	}

	TArray<FAcquiredWordRecord> AcquiredWords;
	Investigation->GetAcquiredWords(AcquiredWords);

	// Only photos that actually carry an analysis sentence count -- a photo with no puzzle
	// has nothing to complete, and counting it would make the set unreachable.
	TArray<FPhotoDefinition> Photos;
	Investigation->GetAllPhotoDefinitions(Photos);
	int32 TotalPhotoSentences = 0;
	int32 SolvedPhotoSentences = 0;
	for (const FPhotoDefinition& Photo : Photos)
	{
		if (Photo.PhotoSentenceID.IsNone())
		{
			continue;
		}
		++TotalPhotoSentences;
		if (Investigation->IsSentenceSolved(Photo.PhotoSentenceID))
		{
			++SolvedPhotoSentences;
		}
	}

	return BalhwajeomInvestigationProgress::IsReadyForStatement(
		AcquiredWords.Num(),
		UBalhwajeomInvestigationSubsystem::DisplayedTotalKeywordCount,
		SolvedPhotoSentences,
		TotalPhotoSentences);
}

void UBalhwajeomInvestigationProgressNotifier::EvaluateProgress()
{
	if (bAnnounced || !IsReadyForStatement())
	{
		return;
	}

	bAnnounced = true;
	BalhwajeomNotification::Show(this, ReadyForStatementText);
}
