#include "Tablet/BalhwajeomTabletWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Tablet/BalhwajeomMessengerWidget.h"
#include "Tablet/BalhwajeomInternetWidget.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	int32 ToPageIndex(const ETabletPage Page)
	{
		return static_cast<int32>(Page);
	}

	/** Strips a UButton's default gray chrome/padding so only its custom content shows. */
	void MakeButtonTransparent(UButton* Button)
	{
		if (!Button)
		{
			return;
		}
		FSlateBrush InvisibleBrush;
		InvisibleBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(InvisibleBrush);
		Style.SetHovered(InvisibleBrush);
		Style.SetPressed(InvisibleBrush);
		Style.SetDisabled(InvisibleBrush);
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(0.0f));
		Button->SetStyle(Style);
	}
}

void UBalhwajeomTabletPersonFolderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BTN_FolderClose)
	{
		BTN_FolderClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void UBalhwajeomTabletPersonFolderWidget::SetFolderHeader(
	const FText& FolderName,
	UTexture2D* FolderIcon)
{
	if (TXT_FolderTitle)
	{
		TXT_FolderTitle->SetText(FolderName);
	}
	if (IMG_FolderTitleIcon)
	{
		if (FolderIcon)
		{
			IMG_FolderTitleIcon->SetBrushFromTexture(FolderIcon, true);
		}
		else
		{
			IMG_FolderTitleIcon->SetBrushFromTexture(nullptr);
		}
	}
}

void UBalhwajeomTabletPersonFolderWidget::ClearFolderSections()
{
	if (SB_EvidencePhotos)
	{
		SB_EvidencePhotos->ClearChildren();
	}
}

void UBalhwajeomTabletPersonFolderWidget::AddFolderSection(UWidget* Section)
{
	if (SB_EvidencePhotos && Section)
	{
		SB_EvidencePhotos->AddChild(Section);
	}
}

void UBalhwajeomTabletPersonFolderWidget::HandleCloseClicked()
{
	OnBackRequested.Broadcast();
}

#if WITH_EDITOR
void UBalhwajeomTabletWidget::InitializeForAutomatedTest()
{
	NativeOnInitialized();
	if (WBP_Messenger)
	{
		WBP_Messenger->InitializeForAutomatedTest();
	}
	if (WBP_Internet)
	{
		WBP_Internet->InitializeForAutomatedTest();
	}
}
#endif

bool UBalhwajeomTabletWidget::PlayTabletOpenAnimation()
{
	bWaitingForCloseAnimation = false;
	if (!TabletUpAnim)
	{
		return false;
	}

	PlayAnimationForward(TabletUpAnim, 1.0f, false);
	return true;
}

bool UBalhwajeomTabletWidget::PlayTabletCloseAnimation()
{
	if (!TabletUpAnim)
	{
		return false;
	}

	bWaitingForCloseAnimation = true;
	PlayAnimationReverse(TabletUpAnim, 1.0f, false);
	return true;
}

void UBalhwajeomTabletWidget::CancelTabletCloseAnimation()
{
	if (!bWaitingForCloseAnimation)
	{
		return;
	}

	bWaitingForCloseAnimation = false;
	if (TabletUpAnim)
	{
		PlayAnimationForward(TabletUpAnim, 1.0f, false);
	}
}

void UBalhwajeomTabletWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);
	if (Animation == TabletUpAnim && bWaitingForCloseAnimation)
	{
		bWaitingForCloseAnimation = false;
		OnTabletCloseAnimationFinished.Broadcast();
	}
}

void UBalhwajeomTabletWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Messenger)
	{
		BTN_Messenger->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMessengerClicked);
	}
	if (BTN_Internet)
	{
		BTN_Internet->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleInternetClicked);
	}
	if (BTN_Memo)
	{
		BTN_Memo->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMemoClicked);
	}
	if (BTN_FolderClose)
	{
		BTN_FolderClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
	if (WBP_PersonFolder)
	{
		WBP_PersonFolder->OnBackRequested.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
		WBP_PersonFolder->SetFolderHeader(
			NSLOCTEXT("Tablet", "DefaultFolderTitle", "여동생"),
			DefaultFolderIcon);
	}
	if (IMG_FolderTitleIcon && DefaultFolderIcon)
	{
		IMG_FolderTitleIcon->SetBrushFromTexture(DefaultFolderIcon, true);
	}
	if (BTN_InternetBack)
	{
		BTN_InternetBack->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
	if (BTN_MemoBack)
	{
		BTN_MemoBack->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
	if (BTN_PhysicalHome)
	{
		BTN_PhysicalHome->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePhysicalHomeClicked);
	}
	if (BTN_PopupClose)
	{
		BTN_PopupClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePopupCloseClicked);
	}
	if (BTN_PlayStoryVoice)
	{
		BTN_PlayStoryVoice->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePlayStoryVoiceClicked);
	}
	if (BTN_StatementSubmit) BTN_StatementSubmit->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStatementSubmitClicked);
	if (WBP_Messenger)
	{
		WBP_Messenger->OnBackRequested.AddUniqueDynamic(
			this,
			&ThisClass::HandleMessengerBackRequested);
		WBP_Messenger->OnTotalUnreadChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleMessengerUnreadChanged);
		WBP_Messenger->InitializeMessenger();
		SetUnreadMessageCount(WBP_Messenger->GetTotalUnreadCount());
	}
	if (WBP_Internet)
	{
		WBP_Internet->OnCloseRequested.AddUniqueDynamic(
			this,
			&ThisClass::HandleInternetCloseRequested);
		WBP_Internet->InitializeInternet();
	}

	CurrentPage = ETabletPage::Home;
	PageHistory.Reset();
	if (WidgetSwitcher_TabletPage)
	{
		WidgetSwitcher_TabletPage->SetActiveWidgetIndex(ToPageIndex(CurrentPage));
	}
	HidePopup();
	UpdateUnreadBadge();
	RefreshHomeFolders();
}

void UBalhwajeomTabletWidget::ResetToDesktop()
{
	PageHistory.Reset();
	SetTabletPage(ETabletPage::Home, false);
}

