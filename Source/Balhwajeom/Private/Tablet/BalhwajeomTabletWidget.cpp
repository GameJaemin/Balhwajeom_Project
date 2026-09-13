#include "Tablet/BalhwajeomTabletWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
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
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

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

	/** Uses only the painted 106x39 area of the 144x47 source, excluding its right/bottom padding. */
	void ApplyKeywordHoverBrush(UBorder* Border)
	{
		if (!Border)
		{
			return;
		}
		UTexture2D* HoverTexture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/Balhwajeom/UI/Tablet/StateMent/keyword_hover.keyword_hover"));
		if (!HoverTexture)
		{
			return;
		}

		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(HoverTexture);
		const float UMax = FMath::Clamp(106.0f / HoverTexture->GetSizeX(), 0.0f, 1.0f);
		const float VMax = FMath::Clamp(39.0f / HoverTexture->GetSizeY(), 0.0f, 1.0f);
		Brush.SetUVRegion(FBox2f(FVector2f::ZeroVector, FVector2f(UMax, VMax)));
		Brush.ImageSize = FVector2D(106.0f, 39.0f);
		Border->SetBrush(Brush);
	}
}

UBalhwajeomTabletDetailWidget::UBalhwajeomTabletDetailWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	KeywordFont = ConstructorHelpers::FObjectFinder<UFont>(
		TEXT("/Game/Balhwajeom/UI/JE/Freesentation-4Regular_Font.Freesentation-4Regular_Font")).Object;
	StatementTextFont = ConstructorHelpers::FObjectFinder<UFont>(
		TEXT("/Game/Balhwajeom/UI/JE/UnBatang_Font.UnBatang_Font")).Object;
}

UBalhwajeomTabletWidget::UBalhwajeomTabletWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FolderButtonWidgetClass = TSoftClassPtr<UBalhwajeomTabletFolderButton>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_TabletFolderButton.WBP_TabletFolderButton_C")));
	FileTileWidgetClass = TSoftClassPtr<UBalhwajeomTabletPhotoButton>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_TabletFileTile.WBP_TabletFileTile_C")));
	FolderSectionWidgetClass = TSoftClassPtr<UBalhwajeomTabletFolderSection>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_TabletFolderSection.WBP_TabletFolderSection_C")));
	StatementDetailWidgetClass = TSoftClassPtr<UBalhwajeomTabletDetailWidget>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/UI/Tablet/StateMent/WBP_TabletStatement.WBP_TabletStatement_C")));
	PhotoDetailWidgetClass = TSoftClassPtr<UBalhwajeomTabletDetailWidget>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_TabletPhoto.WBP_TabletPhoto_C")));
}

UBalhwajeomTabletPersonFolderWidget::UBalhwajeomTabletPersonFolderWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBalhwajeomTabletPersonFolderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BTN_FolderClose)
	{
		BTN_FolderClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
	if (BTN_FolderSister) BTN_FolderSister->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSisterTabClicked);
	if (BTN_FolderMother) BTN_FolderMother->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMotherTabClicked);
	if (BTN_FolderBrother) BTN_FolderBrother->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBrotherTabClicked);
	if (SB_EvidencePhotos) SB_EvidencePhotos->OnUserScrolled.AddUniqueDynamic(this, &ThisClass::HandleFolderScrolled);
	RefreshTabVisuals();
}

