#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
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

	const FNameProperty* OverlayIDProperty =
		FindFProperty<FNameProperty>(RowStruct, TEXT("OverlayID"));
	const FStructProperty* RequiredTagsProperty =
		FindFProperty<FStructProperty>(RowStruct, TEXT("RequiredTags"));
	const FArrayProperty* ImagesProperty =
		FindFProperty<FArrayProperty>(RowStruct, TEXT("Images"));
	const FTextProperty* OverlayTitleProperty =
		FindFProperty<FTextProperty>(RowStruct, TEXT("OverlayTitle"));
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
	TestNotNull(TEXT("OverlayID should be an FName property"), OverlayIDProperty);
	TestNotNull(TEXT("OverlayTitle should be an FText property"), OverlayTitleProperty);
	TestNotNull(TEXT("OverlayText should be an FText property"), OverlayTextProperty);

	// The heading is one line; only the body is authored as multi-line.
	TestFalse(TEXT("OverlayTitle should be a single-line field"),
		OverlayTitleProperty &&
			OverlayTitleProperty->HasMetaData(TEXT("MultiLine")));
	TestTrue(TEXT("OverlayText should stay a multi-line field"),
		OverlayTextProperty &&
			OverlayTextProperty->GetMetaData(TEXT("MultiLine")) == TEXT("true"));
	TestTrue(
		TEXT("CompletionTag should be a GameplayTag"),
		CompletionTagProperty &&
			CompletionTagProperty->Struct == FGameplayTag::StaticStruct());
	if (!OverlayIDProperty || !RequiredTagsProperty || !ImagesProperty ||
		!OverlayTitleProperty || !OverlayTextProperty || !CompletionTagProperty)
	{
		return false;
	}

	// Every overlay image is managed in one folder, so the table never reaches into
	// per-feature asset folders.
	static const TCHAR* ImageFolder = TEXT("/Game/Balhwajeom/UI/Tutorial/Overlay/");

	// Rows that already have art. The rest stay empty until their images are drawn.
	const TMap<FName, int32> ExpectedImageCounts = { { TEXT("OVL_01_001"), 1 } };

	// Ordered IDs, and the Row Name repeated in OverlayID exactly as the other
	// investigation tables do it.
	const TArray<TPair<FName, FName>> ExpectedRows = {
		{ TEXT("OVL_01_001"), TEXT("Tutorial.Overlay.Seen.StatementIntro") },
		{ TEXT("OVL_01_002"), TEXT("Tutorial.Overlay.Seen.MovementAndLook") },
		{ TEXT("OVL_01_003"), TEXT("Tutorial.Overlay.Seen.Interaction") },
		{ TEXT("OVL_01_004"), TEXT("Tutorial.Overlay.Seen.PhotoCamera") },
		{ TEXT("OVL_01_005"), TEXT("Tutorial.Overlay.Seen.MemoryObject") },
		{ TEXT("OVL_01_006"), TEXT("Tutorial.Overlay.Seen.Tablet") },
		{ TEXT("OVL_01_007"), TEXT("Tutorial.Overlay.Seen.SisterFolder") },
		{ TEXT("OVL_01_008"), TEXT("Tutorial.Overlay.Seen.PhotoSentencePuzzle") }
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

		// What the conditions actually are, and that they chain in order, is covered by
		// Balhwajeom.Tutorial.Overlay.RequiredTags.
		const FGameplayTagContainer* RequiredTags =
			RequiredTagsProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(RowData);
		TestTrue(
			FString::Printf(TEXT("Row %s should have a trigger condition"),
				*Expected.Key.ToString()),
			RequiredTags && !RequiredTags->IsEmpty());

		const FName* OverlayID = OverlayIDProperty->ContainerPtrToValuePtr<FName>(RowData);
		TestEqual(
			FString::Printf(TEXT("Row %s should repeat its Row Name in OverlayID"),
				*Expected.Key.ToString()),
			OverlayID ? *OverlayID : NAME_None,
			Expected.Key);

		const FText* OverlayTitle =
			OverlayTitleProperty->ContainerPtrToValuePtr<FText>(RowData);
		TestTrue(
			FString::Printf(TEXT("Row %s should carry a heading"), *Expected.Key.ToString()),
			OverlayTitle && !OverlayTitle->IsEmpty());
		TestFalse(
			FString::Printf(TEXT("Row %s heading should stay on one line"),
				*Expected.Key.ToString()),
			OverlayTitle && OverlayTitle->ToString().Contains(TEXT("\n")));

		const FText* OverlayText =
			OverlayTextProperty->ContainerPtrToValuePtr<FText>(RowData);
		TestTrue(
			FString::Printf(TEXT("Row %s should contain initial overlay copy"),
				*Expected.Key.ToString()),
			OverlayText && !OverlayText->IsEmpty());

		const void* ImagesValue = ImagesProperty->ContainerPtrToValuePtr<void>(RowData);
		FScriptArrayHelper Images(ImagesProperty, ImagesValue);
		const int32 ExpectedImageCount =
			ExpectedImageCounts.FindRef(Expected.Key);
		TestEqual(
			FString::Printf(TEXT("Row %s should carry its authored images"),
				*Expected.Key.ToString()),
			Images.Num(), ExpectedImageCount);

		for (int32 ImageIndex = 0; ImageIndex < Images.Num(); ++ImageIndex)
		{
			const FSoftObjectPtr* Image = reinterpret_cast<const FSoftObjectPtr*>(
				Images.GetRawPtr(ImageIndex));
			const FString ImagePath = Image ? Image->ToSoftObjectPath().ToString() : FString();
			TestTrue(
				FString::Printf(TEXT("Row %s image %d should live in the shared folder"),
					*Expected.Key.ToString(), ImageIndex),
				ImagePath.StartsWith(ImageFolder));

			// A soft reference that no longer resolves would show as a blank overlay
			// rather than as any kind of error at runtime.
			TestNotNull(
				FString::Printf(TEXT("Row %s image %d should resolve"),
					*Expected.Key.ToString(), ImageIndex),
				Image ? LoadObject<UTexture2D>(nullptr, *ImagePath) : nullptr);
		}
	}

	return true;
}

#endif