void UBalhwajeomTabletWidget::SetUnreadMessageCount(const int32 NewCount)
{
	UnreadMessageCount = FMath::Max(0, NewCount);
	UpdateUnreadBadge();
}

void UBalhwajeomTabletWidget::SetTabletPage(const ETabletPage NewPage, const bool bAddToHistory)
{
	if (NewPage == CurrentPage)
	{
		HidePopup();
		return;
	}

	if (bAddToHistory)
	{
		PageHistory.Add(CurrentPage);
	}

	CurrentPage = NewPage;
	HidePopup();
	if (WidgetSwitcher_TabletPage)
	{
		WidgetSwitcher_TabletPage->SetActiveWidgetIndex(ToPageIndex(CurrentPage));
	}
}

void UBalhwajeomTabletWidget::NavigateBack()
{
	HidePopup();
	if (PageHistory.IsEmpty())
	{
		SetTabletPage(ETabletPage::Home, false);
		return;
	}

	const ETabletPage PreviousPage = PageHistory.Pop();
	SetTabletPage(PreviousPage, false);
}

void UBalhwajeomTabletWidget::RefreshHomeFolders()
{
	if (!WB_PersonFolders || !WidgetTree)
	{
		return;
	}

	WB_PersonFolders->ClearChildren();

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation)
	{
		return;
	}

	TArray<FCharacterDefinition> Characters;
	Investigation->GetAllCharacterDefinitions(Characters);
	for (const FCharacterDefinition& Character : Characters)
	{
		USizeBox* EntrySize = WidgetTree->ConstructWidget<USizeBox>();
		EntrySize->SetWidthOverride(109.0f);
		EntrySize->SetHeightOverride(100.0f);
		EntrySize->SetClipping(EWidgetClipping::ClipToBounds);
		UBalhwajeomTabletFolderButton* Entry =
			WidgetTree->ConstructWidget<UBalhwajeomTabletFolderButton>();
		Entry->Configure(Character.CharacterID, Character.FolderName, DefaultFolderIcon, FolderLabelFont);
		Entry->OnFolderSelected.AddUniqueDynamic(this, &ThisClass::HandleHomeFolderSelected);
		EntrySize->AddChild(Entry);
		WB_PersonFolders->AddChild(EntrySize);
	}
}

void UBalhwajeomTabletWidget::ShowFolder(const FName CharacterID)
{
	ActiveCharacterID = CharacterID;
	SetTabletPage(ETabletPage::PersonFolder);
	RefreshFolderContents();
}

UBalhwajeomInvestigationSubsystem* UBalhwajeomTabletWidget::GetInvestigationSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UBalhwajeomInvestigationSubsystem>() : nullptr;
}

namespace
{
	/** Wraps a runtime-created file tile at the shared ~170x170 size, clipped so a long label's
	 * ellipsis-truncated text can never visually spill into the neighboring tile. */
	USizeBox* MakeFolderTileSlot(UWidgetTree& WidgetTree, UWidget* Content)
	{
		USizeBox* EntrySize = WidgetTree.ConstructWidget<USizeBox>();
		EntrySize->SetWidthOverride(109.0f);
		EntrySize->SetHeightOverride(100.0f);
		EntrySize->SetClipping(EWidgetClipping::ClipToBounds);
		EntrySize->AddChild(Content);
		return EntrySize;
	}
}

void UBalhwajeomTabletWidget::RefreshFolderContents()
{
	VisiblePhotoIDs.Reset();
	VisibleStatementIDs.Reset();
	FText FolderName;
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();

	TArray<FPhotoDefinition> Photos;
	if (Investigation)
	{
		Investigation->GetPhotosForCharacter(GetActiveCharacterID(), Photos);
		for (const FPhotoDefinition& Photo : Photos)
		{
			if (Investigation->HasCapturedPhoto(Photo.PhotoID))
			{
				VisiblePhotoIDs.Add(Photo.PhotoID);
			}
		}

		TArray<FSentenceDefinition> Statements;
		Investigation->GetStatementSentencesForCharacter(GetActiveCharacterID(), Statements);
		for (const FSentenceDefinition& Statement : Statements)
		{
			VisibleStatementIDs.Add(Statement.SentenceID);
		}

		FCharacterDefinition Character;
		if (Investigation->GetCharacterDefinition(GetActiveCharacterID(), Character) &&
			!Character.FolderName.IsEmpty())
		{
			FolderName = Character.FolderName;
		}
	}

	if (WBP_PersonFolder)
	{
		WBP_PersonFolder->SetFolderHeader(FolderName, DefaultFolderIcon);
		WBP_PersonFolder->ClearFolderSections();
	}
	else
	{
		if (TXT_FolderTitle)
		{
			TXT_FolderTitle->SetText(FolderName);
		}
		if (IMG_FolderTitleIcon && DefaultFolderIcon)
		{
			IMG_FolderTitleIcon->SetBrushFromTexture(DefaultFolderIcon, true);
		}
		if (SB_EvidencePhotos)
		{
			SB_EvidencePhotos->ClearChildren();
		}
	}

	if ((!WBP_PersonFolder && !SB_EvidencePhotos) || !WidgetTree)
	{
		return;
	}

	// Three buckets, in the order the player should see them: the one thing to read (진술서), the
	// puzzles still needing keywords (분석 문장), then everything already wrapped up (완성 문장).
	UBalhwajeomTabletFolderSection* StatementSection = Cast<UBalhwajeomTabletFolderSection>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletFolderSection::StaticClass(), NAME_None));
	UBalhwajeomTabletFolderSection* NeedsAnalysisSection = Cast<UBalhwajeomTabletFolderSection>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletFolderSection::StaticClass(), NAME_None));
	UBalhwajeomTabletFolderSection* CompletedSection = Cast<UBalhwajeomTabletFolderSection>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletFolderSection::StaticClass(), NAME_None));
	if (!StatementSection || !NeedsAnalysisSection || !CompletedSection)
	{
		return;
	}
	StatementSection->Configure(NSLOCTEXT("Tablet", "FolderSectionStatement", "진술서"));
	NeedsAnalysisSection->Configure(NSLOCTEXT("Tablet", "FolderSectionNeedsAnalysis", "분석 문장"));
	CompletedSection->Configure(NSLOCTEXT("Tablet", "FolderSectionCompleted", "완성 문장"));

	if (Investigation && VisibleStatementIDs.IsValidIndex(0))
	{
		const FName StatementID = VisibleStatementIDs[0];
		const FText Label = FText::Format(
			NSLOCTEXT("Tablet", "DynamicStatementFileLabel", "{0} 진술서"), FolderName);

		UBalhwajeomTabletPhotoButton* Entry =
			WidgetTree->ConstructWidget<UBalhwajeomTabletPhotoButton>();
		Entry->Configure(StatementID, Label, StatementFileIcon);
		Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleStatementTileSelected);
		StatementSection->AddTile(MakeFolderTileSlot(*WidgetTree, Entry));
	}

	for (const FName PhotoID : VisiblePhotoIDs)
	{
		FPhotoDefinition Photo;
		if (!Investigation || !Investigation->GetPhotoDefinition(PhotoID, Photo))
		{
			continue;
		}

		UBalhwajeomTabletPhotoButton* Entry =
			WidgetTree->ConstructWidget<UBalhwajeomTabletPhotoButton>();
		Entry->Configure(PhotoID, Photo.PhotoName, GetOrLoadCapturedPhotoTexture(PhotoID));
		Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleFolderPhotoSelected);
		USizeBox* Tile = MakeFolderTileSlot(*WidgetTree, Entry);

		// Completed = no analysis puzzle to begin with, or its puzzle is already solved.
		// Needs analysis = there's an unsolved keyword puzzle still waiting on this photo.
		const bool bNeedsAnalysis = !Photo.PhotoSentenceID.IsNone()
			&& !Investigation->IsSentenceSolved(Photo.PhotoSentenceID);
		(bNeedsAnalysis ? NeedsAnalysisSection : CompletedSection)->AddTile(Tile);
	}

	for (UBalhwajeomTabletFolderSection* Section : {StatementSection, NeedsAnalysisSection, CompletedSection})
	{
		if (!Section->IsEmpty())
		{
			if (WBP_PersonFolder)
			{
				WBP_PersonFolder->AddFolderSection(Section);
			}
			else
			{
				SB_EvidencePhotos->AddChild(Section);
			}
		}
	}
}