void UBalhwajeomTabletPersonFolderWidget::SetFolderHeader(
	const FText& FolderName,
	UTexture2D* FolderIcon)
{
	if (TXT_FolderTitle)
	{
		TXT_FolderTitle->SetText(FolderName);
	}
	const FString Title = FolderName.ToString();
	if (Title.Contains(TEXT("어머니"))) SetSelectedTab(1);
	else if (Title.Contains(TEXT("형"))) SetSelectedTab(2);
	else SetSelectedTab(0);
	ClearFeedbackMessage();
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

void UBalhwajeomTabletPersonFolderWidget::SetFeedbackMessage(const FText& Message)
{
	if (TXT_FolderFeedback)
	{
		TXT_FolderFeedback->SetText(Message);
		TXT_FolderFeedback->SetVisibility(
			Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UBalhwajeomTabletPersonFolderWidget::ClearFeedbackMessage()
{
	SetFeedbackMessage(FText::GetEmpty());
}

void UBalhwajeomTabletPersonFolderWidget::SetSelectedTab(const int32 TabIndex)
{
	SelectedTabIndex = FMath::Clamp(TabIndex, 0, 2);
	RefreshTabVisuals();
}

void UBalhwajeomTabletPersonFolderWidget::RefreshTabVisuals()
{
	auto ShowPair = [](UImage* Idle, UImage* Selected, const bool bSelected)
	{
		if (Idle) Idle->SetVisibility(bSelected ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		if (Selected) Selected->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	};
	ShowPair(IMG_FolderSisterIdle, IMG_FolderSisterSelected, SelectedTabIndex == 0);
	ShowPair(IMG_FolderMotherIdle, IMG_FolderMotherSelected, SelectedTabIndex == 1);
	ShowPair(IMG_FolderBrotherIdle, IMG_FolderBrotherSelected, SelectedTabIndex == 2);
}

void UBalhwajeomTabletPersonFolderWidget::HandleSisterTabClicked()
{
	SetSelectedTab(0);
	OnTabRequested.Broadcast(0);
}

void UBalhwajeomTabletPersonFolderWidget::HandleMotherTabClicked()
{
	SetSelectedTab(1);
	OnTabRequested.Broadcast(1);
}

void UBalhwajeomTabletPersonFolderWidget::HandleBrotherTabClicked()
{
	SetSelectedTab(2);
	OnTabRequested.Broadcast(2);
}

void UBalhwajeomTabletPersonFolderWidget::HandleFolderScrolled(const float CurrentOffset)
{
	if (!IMG_FolderScroll || !SB_EvidencePhotos)
	{
		return;
	}
	const float EndOffset = SB_EvidencePhotos->GetScrollOffsetOfEnd();
	const float Ratio = EndOffset > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentOffset / EndOffset, 0.0f, 1.0f)
		: 0.0f;
	// 620px visible track minus the authored 135px thumb.
	IMG_FolderScroll->SetRenderTranslation(FVector2D(0.0f, Ratio * 485.0f));
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
		WBP_PersonFolder->OnTabRequested.AddUniqueDynamic(this, &ThisClass::HandlePersonFolderTabRequested);
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
		UClass* EntryClass = FolderButtonWidgetClass.LoadSynchronous();
		if (!EntryClass)
		{
			EntryClass = UBalhwajeomTabletFolderButton::StaticClass();
		}
		UBalhwajeomTabletFolderButton* Entry = Cast<UBalhwajeomTabletFolderButton>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, EntryClass, NAME_None));
		if (!Entry)
		{
			continue;
		}
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

bool UBalhwajeomTabletWidget::OpenInitialStatement()
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation)
	{
		return false;
	}

	const FName CharacterID = ResolveFolderTabCharacterID(0);
	if (CharacterID.IsNone())
	{
		return false;
	}

	TArray<FSentenceDefinition> Statements;
	Investigation->GetStatementSentencesForCharacter(CharacterID, Statements);
	if (Statements.IsEmpty())
	{
		return false;
	}

	ShowFolder(CharacterID);
	HandleStatementTileSelected(Statements[0].SentenceID);
	return true;
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
		EntrySize->SetWidthOverride(152.0f);
		EntrySize->SetHeightOverride(125.0f);
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

	// Folder buckets: statements and unfinished analyses, completed evidence analyses, then
	// ordinary memory photos that never had an analysis sentence.
	UClass* SectionClass = FolderSectionWidgetClass.LoadSynchronous();
	if (!SectionClass)
	{
		SectionClass = UBalhwajeomTabletFolderSection::StaticClass();
	}
	UBalhwajeomTabletFolderSection* StatementSection = Cast<UBalhwajeomTabletFolderSection>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, SectionClass, NAME_None));
	UBalhwajeomTabletFolderSection* CompletedSection = Cast<UBalhwajeomTabletFolderSection>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, SectionClass, NAME_None));
	UBalhwajeomTabletFolderSection* MemorySection = Cast<UBalhwajeomTabletFolderSection>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, SectionClass, NAME_None));
	if (!StatementSection || !CompletedSection || !MemorySection)
	{
		return;
	}
	StatementSection->Configure(NSLOCTEXT("Tablet", "FolderSectionClues", "단서와 정보"));
	CompletedSection->Configure(NSLOCTEXT("Tablet", "FolderSectionEvidence", "증거 사진"));
	MemorySection->Configure(NSLOCTEXT("Tablet", "FolderSectionMemories", "추억 사진"));

	if (Investigation && VisibleStatementIDs.IsValidIndex(0))
	{
		const FName StatementID = VisibleStatementIDs[0];
		const FText Label = FText::Format(
			NSLOCTEXT("Tablet", "DynamicStatementFileLabel", "{0} 진술서"), FolderName);

		UClass* EntryClass = FileTileWidgetClass.LoadSynchronous();
		if (!EntryClass)
		{
			EntryClass = UBalhwajeomTabletPhotoButton::StaticClass();
		}
		UBalhwajeomTabletPhotoButton* Entry = Cast<UBalhwajeomTabletPhotoButton>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, EntryClass, NAME_None));
		if (!Entry)
		{
			return;
		}
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

		UClass* EntryClass = FileTileWidgetClass.LoadSynchronous();
		if (!EntryClass)
		{
			EntryClass = UBalhwajeomTabletPhotoButton::StaticClass();
		}
		UBalhwajeomTabletPhotoButton* Entry = Cast<UBalhwajeomTabletPhotoButton>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, EntryClass, NAME_None));
		if (!Entry)
		{
			continue;
		}
		Entry->Configure(PhotoID, Photo.PhotoName, GetOrLoadCapturedPhotoTexture(PhotoID));
		Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleFolderPhotoSelected);
		USizeBox* Tile = MakeFolderTileSlot(*WidgetTree, Entry);

		if (Photo.PhotoSentenceID.IsNone())
		{
			MemorySection->AddTile(Tile);
		}
		else if (Investigation->IsSentenceSolved(Photo.PhotoSentenceID))
		{
			CompletedSection->AddTile(Tile);
		}
		else
		{
			StatementSection->AddTile(Tile);
		}
	}

	for (UBalhwajeomTabletFolderSection* Section : {StatementSection, CompletedSection, MemorySection})
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
	FText PopupTitle = Photo.PhotoName;
	FCapturedPhotoRecord CapturedPhoto;
	FEvidenceDefinition Evidence;
	if (Investigation->GetCapturedPhoto(PhotoID, CapturedPhoto) &&
		Investigation->GetEvidenceDefinition(CapturedPhoto.ObjectID, Evidence) &&
		!Evidence.ObjectName.IsEmptyOrWhitespace())
	{
		PopupTitle = FText::Format(
			NSLOCTEXT("Tablet", "PhotoDetailObjectTitle", "{0}"),
			Evidence.ObjectName);
	}
	if (!ShowPopup(PopupTitle, Body, GetOrLoadCapturedPhotoTexture(PhotoID)))
	{
		return;
	}
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
	if (TXT_SelectedPhotoResult)
	{
		TXT_SelectedPhotoResult->SetText(FText::GetEmpty());
		TXT_SelectedPhotoResult->SetVisibility(ESlateVisibility::Collapsed);
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
	Investigation->GetAcquiredWords(AcquiredWords);
	for (const FAcquiredWordRecord& Word : AcquiredWords) AvailablePuzzleWordIDs.Add(Word.WordID);

	if (WB_PuzzleWords && WidgetTree)
	{
		if (Sentence.SentenceType == ESentenceType::Statement)
		{
			TArray<FWordDefinition> OrderedWords;
			Investigation->GetAllWordDefinitions(OrderedWords);
			WB_PuzzleWords->SetVisibility(
				OrderedWords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
			for (const FWordDefinition& Word : OrderedWords)
			{
				if (!AvailablePuzzleWordIDs.Contains(Word.WordID))
				{
					USpacer* EmptyCell = WidgetTree->ConstructWidget<USpacer>();
					EmptyCell->SetSize(FVector2D(106.0f, 39.0f));
					WB_PuzzleWords->AddChild(EmptyCell);
					continue;
				}
				UBalhwajeomTabletWordChip* Chip = Cast<UBalhwajeomTabletWordChip>(
					UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletWordChip::StaticClass(), NAME_None));
				if (Chip)
				{
					Chip->Configure(
						Word.WordID,
						Word.DisplayWord,
						true,
						ActiveDetailWidget ? ActiveDetailWidget->GetKeywordFont() : nullptr,
						ActiveDetailWidget ? ActiveDetailWidget->GetKeywordFontSize() : 14);
					WB_PuzzleWords->AddChild(Chip);
				}
			}
			if (TXT_PuzzleKeywordCount)
			{
				TXT_PuzzleKeywordCount->SetText(FText::Format(
					NSLOCTEXT("Tablet", "StatementKeywordCount", "{0}/{1}"),
					AvailablePuzzleWordIDs.Num(),
					OrderedWords.Num()));
				TXT_PuzzleKeywordCount->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			TArray<FWordDefinition> OrderedWords;
			Investigation->GetAllWordDefinitions(OrderedWords);
			WB_PuzzleWords->SetVisibility(
				OrderedWords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
			for (const FWordDefinition& Word : OrderedWords)
			{
				if (!AvailablePuzzleWordIDs.Contains(Word.WordID))
				{
					USpacer* EmptyCell = WidgetTree->ConstructWidget<USpacer>();
					EmptyCell->SetSize(FVector2D(106.0f, 39.0f));
					WB_PuzzleWords->AddChild(EmptyCell);
					continue;
				}
				UBalhwajeomTabletWordChip* Chip = Cast<UBalhwajeomTabletWordChip>(
					UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletWordChip::StaticClass(), NAME_None));
				if (Chip)
				{
					Chip->Configure(
						Word.WordID,
						Word.DisplayWord,
						true,
						ActiveDetailWidget ? ActiveDetailWidget->GetKeywordFont() : nullptr,
						ActiveDetailWidget ? ActiveDetailWidget->GetKeywordFontSize() : 14);
					WB_PuzzleWords->AddChild(Chip);
				}
			}
		}
	}

	// Interactive sentence (draggable blanks) replaces the raw "[]" template text; see BuildSentenceBuilder.
	BuildSentenceBuilder(Sentence);

	// Photo evidence candidates (WB_PuzzlePhotos) start hidden; clicking an empty WB_PhotoSlots slot
	// reveals them (see OpenPhotoPicker) instead of always showing the row.
	if (WB_PuzzlePhotos)
	{
		WB_PuzzlePhotos->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TXT_PuzzlePhotoLabel)
	{
		TXT_PuzzlePhotoLabel->SetVisibility(ESlateVisibility::Collapsed);
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
	const bool bStatementStyle = Sentence.SentenceType == ESentenceType::Statement;
	UFont* StatementFont = bStatementStyle && ActiveDetailWidget
		? ActiveDetailWidget->GetStatementTextFont()
		: nullptr;
	const int32 StatementFontSize = bStatementStyle && ActiveDetailWidget
		? ActiveDetailWidget->GetStatementTextFontSize()
		: 16;

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		if (!Segments[SegmentIndex].IsEmpty())
		{
			UTextBlock* SegmentText = WidgetTree->ConstructWidget<UTextBlock>();
			SegmentText->SetText(FText::FromString(Segments[SegmentIndex]));
			FSlateFontInfo Font = SegmentText->GetFont();
			Font.Size = bStatementStyle ? StatementFontSize : 27;
			if (bStatementStyle)
			{
				Font.FontObject = StatementFont;
			}
			SegmentText->SetFont(Font);
			SegmentText->SetColorAndOpacity(FSlateColor(
				bStatementStyle ? FLinearColor::Black : FLinearColor::White));
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
			Blank->Configure(
				SegmentIndex,
				bStatementStyle,
				StatementFont,
				StatementFontSize);
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

void UBalhwajeomTabletWidget::SetPhotoPuzzleErrorStyle(const bool bError)
{
	if (!WB_SentenceBuilder)
	{
		return;
	}

	const FLinearColor TextColor = bError
		? FLinearColor(0.761f, 0.471f, 0.471f, 1.0f) // #C27878
		: FLinearColor::White;
	for (int32 ChildIndex = 0; ChildIndex < WB_SentenceBuilder->GetChildrenCount(); ++ChildIndex)
	{
		if (UTextBlock* Segment = Cast<UTextBlock>(WB_SentenceBuilder->GetChildAt(ChildIndex)))
		{
			Segment->SetColorAndOpacity(FSlateColor(TextColor));
		}
		else if (UBalhwajeomTabletSentenceBlank* Blank =
			Cast<UBalhwajeomTabletSentenceBlank>(WB_SentenceBuilder->GetChildAt(ChildIndex)))
		{
			Blank->SetErrorStyle(bError);
		}
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
		PhotoSlotWidget->Configure(
			PhotoSlotDefinition.SlotIndex,
			Sentence.SentenceType == ESentenceType::Statement);
		PhotoSlotWidget->OnPhotoSlotDropped.AddUniqueDynamic(this, &ThisClass::HandlePhotoSlotDropped);
		PhotoSlotWidget->OnPhotoSlotClicked.AddUniqueDynamic(this, &ThisClass::HandlePhotoSlotClicked);
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
	if (Sentence.SentenceType == ESentenceType::PhotoAnalysis)
	{
		SetPhotoPuzzleErrorStyle(false);
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
			UTexture2D* SelectedTexture = GetOrLoadCapturedPhotoTexture(PhotoID);
			PhotoSlotWidget->SetFilled(PhotoDef.PhotoName, SelectedTexture);
			if (Sentence.SentenceType == ESentenceType::Statement && IMG_StatementIllustration && SelectedTexture)
			{
				IMG_StatementIllustration->SetBrushFromTexture(SelectedTexture, true);
				IMG_StatementIllustration->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			if (Sentence.SentenceType == ESentenceType::Statement && TXT_SelectedPhotoResult)
			{
				FSentenceDefinition PhotoSentence;
				const bool bHasSolvedResult = !PhotoDef.PhotoSentenceID.IsNone() &&
					Investigation->IsSentenceSolved(PhotoDef.PhotoSentenceID) &&
					Investigation->GetSentenceDefinition(PhotoDef.PhotoSentenceID, PhotoSentence) &&
					!PhotoSentence.ResultText.IsEmpty();
				TXT_SelectedPhotoResult->SetText(
					bHasSolvedResult ? PhotoSentence.ResultText : FText::GetEmpty());
				TXT_SelectedPhotoResult->SetVisibility(
					bHasSolvedResult ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			}
		}
	}

	if (TXT_PuzzleFeedback)
	{
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Collapsed);
	}

	// The candidate row was only revealed to make this drop possible; hide it again now that its
	// job is done, ready to be re-revealed by clicking another empty slot.
	ClosePhotoPicker();

	EvaluatePuzzleIfComplete();
}

void UBalhwajeomTabletWidget::HandlePhotoSlotClicked(const int32 SlotIndex)
{
	OpenPhotoPicker(SlotIndex);
}

void UBalhwajeomTabletWidget::OpenPhotoPicker(const int32 SlotIndex)
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || (!WB_PuzzlePhotos && !PhotoPickerFolder) || !WidgetTree)
	{
		return;
	}
	PendingPhotoSlotIndex = SlotIndex;
	PhotoPickerCharacterID = ActiveCharacterID;
	PopulatePhotoPicker();
}

void UBalhwajeomTabletWidget::PopulatePhotoPicker()
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation || !WidgetTree)
	{
		return;
	}
	if (WB_PuzzlePhotos) WB_PuzzlePhotos->ClearChildren();
	if (PhotoPickerFolder)
	{
		PhotoPickerFolder->ClearFolderSections();
		FCharacterDefinition Character;
		if (Investigation->GetCharacterDefinition(PhotoPickerCharacterID, Character))
		{
			PhotoPickerFolder->SetFolderHeader(Character.FolderName, DefaultFolderIcon);
		}
	}

	// Eligible: captured, its own analysis solved, and it carries a declaration sentence for some
	// statement's evidence puzzle. Reuses the existing WB_PuzzlePhotos/chip drag-and-drop that was
	// already wired up, just hidden until an empty slot is clicked instead of always visible.
	TArray<FCapturedPhotoRecord> CapturedPhotoRecords;
	Investigation->GetCapturedPhotos(CapturedPhotoRecords);
	bool bAnyPickerPhoto = false;
	UBalhwajeomTabletFolderSection* CompletedSection = nullptr;
	UBalhwajeomTabletFolderSection* NeedsAnalysisSection = nullptr;
	if (PhotoPickerFolder)
	{
		UClass* SectionClass = FolderSectionWidgetClass.LoadSynchronous();
		if (!SectionClass) SectionClass = UBalhwajeomTabletFolderSection::StaticClass();
		CompletedSection = Cast<UBalhwajeomTabletFolderSection>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, SectionClass, NAME_None));
		NeedsAnalysisSection = Cast<UBalhwajeomTabletFolderSection>(
			UUserWidget::CreateWidgetInstance(*WidgetTree, SectionClass, NAME_None));
		if (CompletedSection)
		{
			CompletedSection->Configure(NSLOCTEXT("Tablet", "EvidencePickerCompleted", "완성된 사진"));
		}
		if (NeedsAnalysisSection)
		{
			NeedsAnalysisSection->Configure(NSLOCTEXT("Tablet", "EvidencePickerNeedsAnalysis", "분석이 필요한 사진"));
		}
	}
	for (const FCapturedPhotoRecord& Record : CapturedPhotoRecords)
	{
		FPhotoDefinition PhotoDef;
		if (!Investigation->GetPhotoDefinition(Record.PhotoID, PhotoDef) ||
			PhotoDef.PhotoSentenceID.IsNone() ||
			(!PhotoPickerCharacterID.IsNone() && PhotoDef.CharacterID != PhotoPickerCharacterID))
		{
			continue;
		}

		const bool bAnalysisComplete = Investigation->IsSentenceSolved(PhotoDef.PhotoSentenceID);
		if (CompletedSection && NeedsAnalysisSection)
		{
			UClass* EntryClass = FileTileWidgetClass.LoadSynchronous();
			if (!EntryClass) EntryClass = UBalhwajeomTabletPhotoButton::StaticClass();
			UBalhwajeomTabletPhotoButton* Entry = Cast<UBalhwajeomTabletPhotoButton>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, EntryClass, NAME_None));
			if (!Entry) continue;
			Entry->Configure(PhotoDef.PhotoID, PhotoDef.PhotoName, GetOrLoadCapturedPhotoTexture(PhotoDef.PhotoID));
			if (bAnalysisComplete)
			{
				Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandlePhotoPickerSelected);
				CompletedSection->AddTile(MakeFolderTileSlot(*WidgetTree, Entry));
			}
			else
			{
				Entry->SetRenderOpacity(0.55f);
				Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleUnavailablePhotoPickerSelected);
				NeedsAnalysisSection->AddTile(MakeFolderTileSlot(*WidgetTree, Entry));
			}
		}
		else if (bAnalysisComplete)
		{
			UBalhwajeomTabletPhotoChip* Chip = Cast<UBalhwajeomTabletPhotoChip>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletPhotoChip::StaticClass(), NAME_None));
			if (!Chip) continue;
			Chip->Configure(PhotoDef.PhotoID, PhotoDef.PhotoName, GetOrLoadCapturedPhotoTexture(PhotoDef.PhotoID));
			Chip->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandlePhotoPickerSelected);
			WB_PuzzlePhotos->AddChild(Chip);
		}
		bAnyPickerPhoto = true;
	}
	if (PhotoPickerFolder)
	{
		if (CompletedSection && !CompletedSection->IsEmpty()) PhotoPickerFolder->AddFolderSection(CompletedSection);
		if (NeedsAnalysisSection && !NeedsAnalysisSection->IsEmpty()) PhotoPickerFolder->AddFolderSection(NeedsAnalysisSection);
		PhotoPickerFolder->SetVisibility(ESlateVisibility::Visible);
	}

	if (WB_PuzzlePhotos) WB_PuzzlePhotos->SetVisibility(bAnyPickerPhoto ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (PhotoPickerPanel)
	{
		PhotoPickerPanel->SetVisibility(
			bAnyPickerPhoto ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (TXT_PuzzlePhotoLabel)
	{
		TXT_PuzzlePhotoLabel->SetVisibility(bAnyPickerPhoto ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::ClosePhotoPicker()
{
	PendingPhotoSlotIndex = INDEX_NONE;
	PhotoPickerCharacterID = NAME_None;
	if (PhotoPickerFolder)
	{
		PhotoPickerFolder->ClearFolderSections();
		PhotoPickerFolder->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PhotoPickerPanel)
	{
		PhotoPickerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WB_PuzzlePhotos)
	{
		WB_PuzzlePhotos->ClearChildren();
		WB_PuzzlePhotos->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TXT_PuzzlePhotoLabel)
	{
		TXT_PuzzlePhotoLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::HandlePhotoPickerSelected(const FName PhotoID)
{
	if (PendingPhotoSlotIndex != INDEX_NONE)
	{
		HandlePhotoSlotDropped(PendingPhotoSlotIndex, PhotoID);
	}
}

void UBalhwajeomTabletWidget::HandlePhotoPickerCloseClicked()
{
	ClosePhotoPicker();
}

void UBalhwajeomTabletWidget::HandleUnavailablePhotoPickerSelected(const FName PhotoID)
{
	(void)PhotoID;
	if (PhotoPickerFolder)
	{
		PhotoPickerFolder->SetFeedbackMessage(
			NSLOCTEXT("Tablet", "UnavailableEvidencePhoto", "아직은 증거로 사용할 수 없을 것 같다."));
	}
}

FName UBalhwajeomTabletWidget::ResolveFolderTabCharacterID(const int32 TabIndex) const
{
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	if (!Investigation) return NAME_None;
	TArray<FCharacterDefinition> Characters;
	Investigation->GetAllCharacterDefinitions(Characters);
	const TCHAR* WantedNames[] = {TEXT("여동생"), TEXT("어머니"), TEXT("형")};
	const int32 SafeIndex = FMath::Clamp(TabIndex, 0, 2);
	for (const FCharacterDefinition& Character : Characters)
	{
		if (Character.FolderName.ToString().Contains(WantedNames[SafeIndex])) return Character.CharacterID;
	}
	return Characters.IsValidIndex(SafeIndex) ? Characters[SafeIndex].CharacterID : NAME_None;
}

void UBalhwajeomTabletWidget::HandlePersonFolderTabRequested(const int32 TabIndex)
{
	const FName CharacterID = ResolveFolderTabCharacterID(TabIndex);
	if (!CharacterID.IsNone())
	{
		ActiveCharacterID = CharacterID;
		RefreshFolderContents();
	}
}

void UBalhwajeomTabletWidget::HandlePhotoPickerTabRequested(const int32 TabIndex)
{
	const FName CharacterID = ResolveFolderTabCharacterID(TabIndex);
	if (!CharacterID.IsNone())
	{
		PhotoPickerCharacterID = CharacterID;
		PopulatePhotoPicker();
	}
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
		// A wrong-but-completed evidence photo surfaces its own declaration sentence's ResultText
		// (see ValidateSentence) instead of the generic message.
		TXT_PuzzleFeedback->SetText(
			Result.IsEmpty() ? NSLOCTEXT("Tablet", "WrongEvidence", "잘못된 증거인 것 같다.") : Result);
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Visible);
		if (Sentence.SentenceType == ESentenceType::PhotoAnalysis)
		{
			SetPhotoPuzzleErrorStyle(true);
		}
	}
}

bool UBalhwajeomTabletWidget::ActivateDetailWidget(const bool bStatementDetail)
{
	if (!PopupLayer || !WidgetTree)
	{
		ClearActiveDetailWidgets();
		return false;
	}

	const TSoftClassPtr<UBalhwajeomTabletDetailWidget>& DetailClassAsset = bStatementDetail
		? StatementDetailWidgetClass
		: PhotoDetailWidgetClass;
	UClass* DetailClass = DetailClassAsset.LoadSynchronous();
	if (!DetailClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Failed to load tablet %s detail widget: %s"),
			bStatementDetail ? TEXT("statement") : TEXT("photo"),
			*DetailClassAsset.ToSoftObjectPath().ToString());
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
		ClearActiveDetailWidgets();
		return false;
	}

	UBalhwajeomTabletDetailWidget* Detail = Cast<UBalhwajeomTabletDetailWidget>(
		UUserWidget::CreateWidgetInstance(*WidgetTree, DetailClass, NAME_None));
	if (!Detail)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
		ClearActiveDetailWidgets();
		return false;
	}

	PopupLayer->ClearChildren();
	UOverlaySlot* DetailSlot = PopupLayer->AddChildToOverlay(Detail);
	DetailSlot->SetHorizontalAlignment(HAlign_Fill);
	DetailSlot->SetVerticalAlignment(VAlign_Fill);
	ActiveDetailWidget = Detail;
	BindActiveDetailWidgets();
	return true;
}

void UBalhwajeomTabletWidget::BindActiveDetailWidgets()
{
	if (!ActiveDetailWidget)
	{
		return;
	}

	TXT_PopupTitle = ActiveDetailWidget->GetTitleText();
	TXT_PopupBody = ActiveDetailWidget->GetBodyText();
	IMG_PopupPhoto = ActiveDetailWidget->GetPhotoImage();
	IMG_StatementIllustration = ActiveDetailWidget->GetStatementIllustration();
	BTN_PopupClose = ActiveDetailWidget->GetCloseButton();
	BTN_PlayStoryVoice = ActiveDetailWidget->GetPlayVoiceButton();
	WB_PuzzleWords = ActiveDetailWidget->GetPuzzleWords();
	WB_SentenceBuilder = ActiveDetailWidget->GetSentenceBuilder();
	TXT_PuzzleFeedback = ActiveDetailWidget->GetPuzzleFeedback();
	TXT_SelectedPhotoResult = ActiveDetailWidget->GetSelectedPhotoResult();
	if (TXT_SelectedPhotoResult)
	{
		FSlateFontInfo ResultFont = TXT_SelectedPhotoResult->GetFont();
		if (UFont* ConfiguredFont = ActiveDetailWidget->GetSelectedPhotoResultFont())
		{
			ResultFont.FontObject = ConfiguredFont;
		}
		ResultFont.Size = ActiveDetailWidget->GetSelectedPhotoResultFontSize();
		TXT_SelectedPhotoResult->SetFont(ResultFont);
		TXT_SelectedPhotoResult->SetJustification(ETextJustify::Left);
	}
	TXT_PuzzlePhotoLabel = ActiveDetailWidget->GetPuzzlePhotoLabel();
	WB_PuzzlePhotos = ActiveDetailWidget->GetPuzzlePhotos();
	WB_PhotoSlots = ActiveDetailWidget->GetPhotoSlots();
	BTN_StatementSubmit = ActiveDetailWidget->GetStatementSubmitButton();
	TXT_PuzzleKeywordCount = ActiveDetailWidget->GetPuzzleKeywordCount();
	PhotoPickerPanel = ActiveDetailWidget->GetPhotoPickerPanel();
	BTN_PhotoPickerClose = ActiveDetailWidget->GetPhotoPickerCloseButton();
	PhotoPickerFolder = ActiveDetailWidget->GetPhotoPickerFolder();

	if (BTN_PopupClose)
	{
		BTN_PopupClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePopupCloseClicked);
	}
	if (BTN_PlayStoryVoice)
	{
		BTN_PlayStoryVoice->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePlayStoryVoiceClicked);
	}
	if (BTN_StatementSubmit)
	{
		BTN_StatementSubmit->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStatementSubmitClicked);
	}
	if (BTN_PhotoPickerClose)
	{
		BTN_PhotoPickerClose->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePhotoPickerCloseClicked);
	}
	if (PhotoPickerFolder)
	{
		PhotoPickerFolder->OnBackRequested.AddUniqueDynamic(this, &ThisClass::HandlePhotoPickerCloseClicked);
		PhotoPickerFolder->OnTabRequested.AddUniqueDynamic(this, &ThisClass::HandlePhotoPickerTabRequested);
	}
}

