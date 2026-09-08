#include "Tablet/BalhwajeomTabletWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Animation/WidgetAnimation.h"
#include "Tablet/BalhwajeomMessengerWidget.h"
#include "Investigation/BalhwajeomInvestigationSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
	int32 ToPageIndex(const ETabletPage Page)
	{
		return static_cast<int32>(Page);
	}

	FText GetFamilyMemberText(const EFamilyMember FamilyMember)
	{
		switch (FamilyMember)
		{
		case EFamilyMember::Brother:
			return FText::FromString(TEXT("형"));
		case EFamilyMember::Mother:
			return FText::FromString(TEXT("어머니"));
		case EFamilyMember::Sister:
		default:
			return FText::FromString(TEXT("여동생"));
		}
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

	if (BTN_Sister)
	{
		BTN_Sister->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSisterClicked);
	}
	if (BTN_Brother)
	{
		BTN_Brother->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBrotherClicked);
	}
	if (BTN_Mother)
	{
		BTN_Mother->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMotherClicked);
	}
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
	if (BTN_FolderBack)
	{
		BTN_FolderBack->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
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
	if (BTN_EvidencePhoto01)
	{
		BTN_EvidencePhoto01->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePhoto01Clicked);
	}
	if (BTN_EvidencePhoto02)
	{
		BTN_EvidencePhoto02->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePhoto02Clicked);
	}
	if (BTN_EvidenceStatement)
	{
		BTN_EvidenceStatement->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStatementClicked);
	}
	if (BTN_PopupClose)
	{
		BTN_PopupClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePopupCloseClicked);
	}
	if (BTN_PuzzleWord01) BTN_PuzzleWord01->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePuzzleWord01Clicked);
	if (BTN_PuzzleWord02) BTN_PuzzleWord02->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePuzzleWord02Clicked);
	if (BTN_PuzzleWord03) BTN_PuzzleWord03->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePuzzleWord03Clicked);
	if (BTN_PuzzlePhoto01) BTN_PuzzlePhoto01->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePuzzlePhoto01Clicked);
	if (BTN_PuzzlePhoto02) BTN_PuzzlePhoto02->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePuzzlePhoto02Clicked);
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

	CurrentPage = ETabletPage::Home;
	PageHistory.Reset();
	if (WidgetSwitcher_TabletPage)
	{
		WidgetSwitcher_TabletPage->SetActiveWidgetIndex(ToPageIndex(CurrentPage));
	}
	HidePopup();
	UpdateUnreadBadge();
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

void UBalhwajeomTabletWidget::ShowFolder(const EFamilyMember FamilyMember)
{
	ActiveFamilyMember = FamilyMember;
	if (TXT_FolderTitle)
	{
		TXT_FolderTitle->SetText(GetFamilyMemberText(FamilyMember));
	}
	SetTabletPage(ETabletPage::PersonFolder);
	RefreshFolderContents();
}

FName UBalhwajeomTabletWidget::GetActiveCharacterID() const
{
	switch (ActiveFamilyMember)
	{
	case EFamilyMember::Brother:
		return TEXT("BROTHER");
	case EFamilyMember::Mother:
		return TEXT("MOTHER");
	case EFamilyMember::Sister:
	default:
		return TEXT("SISTER");
	}
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
		if (TXT_FolderTitle && !Statements.IsEmpty() && !Statements[0].FolderName.IsEmpty())
		{
			TXT_FolderTitle->SetText(Statements[0].FolderName);
		}
	}

	auto UpdateFileButton = [this, Investigation](UButton* Button, const FName TextWidgetName, const FName ItemID, bool bPhoto)
	{
		if (!Button)
		{
			return;
		}
		const bool bAvailable = Investigation && !ItemID.IsNone();
		Button->SetVisibility(bAvailable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button->SetIsEnabled(bAvailable);
		UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(TextWidgetName));
		if (!Label || !bAvailable)
		{
			return;
		}
		if (bPhoto)
		{
			FPhotoDefinition Photo;
			if (Investigation->GetPhotoDefinition(ItemID, Photo))
			{
				const bool bComplete = Photo.PhotoSentenceID.IsNone() || Investigation->IsSentenceSolved(Photo.PhotoSentenceID);
				Label->SetText(FText::Format(
					NSLOCTEXT("Tablet", "PhotoFileLabel", "{0}  {1}"),
					bComplete ? FText::FromString(TEXT("✓")) : FText::FromString(TEXT("?")),
					Photo.PhotoName));
			}
		}
		else
		{
			FSentenceDefinition Sentence;
			if (Investigation->GetSentenceDefinition(ItemID, Sentence))
			{
				Label->SetText(FText::Format(
					NSLOCTEXT("Tablet", "StatementFileLabel", "{0}  진술서"),
					Investigation->IsSentenceSolved(ItemID)
						? FText::FromString(TEXT("✓")) : FText::FromString(TEXT("?"))));
			}
		}
	};

	UpdateFileButton(BTN_EvidencePhoto01, TEXT("TXT_EvidencePhoto01"),
		VisiblePhotoIDs.IsValidIndex(0) ? VisiblePhotoIDs[0] : NAME_None, true);
	UpdateFileButton(BTN_EvidencePhoto02, TEXT("TXT_EvidencePhoto02"),
		VisiblePhotoIDs.IsValidIndex(1) ? VisiblePhotoIDs[1] : NAME_None, true);
	UpdateFileButton(BTN_EvidenceStatement, TEXT("TXT_EvidenceStatement"),
		VisibleStatementIDs.IsValidIndex(0) ? VisibleStatementIDs[0] : NAME_None, false);
}