void UBalhwajeomTabletWidget::OpenPhoto(const FName PhotoID)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || PhotoID.IsNone())
	{
		return;
	}
	FPhotoDefinition Photo;
	if (!Investigation->GetPhotoDefinition(PhotoID, Photo))
	{
		return;
	}

	FText Body = Photo.CustomDescription;
	if (!Photo.PhotoSentenceID.IsNone())
	{
		FSentenceDefinition Analysis;
		if (Investigation->GetSentenceDefinition(Photo.PhotoSentenceID, Analysis))
		{
			Body = Investigation->IsSentenceSolved(Photo.PhotoSentenceID)
				? Analysis.ResultText : Analysis.SentenceTemplate;
		}
	}
	// WorldStoryCues/WorldStoryLines are the timed captions shown during the in-world capture
	// presentation only (see APhotoWorldStoryActor); the tablet never repeats that text.
	ShowPopup(Photo.PhotoName, Body, GetOrLoadCapturedPhotoTexture(PhotoID));
	ActivePhotoID = PhotoID;
	if (BTN_PlayStoryVoice)
	{
		BTN_PlayStoryVoice->SetVisibility(
			Photo.StoryVoice.IsNull() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (!Photo.PhotoSentenceID.IsNone() && !Investigation->IsSentenceSolved(Photo.PhotoSentenceID))
	{
		PreparePuzzle(Photo.PhotoSentenceID);
	}
}

void UBalhwajeomTabletWidget::PreparePuzzle(FName SentenceID)
{
	ActiveSentenceID = SentenceID;
	ActiveSubmission = FSentenceSubmission{};
	RefreshPuzzleControls();
}

void UBalhwajeomTabletWidget::HidePuzzleControls()
{
	if (WB_PuzzleWords)
	{
		WB_PuzzleWords->ClearChildren();
		WB_PuzzleWords->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WB_SentenceBuilder)
	{
		WB_SentenceBuilder->ClearChildren();
		WB_SentenceBuilder->SetVisibility(ESlateVisibility::Collapsed);
	}
	ActiveBlanksBySlot.Reset();
	if (TXT_PuzzlePhotoLabel)
	{
		TXT_PuzzlePhotoLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WB_PuzzlePhotos)
	{
		WB_PuzzlePhotos->ClearChildren();
		WB_PuzzlePhotos->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WB_PhotoSlots)
	{
		WB_PhotoSlots->ClearChildren();
		WB_PhotoSlots->SetVisibility(ESlateVisibility::Collapsed);
	}
	ActivePhotoSlotsBySlot.Reset();
	if (TXT_PuzzleFeedback)
	{
		TXT_PuzzleFeedback->SetText(FText::GetEmpty());
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TXT_PopupBody)
	{
		// Restored here; RefreshPuzzleControls/BuildSentenceBuilder hides it again if there's
		// an active unsolved puzzle to show the interactive sentence builder instead.
		TXT_PopupBody->SetVisibility(ESlateVisibility::Visible);
	}
	if (BTN_StatementSubmit)
	{
		BTN_StatementSubmit->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::RefreshPuzzleControls()
{
	HidePuzzleControls();
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence))
	{
		return;
	}

	AvailablePuzzleWordIDs.Reset();
	TArray<FAcquiredWordRecord> AcquiredWords;
	if (Sentence.SentenceType == ESentenceType::Statement)
	{
		Investigation->GetAcquiredWordsForCharacter(Sentence.CharacterID, AcquiredWords);
	}
	else
	{
		Investigation->GetAcquiredWords(AcquiredWords);
	}
	for (const FAcquiredWordRecord& Word : AcquiredWords) AvailablePuzzleWordIDs.Add(Word.WordID);

	if (WB_PuzzleWords && WidgetTree)
	{
		WB_PuzzleWords->SetVisibility(
			AvailablePuzzleWordIDs.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		for (const FName WordID : AvailablePuzzleWordIDs)
		{
			FWordDefinition Word;
			if (!Investigation->GetWordDefinition(WordID, Word))
			{
				continue;
			}
			UBalhwajeomTabletWordChip* Chip = Cast<UBalhwajeomTabletWordChip>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletWordChip::StaticClass(), NAME_None));
			if (!Chip)
			{
				continue;
			}
			Chip->Configure(WordID, Word.DisplayWord);
			WB_PuzzleWords->AddChild(Chip);
		}
	}

	// Interactive sentence (draggable blanks) replaces the raw "[]" template text; see BuildSentenceBuilder.
	BuildSentenceBuilder(Sentence);

	// Photo-evidence drop slots only exist when this sentence requires photo evidence (e.g. a
	// Statement disproving a confession). Candidates are captured photos whose own analysis
	// sentence is already solved -- a photo that's merely captured isn't valid evidence yet.
	if (WB_PuzzlePhotos && WidgetTree && !Sentence.PhotoSlots.IsEmpty())
	{
		if (TXT_PuzzlePhotoLabel)
		{
			TXT_PuzzlePhotoLabel->SetVisibility(ESlateVisibility::Visible);
		}
		TArray<FCapturedPhotoRecord> CapturedPhotoRecords;
		Investigation->GetCapturedPhotos(CapturedPhotoRecords);
		bool bAnyEligiblePhoto = false;
		for (const FCapturedPhotoRecord& Record : CapturedPhotoRecords)
		{
			FPhotoDefinition PhotoDef;
			if (!Investigation->GetPhotoDefinition(Record.PhotoID, PhotoDef) ||
				PhotoDef.PhotoSentenceID.IsNone() ||
				!Investigation->IsSentenceSolved(PhotoDef.PhotoSentenceID))
			{
				continue;
			}
			UBalhwajeomTabletPhotoChip* Chip = Cast<UBalhwajeomTabletPhotoChip>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletPhotoChip::StaticClass(), NAME_None));
			if (!Chip)
			{
				continue;
			}
			Chip->Configure(PhotoDef.PhotoID, PhotoDef.PhotoName);
			WB_PuzzlePhotos->AddChild(Chip);
			bAnyEligiblePhoto = true;
		}
		WB_PuzzlePhotos->SetVisibility(bAnyEligiblePhoto ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	BuildPhotoSlots(Sentence);

	if (BTN_StatementSubmit && Sentence.SentenceType == ESentenceType::Statement)
	{
		BTN_StatementSubmit->SetVisibility(ESlateVisibility::Visible);
	}
}