void UBalhwajeomTabletWidget::ClearActiveDetailWidgets()
{
	ActiveDetailWidget = nullptr;
	TXT_PopupTitle = nullptr;
	TXT_PopupBody = nullptr;
	IMG_PopupPhoto = nullptr;
	IMG_StatementIllustration = nullptr;
	BTN_PopupClose = nullptr;
	BTN_PlayStoryVoice = nullptr;
	WB_PuzzleWords = nullptr;
	WB_SentenceBuilder = nullptr;
	TXT_PuzzleFeedback = nullptr;
	TXT_SelectedPhotoResult = nullptr;
	TXT_PuzzlePhotoLabel = nullptr;
	WB_PuzzlePhotos = nullptr;
	WB_PhotoSlots = nullptr;
	BTN_StatementSubmit = nullptr;
	TXT_PuzzleKeywordCount = nullptr;
	PhotoPickerPanel = nullptr;
	BTN_PhotoPickerClose = nullptr;
	PhotoPickerFolder = nullptr;
}

bool UBalhwajeomTabletWidget::ShowPopup(
	const FText& Title,
	const FText& Body,
	UTexture2D* PhotoTexture,
	const bool bStatementDetail)
{
	if (!ActivateDetailWidget(bStatementDetail))
	{
		return false;
	}
	ActiveSentenceID = NAME_None;
	// Reset here; OpenPhoto re-populates this (and BTN_PlayStoryVoice's visibility) right after this
	// call for a photo popup. Callers that open a non-photo popup (e.g. the statement) leave both unset.
	ActivePhotoID = NAME_None;
	if (BTN_PlayStoryVoice)
	{
		BTN_PlayStoryVoice->SetVisibility(ESlateVisibility::Collapsed);
	}
	// Reset here; HandleStatementTileSelected re-populates this right after this call for the
	// statement popup. Every other popup (e.g. a plain photo) leaves it collapsed.
	if (IMG_StatementIllustration)
	{
		IMG_StatementIllustration->SetVisibility(ESlateVisibility::Collapsed);
	}
	ClosePhotoPicker();
	HidePuzzleControls();
	if (TXT_PopupTitle)
	{
		TXT_PopupTitle->SetText(Title);
	}
	if (TXT_PopupBody)
	{
		if (bStatementDetail && ActiveDetailWidget)
		{
			FSlateFontInfo Font = TXT_PopupBody->GetFont();
			Font.FontObject = ActiveDetailWidget->GetStatementTextFont();
			Font.Size = ActiveDetailWidget->GetStatementTextFontSize();
			TXT_PopupBody->SetFont(Font);
		}
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
	return true;
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
	if (TXT_PuzzleKeywordCount)
	{
		TArray<FWordDefinition> OrderedWords;
		Investigation->GetAllWordDefinitions(OrderedWords);
		int32 AcquiredCount = 0;
		for (const FWordDefinition& Word : OrderedWords)
		{
			if (!Investigation->HasAcquiredWord(Word.WordID))
			{
				USpacer* EmptyCell = WidgetTree->ConstructWidget<USpacer>();
				EmptyCell->SetSize(FVector2D(106.0f, 39.0f));
				WB_PuzzleWords->AddChild(EmptyCell);
				continue;
			}

			++AcquiredCount;
			UBalhwajeomTabletWordChip* Chip = Cast<UBalhwajeomTabletWordChip>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletWordChip::StaticClass(), NAME_None));
			if (Chip)
			{
				Chip->Configure(
					Word.WordID,
					Word.DisplayWord,
					true,
					ActiveDetailWidget ? ActiveDetailWidget->GetKeywordFont() : nullptr,
					ActiveDetailWidget ? ActiveDetailWidget->GetKeywordFontSize() : 14);
				WB_PuzzleWords->AddChild(Chip);
			}
		}
		TXT_PuzzleKeywordCount->SetText(FText::Format(
			NSLOCTEXT("Tablet", "StatementKeywordCount", "{0}/{1}"),
			AcquiredCount,
			OrderedWords.Num()));
		TXT_PuzzleKeywordCount->SetVisibility(ESlateVisibility::HitTestInvisible);
		WB_PuzzleWords->SetVisibility(
			OrderedWords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		return;
	}

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
	ClosePhotoPicker();
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

void UBalhwajeomTabletPhotoButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BTN_File)
	{
		BTN_File->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	}
}

