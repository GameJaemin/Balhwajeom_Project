#include "Tablet/BalhwajeomTabletWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Tablet/BalhwajeomMessengerWidget.h"
#include "Tablet/BalhwajeomInternetWidget.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
	int32 ToPageIndex(const ETabletPage Page)
	{
		return static_cast<int32>(Page);
	}
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
		EntrySize->SetWidthOverride(170.0f);
		EntrySize->SetHeightOverride(170.0f);
		UBalhwajeomTabletFolderButton* Entry =
			WidgetTree->ConstructWidget<UBalhwajeomTabletFolderButton>();
		Entry->Configure(Character.CharacterID, Character.FolderName, DefaultFolderIcon);
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

void UBalhwajeomTabletWidget::RefreshFolderContents()
{
	VisiblePhotoIDs.Reset();
	VisibleStatementIDs.Reset();
	FText FolderName;
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (Investigation)
	{
		TArray<FPhotoDefinition> Photos;
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
			if (TXT_FolderTitle)
			{
				TXT_FolderTitle->SetText(FolderName);
			}
		}
	}

	// The statement is its own tile, separate from the scrolling photo grid, pinned at the
	// bottom-center of the folder window (one statement per folder, for now).
	if (SB_StatementTile && WidgetTree)
	{
		if (Investigation && VisibleStatementIDs.IsValidIndex(0))
		{
			const FName StatementID = VisibleStatementIDs[0];
			const bool bComplete = Investigation->IsSentenceSolved(StatementID);
			const FText Label = FText::Format(
				NSLOCTEXT("Tablet", "DynamicStatementFileLabel", "{0}  {1} 진술서"),
				bComplete ? FText::FromString(TEXT("✓")) : FText::FromString(TEXT("?")),
				FolderName);

			UBalhwajeomTabletPhotoButton* Entry =
				WidgetTree->ConstructWidget<UBalhwajeomTabletPhotoButton>();
			Entry->Configure(StatementID, Label);
			Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleStatementTileSelected);
			SB_StatementTile->SetContent(Entry);
			SB_StatementTile->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SB_StatementTile->SetContent(nullptr);
			SB_StatementTile->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (WB_EvidencePhotos && WidgetTree)
	{
		WB_EvidencePhotos->ClearChildren();
		for (const FName PhotoID : VisiblePhotoIDs)
		{
			FPhotoDefinition Photo;
			if (!Investigation || !Investigation->GetPhotoDefinition(PhotoID, Photo))
			{
				continue;
			}
			const bool bComplete = Photo.PhotoSentenceID.IsNone() ||
				Investigation->IsSentenceSolved(Photo.PhotoSentenceID);
			const FText Label = FText::Format(
				NSLOCTEXT("Tablet", "DynamicPhotoFileLabel", "{0}  {1}"),
				bComplete ? FText::FromString(TEXT("✓")) : FText::FromString(TEXT("?")),
				Photo.PhotoName);

			// ~170x170 to roughly match a home-page folder icon's size.
			USizeBox* EntrySize = WidgetTree->ConstructWidget<USizeBox>();
			EntrySize->SetWidthOverride(170.0f);
			EntrySize->SetHeightOverride(170.0f);
			UBalhwajeomTabletPhotoButton* Entry =
				WidgetTree->ConstructWidget<UBalhwajeomTabletPhotoButton>();
			Entry->Configure(PhotoID, Label, GetOrLoadCapturedPhotoTexture(PhotoID));
			Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleFolderPhotoSelected);
			EntrySize->AddChild(Entry);
			WB_EvidencePhotos->AddChild(EntrySize);
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
	else if (Body.IsEmpty())
	{
		Body = FText::Join(FText::FromString(TEXT("\n")), Photo.WorldStoryLines);
	}
	ShowPopup(Photo.PhotoName, Body, GetOrLoadCapturedPhotoTexture(PhotoID));
	if (!Photo.PhotoSentenceID.IsNone() && !Investigation->IsSentenceSolved(Photo.PhotoSentenceID))
	{
		PreparePuzzle(Photo.PhotoSentenceID);
	}
}

void UBalhwajeomTabletWidget::PreparePuzzle(FName SentenceID)
{
	ActiveSentenceID = SentenceID;
	ActiveSubmission = FSentenceSubmission{};
	NextWordSlotCursor = 0;
	NextPhotoSlotCursor = 0;
	RefreshPuzzleControls();
}

void UBalhwajeomTabletWidget::HidePuzzleControls()
{
	if (WB_PuzzleWords)
	{
		WB_PuzzleWords->ClearChildren();
		WB_PuzzleWords->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UButton* Button : {BTN_StatementSubmit.Get()})
	{
		if (Button) Button->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WB_PuzzlePhotos)
	{
		WB_PuzzlePhotos->ClearChildren();
		WB_PuzzlePhotos->SetVisibility(ESlateVisibility::Collapsed);
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

	AvailablePuzzlePhotoIDs.Reset();
	if (Sentence.SentenceType == ESentenceType::Statement)
	{
		TArray<FCapturedPhotoRecord> CapturedPhotos;
		Investigation->GetCapturedPhotos(CapturedPhotos);
		for (const FCapturedPhotoRecord& Captured : CapturedPhotos)
		{
			FPhotoDefinition Photo;
			if (Investigation->GetPhotoDefinition(Captured.PhotoID, Photo) &&
				!Photo.PhotoSentenceID.IsNone() && Investigation->IsSentenceSolved(Photo.PhotoSentenceID))
			{
				AvailablePuzzlePhotoIDs.Add(Photo.PhotoID);
			}
		}
	}

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
			USizeBox* EntrySize = WidgetTree->ConstructWidget<USizeBox>();
			EntrySize->SetWidthOverride(220.0f);
			EntrySize->SetHeightOverride(50.0f);
			UBalhwajeomTabletWordButton* Entry =
				WidgetTree->ConstructWidget<UBalhwajeomTabletWordButton>();
			Entry->Configure(WordID, Word.DisplayWord);
			Entry->OnWordSelected.AddUniqueDynamic(this, &ThisClass::HandlePuzzleWordSelected);
			EntrySize->AddChild(Entry);
			WB_PuzzleWords->AddChild(EntrySize);
		}
	}
	if (WB_PuzzlePhotos && WidgetTree && Sentence.RequiredPhotoCount > 0)
	{
		WB_PuzzlePhotos->SetVisibility(ESlateVisibility::Visible);
		for (const FName PhotoID : AvailablePuzzlePhotoIDs)
		{
			FPhotoDefinition Photo;
			if (!Investigation->GetPhotoDefinition(PhotoID, Photo))
			{
				continue;
			}
			USizeBox* EntrySize = WidgetTree->ConstructWidget<USizeBox>();
			EntrySize->SetWidthOverride(108.0f);
			EntrySize->SetHeightOverride(50.0f);
			UBalhwajeomTabletPhotoButton* Entry =
				WidgetTree->ConstructWidget<UBalhwajeomTabletPhotoButton>();
			Entry->Configure(PhotoID, Photo.PhotoName);
			Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandlePuzzlePhotoSelected);
			EntrySize->AddChild(Entry);
			WB_PuzzlePhotos->AddChild(EntrySize);
		}
	}
	if (BTN_StatementSubmit && Sentence.SentenceType == ESentenceType::Statement)
	{
		BTN_StatementSubmit->SetVisibility(ESlateVisibility::Visible);
	}
}