void UBalhwajeomTabletWidget::BuildSentenceBuilder(const FSentenceDefinition& Sentence)
{
	if (!WB_SentenceBuilder || !WidgetTree)
	{
		return;
	}
	WB_SentenceBuilder->ClearChildren();
	ActiveBlanksBySlot.Reset();

	TArray<FString> Segments;
	Sentence.SentenceTemplate.ToString().ParseIntoArray(Segments, TEXT("[]"), false);
	const int32 SlotCount = Sentence.WordSlots.Num();

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		if (!Segments[SegmentIndex].IsEmpty())
		{
			UTextBlock* SegmentText = WidgetTree->ConstructWidget<UTextBlock>();
			SegmentText->SetText(FText::FromString(Segments[SegmentIndex]));
			FSlateFontInfo Font = SegmentText->GetFont();
			Font.Size = 22;
			SegmentText->SetFont(Font);
			SegmentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.86f, 0.78f, 1.0f)));
			if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(WB_SentenceBuilder->AddChild(SegmentText)))
			{
				WrapSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (SegmentIndex < SlotCount)
		{
			UBalhwajeomTabletSentenceBlank* Blank = Cast<UBalhwajeomTabletSentenceBlank>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletSentenceBlank::StaticClass(), NAME_None));
			if (!Blank)
			{
				continue;
			}
			Blank->Configure(SegmentIndex);
			Blank->OnBlankDropped.AddUniqueDynamic(this, &ThisClass::HandleSentenceBlankDropped);
			ActiveBlanksBySlot.Add(SegmentIndex, Blank);
			if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(WB_SentenceBuilder->AddChild(Blank)))
			{
				WrapSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	const bool bHasBlanks = !ActiveBlanksBySlot.IsEmpty();
	WB_SentenceBuilder->SetVisibility(bHasBlanks ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (TXT_PopupBody && bHasBlanks)
	{
		TXT_PopupBody->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::BuildPhotoSlots(const FSentenceDefinition& Sentence)
{
	if (!WB_PhotoSlots || !WidgetTree)
	{
		return;
	}
	WB_PhotoSlots->ClearChildren();
	ActivePhotoSlotsBySlot.Reset();

	TArray<FSentencePhotoSlot> SortedPhotoSlots = Sentence.PhotoSlots;
	SortedPhotoSlots.Sort(
		[](const FSentencePhotoSlot& A, const FSentencePhotoSlot& B) { return A.SlotIndex < B.SlotIndex; });

	for (const FSentencePhotoSlot& PhotoSlotDefinition : SortedPhotoSlots)
	{
		UBalhwajeomTabletPhotoSlot* PhotoSlotWidget = Cast<UBalhwajeomTabletPhotoSlot>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletPhotoSlot::StaticClass(), NAME_None));
		if (!PhotoSlotWidget)
		{
			continue;
		}
		PhotoSlotWidget->Configure(PhotoSlotDefinition.SlotIndex);
		PhotoSlotWidget->OnPhotoSlotDropped.AddUniqueDynamic(this, &ThisClass::HandlePhotoSlotDropped);
		ActivePhotoSlotsBySlot.Add(PhotoSlotDefinition.SlotIndex, PhotoSlotWidget);
		if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(WB_PhotoSlots->AddChild(PhotoSlotWidget)))
		{
			WrapSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	WB_PhotoSlots->SetVisibility(
		ActivePhotoSlotsBySlot.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UBalhwajeomTabletWidget::HandleSentenceBlankDropped(
	const int32 SlotIndex, const FName WordID, const int32 OriginSlotIndex)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) ||
		!Investigation->HasAcquiredWord(WordID))
	{
		return;
	}

	const bool bValidSlot = Sentence.WordSlots.ContainsByPredicate(
		[SlotIndex](const FSentenceWordSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	if (!bValidSlot || SlotIndex == OriginSlotIndex)
	{
		// Dropped back onto the same blank it came from -- nothing to do.
		return;
	}

	// Whatever word already occupied the destination slot gets displaced by this drop.
	FName DisplacedWordID = NAME_None;
	if (const FSubmittedWordSlot* Existing = ActiveSubmission.SubmittedWords.FindByPredicate(
		[SlotIndex](const FSubmittedWordSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; }))
	{
		DisplacedWordID = Existing->WordID;
	}

	// The drop just fills the blank; correctness (including order-flexible groups) is judged by
	// ValidateSentence once every blank/photo slot has something in it -- see EvaluatePuzzleIfComplete.
	ActiveSubmission.SubmittedWords.RemoveAll(
		[SlotIndex](const FSubmittedWordSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	ActiveSubmission.SubmittedWords.Add({SlotIndex, WordID});

	if (UBalhwajeomTabletSentenceBlank* Blank = ActiveBlanksBySlot.FindRef(SlotIndex))
	{
		FWordDefinition WordDef;
		if (Investigation->GetWordDefinition(WordID, WordDef))
		{
			Blank->SetFilled(WordID, WordDef.DisplayWord);
		}
	}

	// The dragged word came from another blank (not the acquired-keyword list): send the displaced
	// word there instead of just discarding it (a swap), or empty that blank if there was nothing to
	// displace (a plain move).
	if (OriginSlotIndex != INDEX_NONE)
	{
		ActiveSubmission.SubmittedWords.RemoveAll(
			[OriginSlotIndex](const FSubmittedWordSlot& Candidate) { return Candidate.SlotIndex == OriginSlotIndex; });
		UBalhwajeomTabletSentenceBlank* OriginBlank = ActiveBlanksBySlot.FindRef(OriginSlotIndex);
		if (!DisplacedWordID.IsNone())
		{
			ActiveSubmission.SubmittedWords.Add({OriginSlotIndex, DisplacedWordID});
			FWordDefinition DisplacedWordDef;
			if (OriginBlank && Investigation->GetWordDefinition(DisplacedWordID, DisplacedWordDef))
			{
				OriginBlank->SetFilled(DisplacedWordID, DisplacedWordDef.DisplayWord);
			}
		}
		else if (OriginBlank)
		{
			OriginBlank->SetEmpty();
		}
	}

	if (TXT_PuzzleFeedback)
	{
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Collapsed);
	}

	EvaluatePuzzleIfComplete();
}

void UBalhwajeomTabletWidget::HandlePhotoSlotDropped(const int32 SlotIndex, const FName PhotoID)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) ||
		!Investigation->HasCapturedPhoto(PhotoID))
	{
		return;
	}

	const bool bValidSlot = Sentence.PhotoSlots.ContainsByPredicate(
		[SlotIndex](const FSentencePhotoSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	if (!bValidSlot)
	{
		return;
	}

	ActiveSubmission.SubmittedPhotos.RemoveAll(
		[SlotIndex](const FSubmittedPhotoSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	ActiveSubmission.SubmittedPhotos.Add({SlotIndex, PhotoID});

	if (UBalhwajeomTabletPhotoSlot* PhotoSlotWidget = ActivePhotoSlotsBySlot.FindRef(SlotIndex))
	{
		FPhotoDefinition PhotoDef;
		if (Investigation->GetPhotoDefinition(PhotoID, PhotoDef))
		{
			PhotoSlotWidget->SetFilled(PhotoDef.PhotoName);
		}
	}

	if (TXT_PuzzleFeedback)
	{
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Collapsed);
	}

	EvaluatePuzzleIfComplete();
}

void UBalhwajeomTabletWidget::EvaluatePuzzleIfComplete()
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence))
	{
		return;
	}

	// Wait until every blank and every photo slot has something in it before judging correctness,
	// so the "잘못된 증거인 것 같다" feedback doesn't fire after each individual drop.
	if (ActiveSubmission.SubmittedWords.Num() < Sentence.WordSlots.Num() ||
		ActiveSubmission.SubmittedPhotos.Num() < Sentence.PhotoSlots.Num())
	{
		return;
	}

	ValidateActivePuzzle(false);
}

void UBalhwajeomTabletWidget::ValidateActivePuzzle(const bool bExplicitStatementSubmit)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) ||
		(Sentence.SentenceType == ESentenceType::Statement && !bExplicitStatementSubmit)) return;
	FText Result;
	if (Investigation->ValidateSentence(ActiveSentenceID, ActiveSubmission, Result))
	{
		if (TXT_PopupBody) TXT_PopupBody->SetText(Result);
		HidePuzzleControls();
		RefreshAcquiredWordsDisplay();
		RefreshFolderContents();
	}
	else if (TXT_PuzzleFeedback)
	{
		TXT_PuzzleFeedback->SetText(NSLOCTEXT("Tablet", "WrongEvidence", "잘못된 증거인 것 같다."));
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Visible);
	}
}

