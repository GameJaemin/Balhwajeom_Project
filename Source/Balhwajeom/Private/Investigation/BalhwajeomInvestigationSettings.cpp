#include "Investigation/BalhwajeomInvestigationSettings.h"

#include "Story/StoryStateTags.h"

#if WITH_EDITOR
#include "Engine/DataTable.h"
#include "Investigation/EvidenceDefinitions.h"
#include "Misc/DataValidation.h"
#endif

FName UBalhwajeomInvestigationSettings::GetCategoryName() const
{
	return TEXT("Game");
}


FGameplayTag UBalhwajeomInvestigationSettings::ResolveChapter01RequiredActivationTag(
	const FName ObjectID,
	const FGameplayTag AuthoredActivationTag) const
{
	const bool bIsPhase01Object = Phase01ObjectIDs.Contains(ObjectID);
	const bool bIsPhase02Object = Phase02ObjectIDs.Contains(ObjectID);
	const bool bIsPhase03Object = Phase03ObjectIDs.Contains(ObjectID);
	const bool bIsConfiguredPhaseObject =
		bIsPhase01Object || bIsPhase02Object || bIsPhase03Object;

	if (!bEnableChapter01PhaseSystem)
	{
		return bIsConfiguredPhaseObject ? FGameplayTag() : AuthoredActivationTag;
	}

	if (bIsPhase01Object)
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Tutorial.Stage.Done"), false);
	}
	if (bIsPhase02Object)
	{
		return BalhwajeomGameplayTags::Story_Chapter_01_Phase_02_Unlocked;
	}
	if (bIsPhase03Object)
	{
		return BalhwajeomGameplayTags::Story_Chapter_01_Phase_03_Unlocked;
	}

	return AuthoredActivationTag;
}


#if WITH_EDITOR
EDataValidationResult UBalhwajeomInvestigationSettings::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context),
		EDataValidationResult::Valid);

	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	const UDataTable* Definitions = EvidenceDefinitionsTable.LoadSynchronous();
	if (!Definitions)
	{
		AddError(TEXT("Chapter 01 phase settings require a valid EvidenceDefinitionsTable."));
		return Result;
	}

	TSet<FName> SeenObjectIDs;
	auto ValidatePhase = [Definitions, &SeenObjectIDs, &AddError](
		const TCHAR* PhaseName,
		const TArray<FName>& ObjectIDs)
	{
		for (const FName ObjectID : ObjectIDs)
		{
			if (ObjectID.IsNone())
			{
				AddError(FString::Printf(
					TEXT("%s contains an empty ObjectID."), PhaseName));
				continue;
			}
			if (SeenObjectIDs.Contains(ObjectID))
			{
				AddError(FString::Printf(
					TEXT("ObjectID '%s' is assigned to more than one chapter 01 phase."),
					*ObjectID.ToString()));
				continue;
			}

			SeenObjectIDs.Add(ObjectID);
			if (!Definitions->FindRow<FEvidenceDefinition>(
				ObjectID, TEXT("Chapter01PhaseSettingsValidation"), false))
			{
				AddError(FString::Printf(
					TEXT("%s ObjectID '%s' does not exist in EvidenceDefinitionsTable."),
					PhaseName, *ObjectID.ToString()));
			}
		}
	};

	ValidatePhase(TEXT("Phase 01"), Phase01ObjectIDs);
	ValidatePhase(TEXT("Phase 02"), Phase02ObjectIDs);
	ValidatePhase(TEXT("Phase 03"), Phase03ObjectIDs);
	return Result;
}
#endif