void UBalhwajeomTabletPhotoButton::BuildFallbackVisuals()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	BTN_File = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BTN_File"));
	MakeButtonTransparent(BTN_File);
	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>();
	SB_Thumbnail = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SB_Thumbnail"));
	SB_Thumbnail->SetWidthOverride(96.0f);
	SB_Thumbnail->SetHeightOverride(64.0f);
	IMG_Thumbnail = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IMG_Thumbnail"));
	SB_Thumbnail->AddChild(IMG_Thumbnail);
	Layout->AddChildToVerticalBox(SB_Thumbnail);
	TXT_Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_Label"));
	Layout->AddChildToVerticalBox(TXT_Label);
	BTN_File->SetContent(Layout);
	WidgetTree->RootWidget = BTN_File;
	BTN_File->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void UBalhwajeomTabletPhotoButton::Configure(
	const FName InPhotoID,
	const FText& InLabel,
	UTexture2D* Thumbnail)
{
	PhotoID = InPhotoID;
	if (!BTN_File)
	{
		BuildFallbackVisuals();
	}
	if (TXT_Label)
	{
		TXT_Label->SetText(InLabel);
		TXT_Label->SetToolTipText(InLabel);
		TXT_Label->SetJustification(ETextJustify::Left);
		TXT_Label->SetMinDesiredWidth(0.0f);
		TXT_Label->SetAutoWrapText(false);
		TXT_Label->SetClipping(EWidgetClipping::ClipToBounds);
		TXT_Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	}
	if (BTN_File)
	{
		BTN_File->SetToolTipText(InLabel);
	}
	if (IMG_Thumbnail)
	{
		IMG_Thumbnail->SetBrushFromTexture(Thumbnail, true);
	}
	if (SB_Thumbnail)
	{
		SB_Thumbnail->SetVisibility(
			Thumbnail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletPhotoButton::HandleClicked()
{
	if (!PhotoID.IsNone())
	{
		OnPhotoSelected.Broadcast(PhotoID);
	}
}

void UBalhwajeomTabletFolderButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (BTN_Folder)
	{
		BTN_Folder->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
	}
}

void UBalhwajeomTabletFolderButton::BuildFallbackVisuals()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	BTN_Folder = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BTN_Folder"));
	MakeButtonTransparent(BTN_Folder);
	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>();
	IMG_FolderIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IMG_FolderIcon"));
	Layout->AddChildToVerticalBox(IMG_FolderIcon);
	TXT_FolderLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_FolderLabel"));
	Layout->AddChildToVerticalBox(TXT_FolderLabel);
	BTN_Folder->SetContent(Layout);
	WidgetTree->RootWidget = BTN_Folder;
	BTN_Folder->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void UBalhwajeomTabletFolderButton::Configure(
	const FName InCharacterID,
	const FText& InLabel,
	UTexture2D* IconTexture,
	const FSlateFontInfo& InLabelFont)
{
	CharacterID = InCharacterID;
	if (!BTN_Folder)
	{
		BuildFallbackVisuals();
	}
	if (IMG_FolderIcon)
	{
		IMG_FolderIcon->SetBrushFromTexture(IconTexture, true);
	}
	if (TXT_FolderLabel)
	{
		TXT_FolderLabel->SetText(InLabel);
		if (InLabelFont.FontObject || InLabelFont.Size > 0)
		{
			FSlateFontInfo Font = InLabelFont;
			if (!Font.FontObject)
			{
				Font.FontObject = TXT_FolderLabel->GetFont().FontObject;
			}
			TXT_FolderLabel->SetFont(Font);
		}
	}
}

void UBalhwajeomTabletFolderButton::HandleClicked()
{
	if (!CharacterID.IsNone())
	{
		OnFolderSelected.Broadcast(CharacterID);
	}
}

void UBalhwajeomTabletFolderSection::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (HeaderButton)
	{
		HeaderButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHeaderClicked);
	}
}