void UBalhwajeomTabletWidget::ShowPopup(const FText& Title, const FText& Body, UTexture2D* PhotoTexture)
{
	ActiveSentenceID = NAME_None;
	// Reset here; OpenPhoto re-populates this (and BTN_PlayStoryVoice's visibility) right after this
	// call for a photo popup. Callers that open a non-photo popup (e.g. the statement) leave both unset.
	ActivePhotoID = NAME_None;
	if (BTN_PlayStoryVoice)
	{
		BTN_PlayStoryVoice->SetVisibility(ESlateVisibility::Collapsed);
	}
	HidePuzzleControls();
	if (TXT_PopupTitle)
	{
		TXT_PopupTitle->SetText(Title);
	}
	if (TXT_PopupBody)
	{
		TXT_PopupBody->SetText(Body);
	}
	if (IMG_PopupPhoto)
	{
		if (PhotoTexture)
		{
			IMG_PopupPhoto->SetBrushFromTexture(PhotoTexture, true);
			IMG_PopupPhoto->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IMG_PopupPhoto->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	RefreshAcquiredWordsDisplay();
	if (PopupLayer)
	{
		PopupLayer->SetVisibility(ESlateVisibility::Visible);
	}
}

void UBalhwajeomTabletWidget::RefreshAcquiredWordsDisplay()
{
	if (!WB_PuzzleWords || !WidgetTree)
	{
		return;
	}

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || ActiveCharacterID.IsNone())
	{
		return;
	}

	TArray<FAcquiredWordRecord> FolderWords;
	Investigation->GetAcquiredWordsForCharacter(ActiveCharacterID, FolderWords);

	WB_PuzzleWords->ClearChildren();
	for (const FAcquiredWordRecord& Record : FolderWords)
	{
		FWordDefinition Word;
		if (!Investigation->GetWordDefinition(Record.WordID, Word))
		{
			continue;
		}
		UBalhwajeomTabletWordChip* Chip = Cast<UBalhwajeomTabletWordChip>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletWordChip::StaticClass(), NAME_None));
		if (!Chip)
		{
			continue;
		}
		Chip->Configure(Record.WordID, Word.DisplayWord);
		WB_PuzzleWords->AddChild(Chip);
	}
	WB_PuzzleWords->SetVisibility(
		FolderWords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

UTexture2D* UBalhwajeomTabletWidget::GetOrLoadCapturedPhotoTexture(const FName PhotoID)
{
	if (PhotoID.IsNone())
	{
		return nullptr;
	}
	if (const TObjectPtr<UTexture2D>* Cached = CapturedPhotoTextureCache.Find(PhotoID))
	{
		return *Cached;
	}

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FCapturedPhotoRecord Record;
	if (!Investigation || !Investigation->GetCapturedPhoto(PhotoID, Record))
	{
		return nullptr;
	}

	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectSavedDir(), Record.ImageRelativePath));
	UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(AbsolutePath);
	CapturedPhotoTextureCache.Add(PhotoID, Texture);
	return Texture;
}

