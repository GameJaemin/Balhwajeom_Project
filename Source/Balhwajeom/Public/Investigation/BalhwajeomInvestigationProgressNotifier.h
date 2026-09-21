#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BalhwajeomInvestigationProgressNotifier.generated.h"

class UBalhwajeomInvestigationSubsystem;
struct FAcquiredWordRecord;


/**
 * Tells the player when the investigation has everything the statement needs.
 *
 * Without it the last keyword is just another keyword: nothing on screen says the
 * collecting phase is over and the tablet's statement is now the thing to do.
 *
 * Re-checked on the two events that can complete the set -- a keyword being acquired and a
 * photo's analysis sentence being solved -- rather than polled, and announced once.
 */
UCLASS(ClassGroup = (Balhwajeom), meta = (BlueprintSpawnableComponent))
class BALHWAJEOM_API UBalhwajeomInvestigationProgressNotifier : public UActorComponent
{
	GENERATED_BODY()

public:
	UBalhwajeomInvestigationProgressNotifier();

	/** True once every keyword is acquired and every photo's analysis sentence is solved. */
	UFUNCTION(BlueprintPure, Category = "Investigation|Progress")
	bool IsReadyForStatement() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Put across the top of the screen the first time the set is complete. Empty disables it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Investigation|Progress")
	FText ReadyForStatementText;

private:
	UFUNCTION()
	void HandleWordAcquired(const FAcquiredWordRecord& WordRecord);

	UFUNCTION()
	void HandleSentenceSolved(FName SentenceID);

	void EvaluateProgress();

	UBalhwajeomInvestigationSubsystem* GetInvestigationSubsystem() const;

	/** Announced once per session; a later re-check must not repeat it. */
	bool bAnnounced = false;
};