void UBalhwajeomTabletWidget::SelectPuzzleWord(const int32 Index)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !AvailablePuzzleWordIDs.IsValidIndex(Index) ||
		!Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) || Sentence.WordSlots.IsEmpty()) return;
	const int32 SlotIndex = Sentence.WordSlots[NextWordSlotCursor % Sentence.WordSlots.Num()].SlotIndex;
	ActiveSubmission.SubmittedWords.RemoveAll([SlotIndex](const FSubmittedWordSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	ActiveSubmission.SubmittedWords.Add({SlotIndex, AvailablePuzzleWordIDs[Index]});
	++NextWordSlotCursor;
	ValidateActivePuzzle(false);
}

void UBalhwajeomTabletWidget::SelectPuzzlePhoto(const FName PhotoID)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !AvailablePuzzlePhotoIDs.Contains(PhotoID) ||
		!Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) ||
		Sentence.PhotoSlots.IsEmpty() || Sentence.RequiredPhotoCount <= 0) return;
	const int32 SelectableSlotCount = FMath::Min(Sentence.RequiredPhotoCount, Sentence.PhotoSlots.Num());
	const int32 SlotIndex = Sentence.PhotoSlots[NextPhotoSlotCursor % SelectableSlotCount].SlotIndex;
	ActiveSubmission.SubmittedPhotos.RemoveAll([SlotIndex](const FSubmittedPhotoSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	ActiveSubmission.SubmittedPhotos.Add({SlotIndex, PhotoID});
	++NextPhotoSlotCursor;
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
}

void UBalhwajeomTabletWidget::ShowPopup(const FText& Title, const FText& Body, UTexture2D* PhotoTexture)
{
	ActiveSentenceID = NAME_None;
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
		USizeBox* EntrySize = WidgetTree->ConstructWidget<USizeBox>();
		EntrySize->SetWidthOverride(220.0f);
		EntrySize->SetHeightOverride(50.0f);
		UBalhwajeomTabletWordButton* Entry =
			WidgetTree->ConstructWidget<UBalhwajeomTabletWordButton>();
		Entry->Configure(Record.WordID, Word.DisplayWord);
		Entry->OnWordSelected.AddUniqueDynamic(this, &ThisClass::HandlePuzzleWordSelected);
		EntrySize->AddChild(Entry);
		WB_PuzzleWords->AddChild(EntrySize);
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

void UBalhwajeomTabletWordButton::Configure(const FName InWordID, const FText& InLabel)
{
	WordID = InWordID;
	OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);

	UTextBlock* Label = NewObject<UTextBlock>(this);
	Label->SetText(InLabel);
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 20;
	Label->SetFont(Font);
	SetContent(Label);
}

void UBalhwajeomTabletWordButton::HandleClicked()
{
	if (!WordID.IsNone())
	{
		OnWordSelected.Broadcast(WordID);
	}
}

void UBalhwajeomTabletFolderButton::Configure(
	const FName InCharacterID,
	const FText& InLabel,
	UTexture2D* IconTexture)
{
	CharacterID = InCharacterID;
	OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);

	UVerticalBox* Layout = NewObject<UVerticalBox>(this);

	if (IconTexture)
	{
		USizeBox* IconBox = NewObject<USizeBox>(this);
		IconBox->SetWidthOverride(140.0f);
		IconBox->SetHeightOverride(112.0f);

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
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 25;
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

void UBalhwajeomTabletWidget::HandleFolderPhotoSelected(const FName PhotoID)
{
	OpenPhoto(PhotoID);
}

void UBalhwajeomTabletWidget::HandlePuzzlePhotoSelected(const FName PhotoID)
{
	SelectPuzzlePhoto(PhotoID);
}

void UBalhwajeomTabletWidget::HandlePuzzleWordSelected(const FName WordID)
{
	const int32 Index = AvailablePuzzleWordIDs.IndexOfByKey(WordID);
	if (Index != INDEX_NONE)
	{
		SelectPuzzleWord(Index);
	}
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