void UBalhwajeomTabletWidget::HidePopup()
{
	if (PopupLayer)
	{
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::UpdateUnreadBadge()
{
	if (BRD_MessengerBadge)
	{
		BRD_MessengerBadge->SetVisibility(
			UnreadMessageCount > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TXT_UnreadMessageCount)
	{
		TXT_UnreadMessageCount->SetText(FText::AsNumber(UnreadMessageCount));
	}
}

void UBalhwajeomTabletWidget::HandleHomeFolderSelected(const FName CharacterID)
{
	ShowFolder(CharacterID);
}

void UBalhwajeomTabletWidget::HandleMessengerClicked()
{
	if (WBP_Messenger)
	{
		WBP_Messenger->InitializeMessenger();
	}
	SetTabletPage(ETabletPage::Messenger);
}

void UBalhwajeomTabletWidget::HandleMessengerBackRequested()
{
	NavigateBack();
}

void UBalhwajeomTabletWidget::HandleMessengerUnreadChanged(const int32 TotalUnreadCount)
{
	SetUnreadMessageCount(TotalUnreadCount);
}

void UBalhwajeomTabletWidget::HandleInternetClicked()
{
	if (WBP_Internet)
	{
		WBP_Internet->PrepareForDesktopOpen();
	}
	SetTabletPage(ETabletPage::Internet);
}

void UBalhwajeomTabletWidget::HandleInternetCloseRequested()
{
	ResetToDesktop();
}

void UBalhwajeomTabletWidget::HandleMemoClicked()
{
	SetTabletPage(ETabletPage::Memo);
}

void UBalhwajeomTabletWidget::HandleBackClicked()
{
	NavigateBack();
}

void UBalhwajeomTabletWidget::HandlePhysicalHomeClicked()
{
	ResetToDesktop();
}

void UBalhwajeomTabletPhotoButton::Configure(
	const FName InPhotoID,
	const FText& InLabel,
	UTexture2D* Thumbnail)
{
	PhotoID = InPhotoID;
	OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	MakeButtonTransparent(this);

	UVerticalBox* Layout = NewObject<UVerticalBox>(this);

	if (Thumbnail)
	{
		// Sized to fit inside the ~170-wide folder tile (see RefreshFolderContents).
		USizeBox* ThumbnailBox = NewObject<USizeBox>(this);
		ThumbnailBox->SetWidthOverride(150.0f);
		ThumbnailBox->SetHeightOverride(95.0f);

		UImage* Image = NewObject<UImage>(this);
		Image->SetBrushFromTexture(Thumbnail, true);
		ThumbnailBox->AddChild(Image);

		UVerticalBoxSlot* ThumbnailSlot = Layout->AddChildToVerticalBox(ThumbnailBox);
		ThumbnailSlot->SetHorizontalAlignment(HAlign_Center);
		ThumbnailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(InLabel);
	Label->SetJustification(ETextJustify::Center);
	// Single line, truncated with "..." like a real folder's filename label, instead of
	// wrapping and getting clipped by the tile's fixed height once a thumbnail is present.
	Label->SetAutoWrapText(false);
	Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 20;
	Label->SetFont(Font);
	UVerticalBoxSlot* LabelSlot = Layout->AddChildToVerticalBox(Label);
	LabelSlot->SetHorizontalAlignment(HAlign_Fill);

	SetContent(Layout);
}

void UBalhwajeomTabletPhotoButton::HandleClicked()
{
	if (!PhotoID.IsNone())
	{
		OnPhotoSelected.Broadcast(PhotoID);
	}
}

void UBalhwajeomTabletFolderButton::Configure(
	const FName InCharacterID,
	const FText& InLabel,
	UTexture2D* IconTexture,
	const FSlateFontInfo& InLabelFont)
{
	CharacterID = InCharacterID;
	OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	MakeButtonTransparent(this);

	UVerticalBox* Layout = NewObject<UVerticalBox>(this);

	if (IconTexture)
	{
		USizeBox* IconBox = NewObject<USizeBox>(this);
		IconBox->SetWidthOverride(64.0f);
		IconBox->SetHeightOverride(60.0f);

		UImage* Icon = NewObject<UImage>(this);
		Icon->SetBrushFromTexture(IconTexture, true);
		IconBox->AddChild(Icon);

		UVerticalBoxSlot* IconSlot = Layout->AddChildToVerticalBox(IconBox);
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(InLabel);
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(false);
	Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
	FSlateFontInfo Font = InLabelFont;
	if (!Font.FontObject)
	{
		Font = Label->GetFont();
	}
	if (Font.Size <= 0)
	{
		Font.Size = 18;
	}
	Label->SetFont(Font);
	UVerticalBoxSlot* LabelSlot = Layout->AddChildToVerticalBox(Label);
	LabelSlot->SetHorizontalAlignment(HAlign_Fill);

	SetContent(Layout);
}

void UBalhwajeomTabletFolderButton::HandleClicked()
{
	if (!CharacterID.IsNone())
	{
		OnFolderSelected.Broadcast(CharacterID);
	}
}

void UBalhwajeomTabletFolderSection::Configure(const FText& InTitle, const bool bStartExpanded)
{
	Title = InTitle;
	bExpanded = bStartExpanded;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();

	HeaderButton = WidgetTree->ConstructWidget<UButton>();
	MakeButtonTransparent(HeaderButton);
	HeaderButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHeaderClicked);

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();

	ArrowText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo ArrowFont = ArrowText->GetFont();
	ArrowFont.Size = 20;
	ArrowText->SetFont(ArrowFont);
	ArrowText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.86f, 0.78f, 1.0f)));
	UHorizontalBoxSlot* ArrowSlot = HeaderRow->AddChildToHorizontalBox(ArrowText);
	ArrowSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	ArrowSlot->SetVerticalAlignment(VAlign_Center);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 22;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.86f, 0.78f, 1.0f)));
	UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleText);
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	// UButton's content slot defaults to HAlign_Center; without this override the arrow+title
	// row sits centered across the button's full (section-wide) width instead of hugging the left edge.
	if (UButtonSlot* HeaderRowSlot = Cast<UButtonSlot>(HeaderButton->SetContent(HeaderRow)))
	{
		HeaderRowSlot->SetHorizontalAlignment(HAlign_Left);
	}
	UVerticalBoxSlot* HeaderSlot = Root->AddChildToVerticalBox(HeaderButton);
	HeaderSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 10.0f));

	ContentWrapBox = WidgetTree->ConstructWidget<UWrapBox>();
	ContentWrapBox->SetInnerSlotPadding(FVector2D(12.0f, 12.0f));
	UVerticalBoxSlot* ContentSlot = Root->AddChildToVerticalBox(ContentWrapBox);
	ContentSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	WidgetTree->RootWidget = Root;

	ContentWrapBox->SetVisibility(bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	RefreshHeaderText();
}