void UBalhwajeomTabletFolderSection::BuildFallbackVisuals()
{
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
}

void UBalhwajeomTabletFolderSection::Configure(const FText& InTitle, const bool bStartExpanded)
{
	Title = InTitle;
	bExpanded = bStartExpanded;

	if (!HeaderButton || !ArrowText || !TitleText || !ContentWrapBox)
	{
		BuildFallbackVisuals();
	}

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

void UBalhwajeomTabletWordChip::Configure(
	const FName InWordID,
	const FText& InLabel,
	const bool bInStatementStyle,
	UFont* InStatementFont,
	const int32 InStatementFontSize)
{
	WordID = InWordID;
	DisplayLabel = InLabel;
	bStatementStyle = bInStatementStyle;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(
		bStatementStyle
			? FLinearColor(1.0f, 1.0f, 1.0f, 0.0f)
			: FLinearColor(0.30f, 0.24f, 0.16f, 1.0f));
	Background->SetPadding(bStatementStyle ? FMargin(5.0f, 3.0f) : FMargin(10.0f, 6.0f));
	if (bStatementStyle)
	{
		ApplyKeywordHoverBrush(Background);
		Background->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	}

	LabelText = WidgetTree->ConstructWidget<UTextBlock>();
	LabelText->SetText(InLabel);
	LabelText->SetJustification(ETextJustify::Center);
	LabelText->SetColorAndOpacity(FSlateColor(
		bStatementStyle ? FLinearColor::White : FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
	FSlateFontInfo Font = LabelText->GetFont();
	Font.Size = bStatementStyle ? InStatementFontSize : 20;
	if (bStatementStyle)
	{
		Font.FontObject = InStatementFont;
	}
	LabelText->SetFont(Font);
	Background->SetContent(LabelText);

	if (bStatementStyle)
	{
		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>();
		CellSize->SetWidthOverride(106.0f);
		CellSize->SetHeightOverride(39.0f);
		CellSize->SetContent(Background);
		WidgetTree->RootWidget = CellSize;
	}
	else
	{
		WidgetTree->RootWidget = Background;
	}
}

void UBalhwajeomTabletWordChip::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (bStatementStyle && Background && LabelText)
	{
		Background->SetBrushColor(FLinearColor::White);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	}
}

void UBalhwajeomTabletWordChip::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (bStatementStyle && Background && LabelText)
	{
		Background->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
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
	Operation->Pivot = EDragPivot::CenterCenter;

	if (WidgetTree)
	{
		UBorder* DragVisual = WidgetTree->ConstructWidget<UBorder>();
		DragVisual->SetBrushColor(
			bStatementStyle
				? FLinearColor::White
				: FLinearColor(0.30f, 0.24f, 0.16f, 0.9f));
		DragVisual->SetPadding(bStatementStyle ? FMargin(5.0f, 3.0f) : FMargin(10.0f, 6.0f));
		if (bStatementStyle)
		{
			ApplyKeywordHoverBrush(DragVisual);
			DragVisual->SetBrushColor(FLinearColor::FromSRGBColor(FColor(255, 237, 217, 255)));
		}
		UTextBlock* DragLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DragLabel->SetText(DisplayLabel);
		DragLabel->SetJustification(ETextJustify::Center);
		if (LabelText)
		{
			DragLabel->SetFont(LabelText->GetFont());
			DragLabel->SetColorAndOpacity(
				bStatementStyle
					? FSlateColor(FLinearColor::Black)
					: LabelText->GetColorAndOpacity());
		}
		DragVisual->SetContent(DragLabel);
		if (bStatementStyle)
		{
			USizeBox* DragSize = WidgetTree->ConstructWidget<USizeBox>();
			DragSize->SetWidthOverride(106.0f);
			DragSize->SetHeightOverride(39.0f);
			DragSize->SetContent(DragVisual);
			Operation->DefaultDragVisual = DragSize;
		}
		else
		{
			Operation->DefaultDragVisual = DragVisual;
		}
	}

	OutOperation = Operation;
}

void UBalhwajeomTabletSentenceBlank::Configure(
	const int32 InSlotIndex,
	const bool bInStatementStyle,
	UFont* InStatementFont,
	const int32 InStatementFontSize)
{
	SlotIndex = InSlotIndex;
	bStatementStyle = bInStatementStyle;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(
		bStatementStyle
			? FLinearColor(1.0f, 0.72f, 0.72f, 0.72f)
			: FLinearColor::White);
	Background->SetPadding(bStatementStyle ? FMargin(4.0f, 1.0f) : FMargin(4.0f, 1.0f));

	DisplayText = WidgetTree->ConstructWidget<UTextBlock>();
	DisplayText->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = DisplayText->GetFont();
	Font.Size = bStatementStyle ? InStatementFontSize : 27;
	if (bStatementStyle)
	{
		Font.FontObject = InStatementFont;
	}
	DisplayText->SetFont(Font);
	Background->SetContent(DisplayText);

	if (bStatementStyle)
	{
		WidgetTree->RootWidget = Background;
	}
	else
	{
		USizeBox* BlankSize = WidgetTree->ConstructWidget<USizeBox>();
		// Keep the authored empty-blank footprint, but allow a filled keyword to grow
		// horizontally at the same 27px size as the surrounding photo sentence.
		BlankSize->SetMinDesiredWidth(83.0f);
		BlankSize->SetMinDesiredHeight(36.0f);
		BlankSize->SetContent(Background);
		WidgetTree->RootWidget = BlankSize;
	}
	SetEmpty();
}

void UBalhwajeomTabletSentenceBlank::SetEmpty()
{
	FilledWordID = NAME_None;
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(
		bStatementStyle
			? NSLOCTEXT("Tablet", "SentenceBlankPlaceholder", "____")
			: FText::FromString(TEXT("    ")));
	DisplayText->SetColorAndOpacity(FSlateColor(
		bStatementStyle ? FLinearColor::Black : FLinearColor::White));
}

void UBalhwajeomTabletSentenceBlank::SetErrorStyle(const bool bInError)
{
	bErrorStyle = bInError;
	if (!Background || bStatementStyle)
	{
		return;
	}

	Background->SetBrushColor(
		bErrorStyle
			? FLinearColor::Transparent
			: FLinearColor::White);
	if (DisplayText)
	{
		DisplayText->SetColorAndOpacity(FSlateColor(
			bErrorStyle
				? FLinearColor(0.761f, 0.471f, 0.471f, 1.0f)
				: (FilledWordID.IsNone() ? FLinearColor::White : FLinearColor::Black)));
	}
}

void UBalhwajeomTabletSentenceBlank::SetFilled(const FName InWordID, const FText& WordText)
{
	FilledWordID = InWordID;
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(WordText);
	DisplayText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
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
	Operation->Pivot = EDragPivot::CenterCenter;

	if (WidgetTree && DisplayText)
	{
		UBorder* DragVisual = WidgetTree->ConstructWidget<UBorder>();
		DragVisual->SetPadding(FMargin(4.0f, 1.0f));
		ApplyKeywordHoverBrush(DragVisual);
		DragVisual->SetBrushColor(FLinearColor::FromSRGBColor(FColor(255, 237, 217, 255)));
		UTextBlock* DragLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DragLabel->SetText(DisplayText->GetText());
		DragLabel->SetFont(DisplayText->GetFont());
		DragLabel->SetJustification(ETextJustify::Center);
		DragLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		DragVisual->SetContent(DragLabel);
		USizeBox* DragSize = WidgetTree->ConstructWidget<USizeBox>();
		DragSize->SetWidthOverride(bStatementStyle ? 106.0f : 83.0f);
		DragSize->SetHeightOverride(bStatementStyle ? 39.0f : 36.0f);
		DragSize->SetContent(DragVisual);
		Operation->DefaultDragVisual = DragSize;
	}

	OutOperation = Operation;
}

void UBalhwajeomTabletPhotoChip::Configure(const FName InPhotoID, const FText& InLabel, UTexture2D* InThumbnail)
{
	PhotoID = InPhotoID;
	DisplayLabel = InLabel;
	Thumbnail = InThumbnail;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.94f, 0.94f, 0.94f, 1.0f));
	Background->SetPadding(FMargin(8.0f));

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>();

	if (InThumbnail)
	{
		USizeBox* ThumbnailBox = WidgetTree->ConstructWidget<USizeBox>();
		ThumbnailBox->SetWidthOverride(140.0f);
		ThumbnailBox->SetHeightOverride(82.0f);

		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrushFromTexture(InThumbnail, true);
		ThumbnailBox->AddChild(Image);

		UVerticalBoxSlot* ThumbnailSlot = Layout->AddChildToVerticalBox(ThumbnailBox);
		ThumbnailSlot->SetHorizontalAlignment(HAlign_Center);
		ThumbnailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(InLabel);
	Label->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 15;
	Label->SetFont(Font);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.0f)));
	Layout->AddChildToVerticalBox(Label);

	Background->SetContent(Layout);
	USizeBox* TileSize = WidgetTree->ConstructWidget<USizeBox>();
	TileSize->SetWidthOverride(160.0f);
	TileSize->SetHeightOverride(126.0f);
	TileSize->SetContent(Background);
	WidgetTree->RootWidget = TileSize;
}