void UBalhwajeomTabletWidget::OpenPhotoAtIndex(const int32 Index)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || !VisiblePhotoIDs.IsValidIndex(Index))
	{
		return;
	}
	FPhotoDefinition Photo;
	if (!Investigation->GetPhotoDefinition(VisiblePhotoIDs[Index], Photo))
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
		Body = Photo.WorldStoryText;
	}
	ShowPopup(Photo.PhotoName, Body);
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
	for (UButton* Button : {BTN_PuzzleWord01.Get(), BTN_PuzzleWord02.Get(), BTN_PuzzleWord03.Get(),
		BTN_PuzzlePhoto01.Get(), BTN_PuzzlePhoto02.Get(), BTN_StatementSubmit.Get()})
	{
		if (Button) Button->SetVisibility(ESlateVisibility::Collapsed);
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
	Investigation->GetAcquiredWords(AcquiredWords);
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

	auto SetChoiceButton = [this](UButton* Button, FName LabelName, FName ID, const FText& Label)
	{
		if (!Button) return;
		const bool bVisible = !ID.IsNone();
		Button->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button->SetIsEnabled(bVisible);
		if (bVisible)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(LabelName))) Text->SetText(Label);
		}
	};
	UButton* WordButtons[] = {BTN_PuzzleWord01, BTN_PuzzleWord02, BTN_PuzzleWord03};
	const FName WordLabels[] = {TEXT("TXT_PuzzleWord01"), TEXT("TXT_PuzzleWord02"), TEXT("TXT_PuzzleWord03")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FText Label;
		FName ID = AvailablePuzzleWordIDs.IsValidIndex(Index) ? AvailablePuzzleWordIDs[Index] : NAME_None;
		FWordDefinition Word;
		if (!ID.IsNone() && Investigation->GetWordDefinition(ID, Word)) Label = Word.DisplayWord;
		SetChoiceButton(WordButtons[Index], WordLabels[Index], ID, Label);
	}
	UButton* PhotoButtons[] = {BTN_PuzzlePhoto01, BTN_PuzzlePhoto02};
	const FName PhotoLabels[] = {TEXT("TXT_PuzzlePhoto01"), TEXT("TXT_PuzzlePhoto02")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FText Label;
		FName ID = AvailablePuzzlePhotoIDs.IsValidIndex(Index) ? AvailablePuzzlePhotoIDs[Index] : NAME_None;
		FPhotoDefinition Photo;
		if (!ID.IsNone() && Investigation->GetPhotoDefinition(ID, Photo)) Label = Photo.PhotoName;
		SetChoiceButton(PhotoButtons[Index], PhotoLabels[Index], ID, Label);
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

void UBalhwajeomTabletWidget::SelectPuzzlePhoto(const int32 Index)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !AvailablePuzzlePhotoIDs.IsValidIndex(Index) ||
		!Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) || Sentence.PhotoSlots.IsEmpty()) return;
	const int32 SlotIndex = Sentence.PhotoSlots[NextPhotoSlotCursor % Sentence.PhotoSlots.Num()].SlotIndex;
	ActiveSubmission.SubmittedPhotos.RemoveAll([SlotIndex](const FSubmittedPhotoSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	ActiveSubmission.SubmittedPhotos.Add({SlotIndex, AvailablePuzzlePhotoIDs[Index]});
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
		RefreshFolderContents();
	}
}

void UBalhwajeomTabletWidget::ShowPopup(const FText& Title, const FText& Body)
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
	if (PopupLayer)
	{
		PopupLayer->SetVisibility(ESlateVisibility::Visible);
	}
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

void UBalhwajeomTabletWidget::HandleSisterClicked()
{
	ShowFolder(EFamilyMember::Sister);
}

void UBalhwajeomTabletWidget::HandleBrotherClicked()
{
	ShowFolder(EFamilyMember::Brother);
}

void UBalhwajeomTabletWidget::HandleMotherClicked()
{
	ShowFolder(EFamilyMember::Mother);
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
	SetTabletPage(ETabletPage::Internet);
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

void UBalhwajeomTabletWidget::HandlePhoto01Clicked()
{
	OpenPhotoAtIndex(0);
}

void UBalhwajeomTabletWidget::HandlePhoto02Clicked()
{
	OpenPhotoAtIndex(1);
}

void UBalhwajeomTabletWidget::HandleStatementClicked()
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || !VisibleStatementIDs.IsValidIndex(0))
	{
		return;
	}
	FSentenceDefinition Statement;
	if (!Investigation->GetSentenceDefinition(VisibleStatementIDs[0], Statement))
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

void UBalhwajeomTabletWidget::HandlePuzzleWord01Clicked() { SelectPuzzleWord(0); }
void UBalhwajeomTabletWidget::HandlePuzzleWord02Clicked() { SelectPuzzleWord(1); }
void UBalhwajeomTabletWidget::HandlePuzzleWord03Clicked() { SelectPuzzleWord(2); }
void UBalhwajeomTabletWidget::HandlePuzzlePhoto01Clicked() { SelectPuzzlePhoto(0); }
void UBalhwajeomTabletWidget::HandlePuzzlePhoto02Clicked() { SelectPuzzlePhoto(1); }
void UBalhwajeomTabletWidget::HandleStatementSubmitClicked() { ValidateActivePuzzle(true); }