void UBalhwajeomTabletFolderSection::AddTile(UWidget* Tile)
{
	if (!ContentWrapBox || !Tile)
	{
		return;
	}
	ContentWrapBox->AddChild(Tile);
	++TileCount;
	RefreshHeaderText();
}

void UBalhwajeomTabletFolderSection::ClearTiles()
{
	if (ContentWrapBox)
	{
		ContentWrapBox->ClearChildren();
	}
	TileCount = 0;
	RefreshHeaderText();
}

void UBalhwajeomTabletFolderSection::HandleHeaderClicked()
{
	bExpanded = !bExpanded;
	if (ContentWrapBox)
	{
		ContentWrapBox->SetVisibility(bExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	RefreshHeaderText();
}

void UBalhwajeomTabletFolderSection::RefreshHeaderText()
{
	if (ArrowText)
	{
		ArrowText->SetText(FText::FromString(bExpanded ? TEXT("\x25BC") : TEXT("\x25B6")));
	}
	if (TitleText)
	{
		TitleText->SetText(FText::Format(
			NSLOCTEXT("Tablet", "FolderSectionTitle", "{0} ({1})"), Title, TileCount));
	}
}

void UBalhwajeomTabletWordChip::Configure(const FName InWordID, const FText& InLabel)
{
	WordID = InWordID;
	DisplayLabel = InLabel;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.30f, 0.24f, 0.16f, 1.0f));
	Background->SetPadding(FMargin(10.0f, 6.0f));

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(InLabel);
	Label->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 20;
	Label->SetFont(Font);
	Background->SetContent(Label);

	WidgetTree->RootWidget = Background;
}

FReply UBalhwajeomTabletWordChip::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UBalhwajeomTabletWordChip::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UBalhwajeomWordDragDropOperation* Operation = NewObject<UBalhwajeomWordDragDropOperation>(this);
	Operation->WordID = WordID;
	Operation->Pivot = EDragPivot::MouseDown;

	if (WidgetTree)
	{
		UBorder* DragVisual = WidgetTree->ConstructWidget<UBorder>();
		DragVisual->SetBrushColor(FLinearColor(0.30f, 0.24f, 0.16f, 0.9f));
		DragVisual->SetPadding(FMargin(10.0f, 6.0f));
		UTextBlock* DragLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DragLabel->SetText(DisplayLabel);
		FSlateFontInfo Font = DragLabel->GetFont();
		Font.Size = 20;
		DragLabel->SetFont(Font);
		DragVisual->SetContent(DragLabel);
		Operation->DefaultDragVisual = DragVisual;
	}

	OutOperation = Operation;
}

void UBalhwajeomTabletSentenceBlank::Configure(const int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.20f, 0.16f, 0.10f, 1.0f));
	Background->SetPadding(FMargin(10.0f, 4.0f));

	DisplayText = WidgetTree->ConstructWidget<UTextBlock>();
	DisplayText->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = DisplayText->GetFont();
	Font.Size = 22;
	DisplayText->SetFont(Font);
	Background->SetContent(DisplayText);

	WidgetTree->RootWidget = Background;
	SetEmpty();
}

void UBalhwajeomTabletSentenceBlank::SetEmpty()
{
	FilledWordID = NAME_None;
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(NSLOCTEXT("Tablet", "SentenceBlankPlaceholder", "____"));
	DisplayText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.56f, 0.46f, 1.0f)));
}

void UBalhwajeomTabletSentenceBlank::SetFilled(const FName InWordID, const FText& WordText)
{
	FilledWordID = InWordID;
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(WordText);
	DisplayText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
}

