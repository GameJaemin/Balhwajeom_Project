#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "UObject/UnrealType.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTutorialOverlayDataTableTest,
	"Balhwajeom.Tutorial.Overlay.DataTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialOverlayDataTableTest::RunTest(const FString& Parameters)
{
	const UDataTable* Table = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_TutorialOverlay.DT_TutorialOverlay"));
	if (!TestNotNull(TEXT("DT_TutorialOverlay should load"), Table))
	{
		return false;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	if (!TestNotNull(TEXT("DT_TutorialOverlay should have a row struct"), RowStruct))
	{
		return false;
	}
	TestEqual(
		TEXT("DT_TutorialOverlay should use FTutorialOverlayDefinition"),
		RowStruct->GetFName(),
		FName(TEXT("TutorialOverlayDefinition")));

	const FStructProperty* RequiredTagsProperty =
		FindFProperty<FStructProperty>(RowStruct, TEXT("RequiredTags"));
	const FArrayProperty* ImagesProperty =
		FindFProperty<FArrayProperty>(RowStruct, TEXT("Images"));
	const FTextProperty* OverlayTextProperty =
		FindFProperty<FTextProperty>(RowStruct, TEXT("OverlayText"));
	const FStructProperty* CompletionTagProperty =
		FindFProperty<FStructProperty>(RowStruct, TEXT("CompletionTag"));

	TestTrue(
		TEXT("RequiredTags should be a GameplayTagContainer"),
		RequiredTagsProperty &&
			RequiredTagsProperty->Struct == FGameplayTagContainer::StaticStruct());
	const FSoftObjectProperty* ImageInnerProperty = ImagesProperty
		? CastField<FSoftObjectProperty>(ImagesProperty->Inner)
		: nullptr;
	TestTrue(
		TEXT("Images should be an array of soft Texture2D references"),
		ImageInnerProperty && ImageInnerProperty->PropertyClass == UTexture2D::StaticClass());
	TestNotNull(TEXT("OverlayText should be an FText property"), OverlayTextProperty);
	TestTrue(
		TEXT("CompletionTag should be a GameplayTag"),
		CompletionTagProperty &&
			CompletionTagProperty->Struct == FGameplayTag::StaticStruct());
	if (!RequiredTagsProperty || !ImagesProperty || !OverlayTextProperty ||
		!CompletionTagProperty)
	{
		return false;
	}

	const TArray<TPair<FName, FName>> ExpectedRows = {
		{ TEXT("StatementIntro"), TEXT("Tutorial.Overlay.Seen.StatementIntro") },
		{ TEXT("MovementAndLook"), TEXT("Tutorial.Overlay.Seen.MovementAndLook") },
		{ TEXT("Interaction"), TEXT("Tutorial.Overlay.Seen.Interaction") },
		{ TEXT("PhotoCamera"), TEXT("Tutorial.Overlay.Seen.PhotoCamera") },
		{ TEXT("MemoryObject"), TEXT("Tutorial.Overlay.Seen.MemoryObject") },
		{ TEXT("Tablet"), TEXT("Tutorial.Overlay.Seen.Tablet") },
		{ TEXT("SisterFolder"), TEXT("Tutorial.Overlay.Seen.SisterFolder") },
		{ TEXT("PhotoSentencePuzzle"), TEXT("Tutorial.Overlay.Seen.PhotoSentencePuzzle") }
	};

	TestEqual(TEXT("DT_TutorialOverlay should contain eight rows"),
		Table->GetRowMap().Num(), ExpectedRows.Num());
	for (const TPair<FName, FName>& Expected : ExpectedRows)
	{
		const FGameplayTag RegisteredTag = FGameplayTag::RequestGameplayTag(
			Expected.Value, false);
		TestTrue(
			FString::Printf(TEXT("Completion tag %s should be registered"),
				*Expected.Value.ToString()),
			RegisteredTag.IsValid());

		const uint8* RowData = Table->FindRowUnchecked(Expected.Key);
		if (!TestNotNull(
			FString::Printf(TEXT("Row %s should exist"), *Expected.Key.ToString()),
			RowData))
		{
			continue;
		}

		const FGameplayTag* CompletionTag =
			CompletionTagProperty->ContainerPtrToValuePtr<FGameplayTag>(RowData);
		TestEqual(
			FString::Printf(TEXT("Row %s should use its approved completion tag"),
				*Expected.Key.ToString()),
			CompletionTag ? CompletionTag->GetTagName() : NAME_None,
			Expected.Value);

		const FGameplayTagContainer* RequiredTags =
			RequiredTagsProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(RowData);
		TestTrue(
			FString::Printf(TEXT("Row %s should remain inactive until conditions are authored"),
				*Expected.Key.ToString()),
			RequiredTags && RequiredTags->IsEmpty());

		const FText* OverlayText =
			OverlayTextProperty->ContainerPtrToValuePtr<FText>(RowData);
		TestTrue(
			FString::Printf(TEXT("Row %s should contain initial overlay copy"),
				*Expected.Key.ToString()),
			OverlayText && !OverlayText->IsEmpty());
	}

	return true;
}

#endif