FReply UBalhwajeomTabletPhotoChip::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UBalhwajeomTabletPhotoChip::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnPhotoSelected.Broadcast(PhotoID);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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

		UVerticalBox* DragLayout = WidgetTree->ConstructWidget<UVerticalBox>();

		if (Thumbnail)
		{
			USizeBox* ThumbnailBox = WidgetTree->ConstructWidget<USizeBox>();
			ThumbnailBox->SetWidthOverride(76.0f);
			ThumbnailBox->SetHeightOverride(48.0f);
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetBrushFromTexture(Thumbnail, true);
			ThumbnailBox->AddChild(Image);
			UVerticalBoxSlot* ThumbnailSlot = DragLayout->AddChildToVerticalBox(ThumbnailBox);
			ThumbnailSlot->SetHorizontalAlignment(HAlign_Center);
			ThumbnailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}

		UTextBlock* DragLabel = WidgetTree->ConstructWidget<UTextBlock>();
		DragLabel->SetText(DisplayLabel);
		FSlateFontInfo Font = DragLabel->GetFont();
		Font.Size = 20;
		DragLabel->SetFont(Font);
		DragLayout->AddChildToVerticalBox(DragLabel);

		DragVisual->SetContent(DragLayout);
		Operation->DefaultDragVisual = DragVisual;
	}

	OutOperation = Operation;
}