bool UBalhwajeomTabletSentenceBlank::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (const UBalhwajeomWordDragDropOperation* WordOp = Cast<UBalhwajeomWordDragDropOperation>(InOperation))
	{
		OnBlankDropped.Broadcast(SlotIndex, WordOp->WordID, WordOp->OriginSlotIndex);
		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

FReply UBalhwajeomTabletSentenceBlank::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Only a filled blank can be picked back up -- an empty one has nothing to drag.
	if (!FilledWordID.IsNone() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UBalhwajeomTabletSentenceBlank::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (FilledWordID.IsNone())
	{
		return;
	}

	// This blank is left showing its word until the drop actually resolves (see
	// UBalhwajeomTabletWidget::HandleSentenceBlankDropped), so a cancelled drag (dropped nowhere)
	// leaves the puzzle untouched instead of losing the word.
	UBalhwajeomWordDragDropOperation* Operation = NewObject<UBalhwajeomWordDragDropOperation>(this);
	Operation->WordID = FilledWordID;
	Operation->OriginSlotIndex = SlotIndex;
	Operation->Pivot = EDragPivot::MouseDown;

	if (WidgetTree && DisplayText)
	{
		UBorder* DragVisual = WidgetTree->ConstructWidget<UBorder>();
		DragVisual->SetBrushColor(FLinearColor(0.30f, 0.24f, 0.16f, 0.9f));
		DragVisual->SetPadding(FMargin(10.0f, 6.0f));
		UTextBlock* DragLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DragLabel->SetText(DisplayText->GetText());
		FSlateFontInfo Font = DragLabel->GetFont();
		Font.Size = 20;
		DragLabel->SetFont(Font);
		DragVisual->SetContent(DragLabel);
		Operation->DefaultDragVisual = DragVisual;
	}

	OutOperation = Operation;
}

void UBalhwajeomTabletPhotoChip::Configure(const FName InPhotoID, const FText& InLabel)
{
	PhotoID = InPhotoID;
	DisplayLabel = InLabel;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.30f, 0.24f, 0.16f, 1.0f));
	Background->SetPadding(FMargin(10.0f, 6.0f));

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(InLabel);
	Label->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 20;
	Label->SetFont(Font);
	Background->SetContent(Label);

	WidgetTree->RootWidget = Background;
}

FReply UBalhwajeomTabletPhotoChip::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UBalhwajeomTabletPhotoChip::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UBalhwajeomPhotoDragDropOperation* Operation = NewObject<UBalhwajeomPhotoDragDropOperation>(this);
	Operation->PhotoID = PhotoID;
	Operation->Pivot = EDragPivot::MouseDown;

	if (WidgetTree)
	{
		UBorder* DragVisual = WidgetTree->ConstructWidget<UBorder>();
		DragVisual->SetBrushColor(FLinearColor(0.30f, 0.24f, 0.16f, 0.9f));
		DragVisual->SetPadding(FMargin(10.0f, 6.0f));
		UTextBlock* DragLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DragLabel->SetText(DisplayLabel);
		FSlateFontInfo Font = DragLabel->GetFont();
		Font.Size = 20;
		DragLabel->SetFont(Font);
		DragVisual->SetContent(DragLabel);
		Operation->DefaultDragVisual = DragVisual;
	}

	OutOperation = Operation;
}

void UBalhwajeomTabletPhotoSlot::Configure(const int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.20f, 0.16f, 0.10f, 1.0f));
	Background->SetPadding(FMargin(10.0f, 4.0f));

	DisplayText = WidgetTree->ConstructWidget<UTextBlock>();
	DisplayText->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = DisplayText->GetFont();
	Font.Size = 22;
	DisplayText->SetFont(Font);
	Background->SetContent(DisplayText);

	WidgetTree->RootWidget = Background;
	SetEmpty();
}

void UBalhwajeomTabletPhotoSlot::SetEmpty()
{
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(NSLOCTEXT("Tablet", "PhotoSlotPlaceholder", "증거 사진"));
	DisplayText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.56f, 0.46f, 1.0f)));
}

void UBalhwajeomTabletPhotoSlot::SetFilled(const FText& PhotoLabel)
{
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(PhotoLabel);
	DisplayText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
}

bool UBalhwajeomTabletPhotoSlot::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (const UBalhwajeomPhotoDragDropOperation* PhotoOp = Cast<UBalhwajeomPhotoDragDropOperation>(InOperation))
	{
		OnPhotoSlotDropped.Broadcast(SlotIndex, PhotoOp->PhotoID);
		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UBalhwajeomTabletWidget::HandleFolderPhotoSelected(const FName PhotoID)
{
	OpenPhoto(PhotoID);
}

void UBalhwajeomTabletWidget::HandleStatementTileSelected(const FName SentenceID)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || SentenceID.IsNone())
	{
		return;
	}
	FSentenceDefinition Statement;
	if (!Investigation->GetSentenceDefinition(SentenceID, Statement))
	{
		return;
	}
	const FText Answer = Investigation->IsSentenceSolved(Statement.SentenceID)
		? Statement.ResultText : Statement.SentenceTemplate;
	ShowPopup(
		FText::FromString(TEXT("진술서")),
		FText::Format(
			NSLOCTEXT("Tablet", "StatementPopup", "거짓말\n{0}\n\n반증\n{1}"),
			Statement.LieText,
			Answer));
	if (!Investigation->IsSentenceSolved(Statement.SentenceID))
	{
		PreparePuzzle(Statement.SentenceID);
	}
}

void UBalhwajeomTabletWidget::HandlePopupCloseClicked()
{
	HidePopup();
}

void UBalhwajeomTabletWidget::HandleStatementSubmitClicked() { ValidateActivePuzzle(true); }

void UBalhwajeomTabletWidget::HandlePlayStoryVoiceClicked()
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FPhotoDefinition Photo;
	if (!Investigation || ActivePhotoID.IsNone() || !Investigation->GetPhotoDefinition(ActivePhotoID, Photo))
	{
		return;
	}
	// One-shot playback triggered by a click; a synchronous load keeps this simple and is
	// acceptable here since StoryVoice clips are short narration lines, not streamed music.
	USoundBase* Voice = Photo.StoryVoice.LoadSynchronous();
	if (Voice)
	{
		UGameplayStatics::PlaySound2D(this, Voice);
	}
}