void UBalhwajeomTabletPhotoSlot::Configure(
	const int32 InSlotIndex,
	const bool bInStatementStyle)
{
	SlotIndex = InSlotIndex;
	bStatementStyle = bInStatementStyle;

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(
		bStatementStyle ? FLinearColor::White : FLinearColor(0.20f, 0.16f, 0.10f, 1.0f));
	Background->SetPadding(bStatementStyle ? FMargin(0.0f) : FMargin(10.0f, 4.0f));
	if (bStatementStyle)
	{
		if (UTexture2D* ButtonTexture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/Balhwajeom/UI/Tablet/StateMent/evidence_select_button.evidence_select_button")))
		{
			Background->SetBrushFromTexture(ButtonTexture);
		}
	}

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>();

	ThumbnailBox = WidgetTree->ConstructWidget<USizeBox>();
	ThumbnailBox->SetWidthOverride(bStatementStyle ? 101.0f : 76.0f);
	ThumbnailBox->SetHeightOverride(bStatementStyle ? 38.0f : 48.0f);
	ThumbnailImage = WidgetTree->ConstructWidget<UImage>();
	ThumbnailBox->AddChild(ThumbnailImage);
	UVerticalBoxSlot* ThumbnailSlot = Layout->AddChildToVerticalBox(ThumbnailBox);
	ThumbnailSlot->SetHorizontalAlignment(HAlign_Center);
	ThumbnailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	DisplayText = WidgetTree->ConstructWidget<UTextBlock>();
	DisplayText->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = DisplayText->GetFont();
	Font.Size = bStatementStyle ? 13 : 22;
	DisplayText->SetFont(Font);
	Layout->AddChildToVerticalBox(DisplayText);

	Background->SetContent(Layout);
	if (bStatementStyle)
	{
		USizeBox* ClickArea = WidgetTree->ConstructWidget<USizeBox>();
		ClickArea->SetWidthOverride(101.0f);
		ClickArea->SetHeightOverride(38.0f);
		UOverlay* CenteringOverlay = WidgetTree->ConstructWidget<UOverlay>();
		ClickArea->SetContent(CenteringOverlay);
		USizeBox* ButtonVisualSize = WidgetTree->ConstructWidget<USizeBox>();
		ButtonVisualSize->SetWidthOverride(78.0f);
		ButtonVisualSize->SetHeightOverride(29.0f);
		ButtonVisualSize->SetContent(Background);
		UOverlaySlot* VisualSlot = CenteringOverlay->AddChildToOverlay(ButtonVisualSize);
		VisualSlot->SetHorizontalAlignment(HAlign_Center);
		VisualSlot->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = ClickArea;
	}
	else
	{
		WidgetTree->RootWidget = Background;
	}
	SetEmpty();
}

void UBalhwajeomTabletPhotoSlot::SetEmpty()
{
	bFilled = false;
	if (ThumbnailBox)
	{
		ThumbnailBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(
		bStatementStyle
			? FText::GetEmpty()
			: NSLOCTEXT("Tablet", "PhotoSlotPlaceholder", "증거 사진 (클릭해서 선택)"));
	DisplayText->SetColorAndOpacity(FSlateColor(
		bStatementStyle ? FLinearColor::White : FLinearColor(0.62f, 0.56f, 0.46f, 1.0f)));
}

void UBalhwajeomTabletPhotoSlot::SetFilled(const FText& PhotoLabel, UTexture2D* Thumbnail)
{
	bFilled = true;
	// The statement already shows the selected evidence at full size in IMG_StatementIllustration.
	// Keep this slot as the unchanged browse/replace button instead of drawing a duplicate thumbnail
	// and filename on top of that full-size photo.
	if (bStatementStyle)
	{
		if (ThumbnailBox)
		{
			ThumbnailBox->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (DisplayText)
		{
			DisplayText->SetText(FText::GetEmpty());
		}
		return;
	}
	if (ThumbnailBox && ThumbnailImage)
	{
		if (Thumbnail)
		{
			ThumbnailImage->SetBrushFromTexture(Thumbnail, true);
			ThumbnailBox->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ThumbnailBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (!DisplayText)
	{
		return;
	}
	DisplayText->SetText(PhotoLabel);
	DisplayText->SetColorAndOpacity(FSlateColor(
		bStatementStyle ? FLinearColor::White : FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
}

FReply UBalhwajeomTabletPhotoSlot::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnPhotoSlotClicked.Broadcast(SlotIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
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
	if (!ShowPopup(
		FText::FromString(TEXT("진술서")),
		FText::Format(
			NSLOCTEXT("Tablet", "StatementPopup", "반증\n{0}"),
			Answer),
		nullptr,
		true))
	{
		return;
	}
	if (IMG_StatementIllustration)
	{
		// Do not reveal the authored correct answer. The evidence area is filled only after the
		// player chooses a photo from the folder-style picker.
		IMG_StatementIllustration->SetVisibility(ESlateVisibility::Collapsed);
	}
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
