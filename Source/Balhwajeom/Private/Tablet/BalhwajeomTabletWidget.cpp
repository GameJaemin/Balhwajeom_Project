#include "Tablet/BalhwajeomTabletWidget.h"

#include "Story/StoryStateTags.h"
#include "Tutorial/BalhwajeomTutorialOverlayTriggers.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Internationalization/BreakIterator.h"
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
	// Page_PersonFolder was moved out of WidgetSwitcher_TabletPage into its own overlay layer above
	// it (see UBalhwajeomTabletWidget::SetTabletPage), so it no longer occupies a switcher slot --
	// every remaining page shifts down by one index, and PersonFolder itself never maps to one.
	int32 ToPageIndex(const ETabletPage Page)
	{
		switch (Page)
		{
		case ETabletPage::Home: return 0;
		case ETabletPage::Messenger: return 1;
		case ETabletPage::Internet: return 2;
		case ETabletPage::Memo: return 3;
		default: return 0;
		}
	}

	// Timings for UBalhwajeomTabletWidget's solved-puzzle transition (flash -> hold -> converge ->
	// reveal). Tuned for a snappy "got it" beat rather than a long cutscene-style pause.
	constexpr float PuzzleSuccessFlashDuration = 0.12f;
	constexpr float PuzzleSuccessHoldDuration = 0.5f;
	// Converge and Reveal both explode their text into one UTextBlock per character (see
	// BuildRandomFadeCharacters) and fade each character out/in at its own random point within these
	// windows, so letters disappear/appear in scattered order rather than all together or in a
	// left-to-right sweep. PuzzleSuccessCharFadeDuration is how long any single character's own
	// fade takes; it must stay shorter than either window (each character's random start delay is
	// clamped to [0, window - PuzzleSuccessCharFadeDuration] so it still finishes inside the window).
	constexpr float PuzzleSuccessConvergeDuration = 0.6f;
	constexpr float PuzzleSuccessRevealFadeDuration = 0.6f;
	constexpr float PuzzleSuccessCharFadeDuration = 0.2f;
	// Short crossfade from the random-fade character grid to TXT_PopupBody once Reveal finishes (see
	// BeginHandoffStage) -- long enough to mask the two widgets not necessarily sharing the exact
	// same position, short enough that it doesn't read as its own separate beat.
	constexpr float PuzzleSuccessHandoffDuration = 0.12f;

	// Matches UBalhwajeomTabletSentenceBlank::Configure's own BlankSize->SetMinDesiredWidth(60.0f):
	// BuildFlatSolvedSentenceText brackets each blank's word between these two sentinels so
	// BuildRandomFadeCharacters can reserve the same minimum width around it, instead of the word
	// visibly shrinking to its bare text width (and every word after it sliding over to meet it) the
	// instant the blank's own box disappears at the start of Converge.
	constexpr TCHAR BlankWordRunStart = TEXT('\x01');
	constexpr TCHAR BlankWordRunEnd = TEXT('\x02');
	constexpr float PuzzleBlankMinWidth = 60.0f;

	// One contiguous run of a line's text: either plain sentence text, or a former blank's word (see
	// BlankWordRunStart/End) that BuildRandomFadeCharacters wraps in a minimum-width box.
	struct FTextRun
	{
		FString Text;
		bool bIsBlankWord = false;
	};

	TArray<FTextRun> ParseTextRuns(const FString& Line)
	{
		TArray<FTextRun> Runs;
		FString Current;
		bool bInBlankWord = false;
		auto FlushCurrent = [&Runs, &Current, &bInBlankWord]()
		{
			if (!Current.IsEmpty())
			{
				Runs.Add({Current, bInBlankWord});
				Current.Reset();
			}
		};
		for (const TCHAR Ch : Line)
		{
			if (Ch == BlankWordRunStart)
			{
				FlushCurrent();
				bInBlankWord = true;
			}
			else if (Ch == BlankWordRunEnd)
			{
				FlushCurrent();
				bInBlankWord = false;
			}
			else
			{
				Current.AppendChar(Ch);
			}
		}
		FlushCurrent();
		return Runs;
	}

	// Splits Text into user-perceived "characters" (grapheme clusters) for BuildRandomFadeCharacters,
	// instead of FString::Mid(i, 1)'s raw UTF-16-code-unit slicing. Matters for Hangul: if the source
	// text is decomposed (NFD -- a syllable stored as separate leading/vowel/trailing Jamo code
	// points instead of one precomposed block), slicing by code unit splits a single syllable across
	// several widgets, and the font can only compose Jamo into a syllable within one continuous text
	// run -- so each widget ends up showing a disconnected Jamo fragment instead of a whole letter.
	// FBreakIterator's character-boundary iterator is the same ICU-backed logic Slate's own editable
	// text widgets use for cursor movement, so it always groups a full grapheme (composed or not)
	// into one slice.
	TArray<FString> SplitIntoGraphemes(const FString& Text)
	{
		TArray<FString> Graphemes;
		if (Text.IsEmpty())
		{
			return Graphemes;
		}
		const TSharedRef<IBreakIterator> CharIterator = FBreakIterator::CreateCharacterBoundaryIterator();
		CharIterator->SetString(Text);
		int32 PreviousPosition = 0;
		for (int32 CurrentPosition = CharIterator->MoveToNext(); CurrentPosition != INDEX_NONE;
			 CurrentPosition = CharIterator->MoveToNext())
		{
			Graphemes.Add(Text.Mid(PreviousPosition, CurrentPosition - PreviousPosition));
			PreviousPosition = CurrentPosition;
		}
		return Graphemes;
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

	/** Manually shortens InText to fit MaxWidth, appending "...", instead of relying on UMG's
	 * ETextOverflowPolicy::Ellipsis -- that overflow policy only truncates correctly (visible "..."
	 * at the tail) for left-justified text. TXT_Label is center-justified by design, which made
	 * long labels get hard-clipped on both sides instead, with no ellipsis shown.
	 *
	 * The kept prefix is measured against MaxWidth on its own (not prefix+"..." together), so the
	 * readable word is never shortened by one more character just to make room for the dots -- the
	 * trailing "..." is allowed to spill past MaxWidth instead and rely on the label's own
	 * ClipToBounds to crop it, which is preferable to ever clipping mid-word. */
	FText TruncateLabelToFit(const FText& InText, const FSlateFontInfo& FontInfo, const float MaxWidth)
	{
		const FString FullString = InText.ToString();
		if (MaxWidth <= 0.0f || FullString.IsEmpty())
		{
			return InText;
		}

		const TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		if (FontMeasure->Measure(FullString, FontInfo).X <= MaxWidth)
		{
			return InText;
		}

		// Binary search for the longest prefix of FullString that alone fits within MaxWidth.
		int32 Low = 0;
		int32 High = FullString.Len();
		FString Best;
		while (Low <= High)
		{
			const int32 Mid = (Low + High) / 2;
			const FString Candidate = FullString.Left(Mid);
			if (FontMeasure->Measure(Candidate, FontInfo).X <= MaxWidth)
			{
				Best = Candidate;
				Low = Mid + 1;
			}
			else
			{
				High = Mid - 1;
			}
		}
		return FText::FromString(Best + TEXT("..."));
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
	KeywordDropSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/Audio/SFX/Sentence/keyword_drop.keyword_drop")));
	SentenceCorrectSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/Audio/SFX/Sentence/Wave.Wave")));
	SentenceErrorSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/Audio/SFX/Sentence/error.error")));
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

	// Only BTN_FolderSister's slot is used now to show whichever family member's folder is
	// actually open; BTN_FolderMother/BTN_FolderBrother and every *Selected image are unused.
	const FString Title = FolderName.ToString();
	UTexture2D* ActiveFolderIcon = SisterFolderIcon;
	if (Title.Contains(TEXT("어머니"))) ActiveFolderIcon = MotherFolderIcon;
	else if (Title.Contains(TEXT("형"))) ActiveFolderIcon = BrotherFolderIcon;

	if (IMG_FolderSisterIdle)
	{
		IMG_FolderSisterIdle->SetBrushFromTexture(ActiveFolderIcon, true);
		IMG_FolderSisterIdle->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (IMG_FolderSisterSelected)
	{
		IMG_FolderSisterSelected->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (BTN_FolderMother)
	{
		BTN_FolderMother->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (BTN_FolderBrother)
	{
		BTN_FolderBrother->SetVisibility(ESlateVisibility::Collapsed);
	}

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
	// IMG_FolderSisterIdle/Selected are no longer driven by SelectedTabIndex -- SetFolderHeader
	// sets IMG_FolderSisterIdle's texture directly and always leaves it visible/Selected
	// collapsed, since only BTN_FolderSister's slot is used now. Mother/Brother stay untouched
	// here too since their buttons are permanently hidden by SetFolderHeader.
	auto ShowPair = [](UImage* Idle, UImage* Selected, const bool bSelected)
	{
		if (Idle) Idle->SetVisibility(bSelected ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		if (Selected) Selected->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	};
	ShowPair(IMG_FolderMotherIdle, IMG_FolderMotherSelected, SelectedTabIndex == 1);
	ShowPair(IMG_FolderBrotherIdle, IMG_FolderBrotherSelected, SelectedTabIndex == 2);
}

// BTN_FolderSister/Mother/Brother no longer switch between family members' folders (only
// BTN_FolderSister's slot/icon is used at all now, see SetFolderHeader) -- with just one real
// character in the current data, switching to "mother"/"brother" had nowhere valid to resolve to
// and fell back to whichever character sat at index 0 (sister), which looked like clicking those
// icons "closed back to the sister folder" instead of actually closing. All three now just close
// the folder like BTN_FolderClose.
void UBalhwajeomTabletPersonFolderWidget::HandleSisterTabClicked()
{
	HandleCloseClicked();
}

void UBalhwajeomTabletPersonFolderWidget::HandleMotherTabClicked()
{
	HandleCloseClicked();
}

void UBalhwajeomTabletPersonFolderWidget::HandleBrotherTabClicked()
{
	HandleCloseClicked();
}

void UBalhwajeomTabletPersonFolderWidget::HandleFolderScrolled(const float CurrentOffset)
{
	if (!IMG_FolderScroll || !SB_EvidencePhotos)
	{
		return;
	}
	const float EndOffset = SB_EvidencePhotos->GetScrollOffsetOfEnd();
	// Nothing to scroll (all content already fits): hide the indicator instead of leaving a
	// scrollbar stuck at the top for a folder the player can never actually scroll.
	IMG_FolderScroll->SetVisibility(
		EndOffset > KINDA_SMALL_NUMBER ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	const float Ratio = EndOffset > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentOffset / EndOffset, 0.0f, 1.0f)
		: 0.0f;
	// 620px visible track minus the authored 135px thumb.
	IMG_FolderScroll->SetRenderTranslation(FVector2D(0.0f, Ratio * 285.0f));
}

void UBalhwajeomTabletPersonFolderWidget::RefreshScrollIndicator()
{
	if (!SB_EvidencePhotos)
	{
		return;
	}
	// Content was just repopulated (ClearFolderSections/AddFolderSection), which never fires
	// SB_EvidencePhotos's own OnUserScrolled -- re-run the same visibility/position check with
	// its current offset (0 right after a refresh) instead of waiting for the player to scroll.
	HandleFolderScrolled(SB_EvidencePhotos->GetScrollOffset());
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

	// Existing generated assets created Page_PersonFolder without Is Variable, so BindWidgetOptional
	// cannot populate the property. Resolve it by name to keep those assets usable without rebuilding.
	if (!Page_PersonFolder && WidgetTree)
	{
		Page_PersonFolder = WidgetTree->FindWidget(TEXT("Page_PersonFolder"));
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

	// Keep the folder in the same logical-screen canvas as the switcher. Some existing assets
	// either keep it inside the switcher or attach it to Canvas_ViewportRoot with a 1x1 slot;
	// both layouts make the folder invisible when opened.
	if (Page_PersonFolder && WidgetSwitcher_TabletPage)
	{
		if (UCanvasPanel* ScreenLayers = Cast<UCanvasPanel>(WidgetSwitcher_TabletPage->GetParent()))
		{
			if (Page_PersonFolder->GetParent() != ScreenLayers)
			{
				Page_PersonFolder->RemoveFromParent();
				ScreenLayers->AddChildToCanvas(Page_PersonFolder);
			}

			if (UCanvasPanelSlot* FolderSlot = Cast<UCanvasPanelSlot>(Page_PersonFolder->Slot))
			{
				FolderSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				FolderSlot->SetOffsets(FMargin(0.0f));
				FolderSlot->SetZOrder(5);
			}
		}
	}

	if (WidgetSwitcher_TabletPage)
	{
		WidgetSwitcher_TabletPage->SetActiveWidgetIndex(ToPageIndex(CurrentPage));
	}
	if (Page_PersonFolder)
	{
		Page_PersonFolder->SetVisibility(ESlateVisibility::Collapsed);
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
	// Do not return early for the currently selected page. A previous attempt may have changed
	// CurrentPage while its optional widget binding was unresolved, leaving the visual collapsed.
	const bool bPageChanged = NewPage != CurrentPage;
	if (bPageChanged && bAddToHistory)
	{
		PageHistory.Add(CurrentPage);
	}

	CurrentPage = NewPage;
	HidePopup();

	// Resolve by designer name on every navigation as a fallback for legacy WBP_Tablet assets
	// whose Page_PersonFolder was created without Is Variable.
	if (!Page_PersonFolder && WidgetTree)
	{
		Page_PersonFolder = WidgetTree->FindWidget(TEXT("Page_PersonFolder"));
	}

	if (Page_PersonFolder)
	{
		Page_PersonFolder->SetVisibility(
			NewPage == ETabletPage::PersonFolder ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// PersonFolder now renders as an overlay above the switcher (Page_PersonFolder) instead of a
	// switcher page of its own, so opening it leaves the switcher on whatever page was already
	// active underneath (in practice always Home, since that's the only place a folder is opened
	// from) -- only every other page actually drives the switcher.
	if (WidgetSwitcher_TabletPage && NewPage != ETabletPage::PersonFolder)
	{
		WidgetSwitcher_TabletPage->SetActiveWidgetIndex(ToPageIndex(NewPage));
	}

	// Every route into and out of the sister's folder passes through here, so the tutorial
	// trigger tracks the page itself rather than the one button that happens to open it.
	static const FName SisterCharacterID(TEXT("CHARACTER_SISTER"));
	const bool bSisterFolderOpen =
		NewPage == ETabletPage::PersonFolder && ActiveCharacterID == SisterCharacterID;
	if (bSisterFolderOpen)
	{
		BalhwajeomTutorialOverlayTriggers::Set(
			this, BalhwajeomGameplayTags::Tutorial_Trigger_SisterFolderOpened);
	}
	else
	{
		BalhwajeomTutorialOverlayTriggers::Clear(
			this, BalhwajeomGameplayTags::Tutorial_Trigger_SisterFolderOpened);
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
	/** Wraps a runtime-created file tile, clipped so a long label's ellipsis-truncated text can
	 * never visually spill into the neighboring tile. Defaults match every regular photo/memo-
	 * tile; the single folder statement tile passes a larger explicit size instead. */
	USizeBox* MakeFolderTileSlot(UWidgetTree& WidgetTree, UWidget* Content, const float Width = 144.0f, const float Height = 140.0f)
	{
		USizeBox* EntrySize = WidgetTree.ConstructWidget<USizeBox>();
		EntrySize->SetWidthOverride(Width);
		EntrySize->SetHeightOverride(Height);
		EntrySize->SetClipping(EWidgetClipping::ClipToBounds);
		// Content (the whole file tile) is usually shorter than this box, since the statement
		// tile and regular photo tiles pass different Height here on purpose. Bottom-align it so
		// the label at its bottom lines up across tiles once AddTile also bottom-aligns these
		// outer boxes against each other in the section's wrap row.
		if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(EntrySize->AddChild(Content)))
		{
			ContentSlot->SetVerticalAlignment(VAlign_Bottom);
		}
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
		// Larger than the shared Class Default thumbnail size so the folder's single statement
		// file's thumbnail stands out too, not just its outer tile box below. Must be called
		// before Configure() -- see SetThumbnailSizeOverride.
		Entry->SetThumbnailSizeOverride(144.0f, 100.0f);
		Entry->Configure(StatementID, Label, StatementFileIcon);
		Entry->OnPhotoSelected.AddUniqueDynamic(this, &ThisClass::HandleStatementTileSelected);
		// Larger than the regular 152x125 photo/memory tiles so the folder's single statement
		// file stands out. Adjust these two numbers directly to resize just this tile.
		StatementSection->AddTile(MakeFolderTileSlot(*WidgetTree, Entry, 144.0f, 140.0f));
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
	if (WBP_PersonFolder)
	{
		WBP_PersonFolder->RefreshScrollIndicator();
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
	bool bIsSolvedAnalysisResult = false;
	if (!Photo.PhotoSentenceID.IsNone())
	{
		FSentenceDefinition Analysis;
		if (Investigation->GetSentenceDefinition(Photo.PhotoSentenceID, Analysis))
		{
			bIsSolvedAnalysisResult = Investigation->IsSentenceSolved(Photo.PhotoSentenceID);
			Body = bIsSolvedAnalysisResult ? Analysis.ResultText : Analysis.SentenceTemplate;
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
	ApplyPopupBodyResultStyle(bIsSolvedAnalysisResult);
	if (BTN_PlayStoryVoice)
	{
		BTN_PlayStoryVoice->SetVisibility(
			Photo.StoryVoice.IsNull() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (!Photo.PhotoSentenceID.IsNone() && !bIsSolvedAnalysisResult)
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

void UBalhwajeomTabletWidget::HidePuzzleWordAndPhotoControls()
{
	if (WB_PuzzleWords)
	{
		WB_PuzzleWords->ClearChildren();
		WB_PuzzleWords->SetVisibility(ESlateVisibility::Collapsed);
	}
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
	if (BTN_StatementSubmit)
	{
		BTN_StatementSubmit->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::ClearSentenceBuilder()
{
	if (WB_SentenceBuilder)
	{
		WB_SentenceBuilder->ClearChildren();
		WB_SentenceBuilder->SetVisibility(ESlateVisibility::Collapsed);
	}
	ActiveBlanksBySlot.Reset();
	ActiveSentenceSegments.Reset();
}

void UBalhwajeomTabletWidget::HidePuzzleControls()
{
	HidePuzzleWordAndPhotoControls();
	ClearSentenceBuilder();
	if (TXT_PopupBody)
	{
		// Restored here; RefreshPuzzleControls/BuildSentenceBuilder hides it again if there's
		// an active unsolved puzzle to show the interactive sentence builder instead.
		TXT_PopupBody->SetVisibility(ESlateVisibility::Visible);
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
					Chip->OnWordChipClicked.AddUniqueDynamic(this, &ThisClass::HandleWordChipClicked);
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
					if (UWrapBoxSlot* EmptySlot = Cast<UWrapBoxSlot>(WB_PuzzleWords->AddChild(EmptyCell)))
					{
						EmptySlot->SetVerticalAlignment(VAlign_Center);
					}
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
					Chip->OnWordChipClicked.AddUniqueDynamic(this, &ThisClass::HandleWordChipClicked);
					if (UWrapBoxSlot* ChipSlot = Cast<UWrapBoxSlot>(WB_PuzzleWords->AddChild(Chip)))
					{
						ChipSlot->SetVerticalAlignment(VAlign_Center);
					}
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
	ActiveSentenceSegments.Reset();

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
	// 24 matches WBP_CapturePhoto's AnalysisSentenceFontSize so the puzzle text in the tablet
	// looks the same size as the sentence shown on the captured photo card.
	const int32 SegmentFontSize = bStatementStyle ? StatementFontSize : 20;
	int32 BlankSlotIndex = 0;

	// WB_SentenceBuilder itself stays the Designer-authored UWrapBox (so no WBP regen is needed);
	// it hosts a single full-width child, this UVerticalBox, with one UHorizontalBox row per
	// authored line (SentenceTemplate's "\n" boundaries -- no "\n" at all means the whole template
	// is one line). Each line's VerticalBoxSlot is HAlign_Center, so every line centers
	// independently within the full sentence area, matching how a single centered UTextBlock
	// (e.g. TXT_PopupBody) looks -- a plain UWrapBox has no such per-line alignment concept, which
	// is why the sentence used to always hug the left edge.
	//
	// Every SentenceTemplate (Photo and Statement alike) is expected to have "\n" placed by hand at
	// every intended line break; a line never auto-wraps on its own, so a UHorizontalBox (which
	// just lays its children out in one row, however wide that ends up being) is enough -- no need
	// for a UWrapBox's width tracking/auto-wrap machinery here.
	//
	// Read the real authored width off WB_SentenceBuilder's own Canvas slot instead of hardcoding
	// it: the Designer copy of this box has already drifted from what TabletWidgetBlueprintLibrary.cpp
	// generates (e.g. the photo variant is 896px wide there, not the 560px the generator script
	// says), so a literal here would silently center against the wrong width again the next time
	// someone resizes the box by hand.
	float SentenceAreaWidth = bStatementStyle ? 350.0f : 560.0f;
	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WB_SentenceBuilder->Slot))
	{
		SentenceAreaWidth = CanvasSlot->GetSize().X;
	}
	// Gaps between pieces within a line, and between separate lines.
	const FMargin LineItemPadding(2.0f, 0.0f);
	const float LineSpacing = bStatementStyle ? 3.0f : 8.0f;
	UVerticalBox* SentenceLines = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBox* CurrentLine = nullptr;
	auto StartNewLine = [this, SentenceLines, &CurrentLine, LineSpacing, bStatementStyle]()
	{
		CurrentLine = WidgetTree->ConstructWidget<UHorizontalBox>();
		if (UVerticalBoxSlot* Slot = SentenceLines->AddChildToVerticalBox(CurrentLine))
		{
			// Statement lines read like a written declaration, so they stay left-aligned; photo
			// analysis lines keep centering like a puzzle caption.
			Slot->SetHorizontalAlignment(bStatementStyle ? HAlign_Left : HAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, LineSpacing));
		}
	};
	auto AddToCurrentLine = [&CurrentLine, &StartNewLine, LineItemPadding](UWidget* Child)
	{
		if (!CurrentLine)
		{
			StartNewLine();
		}
		if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(CurrentLine->AddChild(Child)))
		{
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(LineItemPadding);
		}
	};

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		// The Unicode Line Separator (codepoint 0x2028) and Paragraph Separator (0x2029) are also
		// normalized to a plain newline: a DataTable row's multi-line text field inserts one of
		// these instead of a plain newline when a line break is typed directly into it in the
		// editor, rather than arriving via CSV reimport -- see the matching normalization in
		// BuildRandomFadeCharacters.
		const TCHAR LineSepBuf[2] = { (TCHAR)0x2028, 0 };
		const TCHAR ParaSepBuf[2] = { (TCHAR)0x2029, 0 };
		FString NormalizedSegment = Segments[SegmentIndex]
			.Replace(TEXT("\r\n"), TEXT("\n"))
			.Replace(TEXT("\r"), TEXT("\n"))
			.Replace(LineSepBuf, TEXT("\n"))
			.Replace(ParaSepBuf, TEXT("\n"));
		TArray<FString> Lines;
		NormalizedSegment.ParseIntoArray(Lines, TEXT("\n"), false);
		for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
		{
			if (LineIndex > 0)
			{
				StartNewLine();
			}
			if (!Lines[LineIndex].IsEmpty())
			{
				UTextBlock* SegmentText = WidgetTree->ConstructWidget<UTextBlock>();
				SegmentText->SetText(FText::FromString(Lines[LineIndex]));
				FSlateFontInfo Font = SegmentText->GetFont();
				Font.Size = SegmentFontSize;
				if (bStatementStyle)
				{
					Font.FontObject = StatementFont;
				}
				SegmentText->SetFont(Font);
				SegmentText->SetColorAndOpacity(FSlateColor(
					bStatementStyle ? FLinearColor::Black : FLinearColor::White));
				ActiveSentenceSegments.Add(SegmentText);
				AddToCurrentLine(SegmentText);
			}
		}

		if (SegmentIndex < Segments.Num() - 1 && BlankSlotIndex < SlotCount)
		{
			UBalhwajeomTabletSentenceBlank* Blank = Cast<UBalhwajeomTabletSentenceBlank>(
				UUserWidget::CreateWidgetInstance(*WidgetTree, UBalhwajeomTabletSentenceBlank::StaticClass(), NAME_None));
			if (!Blank)
			{
				continue;
			}
			Blank->Configure(
				BlankSlotIndex,
				bStatementStyle,
				StatementFont,
				StatementFontSize);
			Blank->OnBlankDropped.AddUniqueDynamic(this, &ThisClass::HandleSentenceBlankDropped);
			Blank->OnBlankClicked.AddUniqueDynamic(this, &ThisClass::HandleSentenceBlankClicked);
			ActiveBlanksBySlot.Add(BlankSlotIndex, Blank);
			AddToCurrentLine(Blank);
			++BlankSlotIndex;
		}
	}

	// A UWrapBox only ever gives a child the space that child itself asks for -- HAlign_Fill on
	// the slot does not stretch it out to the box's full width. Force that width with an explicit
	// SizeBox instead, so each line's HAlign_Center below centers against the sentence area's
	// actual width, not whatever width the VerticalBox happens to end up wanting.
	USizeBox* SentenceLinesSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	SentenceLinesSizeBox->SetWidthOverride(SentenceAreaWidth);
	SentenceLinesSizeBox->SetContent(SentenceLines);
	WB_SentenceBuilder->AddChild(SentenceLinesSizeBox);

	const bool bHasBlanks = !ActiveBlanksBySlot.IsEmpty();
	WB_SentenceBuilder->SetVisibility(bHasBlanks ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (TXT_PopupBody && bHasBlanks)
	{
		TXT_PopupBody->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomTabletWidget::ApplyPopupBodyResultStyle(const bool bIsSolvedAnalysisResult)
{
	if (!TXT_PopupBody)
	{
		return;
	}
	// A solved analysis sentence's ResultText reads left-aligned in its own font; every other case
	// (plain natural-language photos, an unsolved template shown briefly before PreparePuzzle
	// hides this in favor of the interactive blanks) keeps the Designer default cached in
	// BindActiveDetailWidgets. Called both when a photo popup opens (OpenPhoto) and the instant an
	// analysis puzzle is solved without closing the popup (ValidateActivePuzzle), so the style
	// doesn't wait for the next time the photo is reopened.
	if (bIsSolvedAnalysisResult && ActiveDetailWidget)
	{
		TXT_PopupBody->SetJustification(ETextJustify::Left);
		FSlateFontInfo Font = TXT_PopupBody->GetFont();
		if (UFont* ConfiguredFont = ActiveDetailWidget->GetAnalysisResultFont())
		{
			Font.FontObject = ConfiguredFont;
		}
		Font.Size = ActiveDetailWidget->GetAnalysisResultFontSize();
		TXT_PopupBody->SetFont(Font);
	}
	else
	{
		TXT_PopupBody->SetJustification(ETextJustify::Center);
		TXT_PopupBody->SetFont(DefaultPopupBodyFont);
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
	for (UTextBlock* Segment : ActiveSentenceSegments)
	{
		if (Segment)
		{
			Segment->SetColorAndOpacity(FSlateColor(TextColor));
		}
	}
	// Blanks are always empty by the time this fires on a wrong guess (see ValidateActivePuzzle),
	// so SetErrorStyle now paints the box itself red instead of hiding it -- a clearly-marked
	// empty slot the player can still see and drop a new keyword into.
	for (const TPair<int32, TObjectPtr<UBalhwajeomTabletSentenceBlank>>& Pair : ActiveBlanksBySlot)
	{
		if (UBalhwajeomTabletSentenceBlank* Blank = Pair.Value)
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
	if (USoundBase* Sound = KeywordDropSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
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

void UBalhwajeomTabletWidget::HandleWordChipClicked(const FName WordID)
{
	if (WordID.IsNone() || ActiveBlanksBySlot.IsEmpty())
	{
		// No active blank puzzle to place it into (e.g. the folder's plain acquired-word list).
		return;
	}

	// Same destination a drag would pick: the lowest-index blank that isn't already filled.
	TArray<int32> SlotIndices;
	ActiveBlanksBySlot.GetKeys(SlotIndices);
	SlotIndices.Sort();
	for (const int32 SlotIndex : SlotIndices)
	{
		const bool bSlotFilled = ActiveSubmission.SubmittedWords.ContainsByPredicate(
			[SlotIndex](const FSubmittedWordSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
		if (!bSlotFilled)
		{
			HandleSentenceBlankDropped(SlotIndex, WordID, INDEX_NONE);
			return;
		}
	}
}

void UBalhwajeomTabletWidget::HandleSentenceBlankClicked(const int32 SlotIndex)
{
	UBalhwajeomTabletSentenceBlank* Blank = ActiveBlanksBySlot.FindRef(SlotIndex);
	if (!Blank || !Blank->IsFilled())
	{
		return;
	}
	if (USoundBase* Sound = KeywordDropSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}

	ActiveSubmission.SubmittedWords.RemoveAll(
		[SlotIndex](const FSubmittedWordSlot& Candidate) { return Candidate.SlotIndex == SlotIndex; });
	Blank->SetEmpty();

	if (TXT_PuzzleFeedback)
	{
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Collapsed);
	}

	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (Investigation && Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) &&
		Sentence.SentenceType == ESentenceType::PhotoAnalysis)
	{
		SetPhotoPuzzleErrorStyle(false);
	}
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
	// A success transition is already mid-flight (see PlayPuzzleSuccessTransition) -- ignore any
	// further drop/submit while the solved sentence is still flashing/converging away, instead of
	// letting a second ValidateSentence call race the one already animating.
	if (PuzzleSuccessStage != EPuzzleSuccessStage::Inactive)
	{
		return;
	}
	UBalhwajeomInvestigationSubsystem* Investigation = GetInvestigationSubsystem();
	FSentenceDefinition Sentence;
	if (!Investigation || !Investigation->GetSentenceDefinition(ActiveSentenceID, Sentence) ||
		(Sentence.SentenceType == ESentenceType::Statement && !bExplicitStatementSubmit)) return;
	FText Result;
	if (Investigation->ValidateSentence(ActiveSentenceID, ActiveSubmission, Result))
	{
		// The candidate word/photo lists disappear immediately; the filled sentence itself stays on
		// screen for PlayPuzzleSuccessTransition's flash/hold/converge stages and TXT_PopupBody
		// doesn't get Result until the reveal stage (see BeginResultRevealStage).
		HidePuzzleWordAndPhotoControls();
		RefreshAcquiredWordsDisplay();
		RefreshFolderContents();
		PlayPuzzleSuccessTransition(
			Result, Sentence.SentenceTemplate, Sentence.SentenceType == ESentenceType::PhotoAnalysis);
		if (Sentence.SentenceType == ESentenceType::Statement)
		{
			OnStatementSolved.Broadcast();
		}
		if (USoundBase* Sound = SentenceCorrectSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Sound);
		}
	}
	else
	{
		if (USoundBase* Sound = SentenceErrorSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Sound);
		}
		if (!TXT_PuzzleFeedback)
		{
			return;
		}
		// A wrong-but-completed evidence photo surfaces its own declaration sentence's ResultText
		// (see ValidateSentence) instead of the generic message.
		TXT_PuzzleFeedback->SetText(
			Result.IsEmpty() ? NSLOCTEXT("Tablet", "WrongEvidence", "잘못된 증거인 것 같다.") : Result);
		TXT_PuzzleFeedback->SetVisibility(ESlateVisibility::Visible);
		if (Sentence.SentenceType == ESentenceType::PhotoAnalysis)
		{
			// Wrong guess: clear the submission and rebuild the blanks from scratch so every
			// slot's box goes back to its authored default rectangle (just clearing the text
			// left a blank stretched to fit whatever long keyword had been dropped into it),
			// then re-apply the red tint to the freshly rebuilt widgets.
			ActiveSubmission.SubmittedWords.Reset();
			BuildSentenceBuilder(Sentence);
			SetPhotoPuzzleErrorStyle(true);
		}
	}
}

void UBalhwajeomTabletWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (PuzzleSuccessStage == EPuzzleSuccessStage::Inactive)
	{
		return;
	}

	PuzzleSuccessStageElapsed += InDeltaTime;
	switch (PuzzleSuccessStage)
	{
	case EPuzzleSuccessStage::Flash:
		if (PuzzleSuccessStageElapsed >= PuzzleSuccessFlashDuration)
		{
			PuzzleSuccessStage = EPuzzleSuccessStage::Hold;
			PuzzleSuccessStageElapsed = 0.0f;
		}
		break;
	case EPuzzleSuccessStage::Hold:
		if (PuzzleSuccessStageElapsed >= PuzzleSuccessHoldDuration)
		{
			BeginPuzzleConvergeStage();
		}
		break;
	case EPuzzleSuccessStage::Converge:
		TickPuzzleConvergeStage();
		if (PuzzleSuccessStageElapsed >= PuzzleSuccessConvergeDuration)
		{
			BeginResultRevealStage();
		}
		break;
	case EPuzzleSuccessStage::Reveal:
		TickResultRevealStage();
		break;
	case EPuzzleSuccessStage::Handoff:
		TickHandoffStage();
		break;
	default:
		break;
	}
}

void UBalhwajeomTabletWidget::PlayPuzzleSuccessTransition(
	const FText& ResultText, const FText& SentenceTemplate, const bool bApplyAnalysisResultStyle)
{
	PendingResultText = ResultText;
	PendingSentenceTemplate = SentenceTemplate;
	bPendingApplyAnalysisResultStyle = bApplyAnalysisResultStyle;
	PuzzleSuccessStage = EPuzzleSuccessStage::Flash;
	PuzzleSuccessStageElapsed = 0.0f;

	// Gold flash for a solved photo-analysis puzzle, blue for a solved statement -- mirrors
	// SetPhotoPuzzleErrorStyle's red wrong-answer tint but for the correct case, and gives the two
	// sentence types visually distinct "correct" cues. Held by NativeTick's Flash/Hold stages until
	// BeginPuzzleConvergeStage explodes these same words into the random-fade character grid (which
	// reads FlashColor back off these same widgets, so it doesn't need to be picked again there).
	static const FLinearColor GoldFlashColor = FLinearColor::FromSRGBColor(FColor(255, 209, 102, 255));
	static const FLinearColor BlueFlashColor = FLinearColor::FromSRGBColor(FColor(102, 178, 255, 255));
	const FLinearColor FlashColor = bApplyAnalysisResultStyle ? GoldFlashColor : BlueFlashColor;
	for (const TObjectPtr<UTextBlock>& Segment : ActiveSentenceSegments)
	{
		if (Segment)
		{
			Segment->SetColorAndOpacity(FSlateColor(FlashColor));
		}
	}
	for (const TPair<int32, TObjectPtr<UBalhwajeomTabletSentenceBlank>>& Pair : ActiveBlanksBySlot)
	{
		if (UBalhwajeomTabletSentenceBlank* Blank = Pair.Value)
		{
			Blank->SetSuccessStyle(true, FlashColor);
		}
	}
}

void UBalhwajeomTabletWidget::BeginPuzzleConvergeStage()
{
	PuzzleSuccessStage = EPuzzleSuccessStage::Converge;
	PuzzleSuccessStageElapsed = 0.0f;

	// Copy font/color off whatever's still on screen (a plain segment if there is one, else the
	// first blank) so the character grid this builds reads as a seamless continuation of the flash
	// rather than a style change.
	FSlateFontInfo CharFont;
	FSlateColor CharColor = FSlateColor(FLinearColor::White);
	if (!ActiveSentenceSegments.IsEmpty() && ActiveSentenceSegments[0])
	{
		CharFont = ActiveSentenceSegments[0]->GetFont();
		CharColor = ActiveSentenceSegments[0]->GetColorAndOpacity();
	}
	else
	{
		for (const TPair<int32, TObjectPtr<UBalhwajeomTabletSentenceBlank>>& Pair : ActiveBlanksBySlot)
		{
			if (Pair.Value)
			{
				CharFont = Pair.Value->GetDisplayFont();
				CharColor = Pair.Value->GetDisplayColor();
				break;
			}
		}
	}

	// Read while ActiveBlanksBySlot/ActiveSentenceSegments are still populated; BuildRandomFadeCharacters
	// clears them (via WB_SentenceBuilder->ClearChildren) as its first step.
	const FString FlatText = BuildFlatSolvedSentenceText(PendingSentenceTemplate);
	// Matches BuildSentenceBuilder's own bStatementStyle ? HAlign_Left : HAlign_Center, so the grid
	// this rebuilds starts out aligned exactly like the puzzle text it's replacing.
	const bool bLeftAligned = !bPendingApplyAnalysisResultStyle;
	// Matches BuildSentenceBuilder's own LineSpacing (const float LineSpacing = bStatementStyle ?
	// 3.0f : 8.0f;) -- without it, a multi-line sentence's lines would visibly tighten up the moment
	// this rebuilds the puzzle text into the random-fade grid.
	const float LinePadding = bLeftAligned ? 3.0f : 8.0f;
	BuildRandomFadeCharacters(
		FlatText,
		CharFont,
		CharColor,
		/*bStartVisible=*/true,
		PuzzleSuccessConvergeDuration,
		bLeftAligned,
		LinePadding);
}

void UBalhwajeomTabletWidget::TickPuzzleConvergeStage()
{
	TickRandomFadeChars(/*bFadeIn=*/false);
}

void UBalhwajeomTabletWidget::BeginResultRevealStage()
{
	// Discards the Converge stage's per-character widgets (dead by now -- Converge already faded
	// them all to 0).
	ClearSentenceBuilder();
	RandomFadeChars.Reset();

	if (bPendingApplyAnalysisResultStyle)
	{
		// Sets Left justification plus the photo-analysis-specific result font/size.
		ApplyPopupBodyResultStyle(true);
	}
	else if (TXT_PopupBody)
	{
		// A solved statement reads left-aligned too (like a written declaration), but keeps its own
		// statement font from ShowPopup -- ApplyPopupBodyResultStyle(true) would swap in the
		// photo-analysis font/size instead, which isn't right for this popup.
		TXT_PopupBody->SetJustification(ETextJustify::Left);
	}

	PuzzleSuccessStage = EPuzzleSuccessStage::Reveal;
	PuzzleSuccessStageElapsed = 0.0f;

	// TXT_PopupBody's font/color are already correct here (ApplyPopupBodyResultStyle just above for
	// a photo-analysis result, or whatever ShowPopup set for a statement popup, now left-justified
	// either way) -- copy them onto the character grid so it matches exactly, then hand off to
	// TXT_PopupBody itself once every character has finished fading in (see TickResultRevealStage).
	FSlateFontInfo CharFont;
	FSlateColor CharColor = FSlateColor(FLinearColor::Black);
	constexpr bool bLeftAligned = true;
	if (TXT_PopupBody)
	{
		CharFont = TXT_PopupBody->GetFont();
		CharColor = TXT_PopupBody->GetColorAndOpacity();
		TXT_PopupBody->SetVisibility(ESlateVisibility::Collapsed);
	}
	BuildRandomFadeCharacters(
		PendingResultText.ToString(),
		CharFont,
		CharColor,
		/*bStartVisible=*/false,
		PuzzleSuccessRevealFadeDuration,
		bLeftAligned,
		// 0 here (unlike Converge's LinePadding): this grid hands off to TXT_PopupBody, which spaces
		// its own lines by font metrics alone, not BuildSentenceBuilder's puzzle-specific LineSpacing.
		/*LinePadding=*/0.0f);
}

void UBalhwajeomTabletWidget::TickResultRevealStage()
{
	TickRandomFadeChars(/*bFadeIn=*/true);
	if (PuzzleSuccessStageElapsed >= PuzzleSuccessRevealFadeDuration)
	{
		BeginHandoffStage();
	}
}

void UBalhwajeomTabletWidget::BeginHandoffStage()
{
	PuzzleSuccessStage = EPuzzleSuccessStage::Handoff;
	PuzzleSuccessStageElapsed = 0.0f;

	// TXT_PopupBody starts invisible (opacity 0) and fades in while the character grid
	// (WB_SentenceBuilder, still fully opaque from Reveal) fades out over it -- see
	// TickHandoffStage -- instead of the grid being deleted and TXT_PopupBody snapping to full
	// opacity in the same frame, which read as the finished sentence being "dropped into place."
	if (TXT_PopupBody)
	{
		TXT_PopupBody->SetText(PendingResultText);
		TXT_PopupBody->SetRenderOpacity(0.0f);
		TXT_PopupBody->SetVisibility(ESlateVisibility::Visible);
	}
}

void UBalhwajeomTabletWidget::TickHandoffStage()
{
	const float Alpha =
		FMath::Clamp(PuzzleSuccessStageElapsed / PuzzleSuccessHandoffDuration, 0.0f, 1.0f);
	if (WB_SentenceBuilder)
	{
		WB_SentenceBuilder->SetRenderOpacity(1.0f - Alpha);
	}
	if (TXT_PopupBody)
	{
		TXT_PopupBody->SetRenderOpacity(Alpha);
	}

	if (Alpha >= 1.0f)
	{
		// Every character has finished fading in and handed off -- swap to the plain TXT_PopupBody
		// the rest of the class expects to hold the final text (reopening this popup, a
		// wrong-then-right retry, etc. all just read TXT_PopupBody, not the transient character
		// grid). WB_SentenceBuilder's own opacity is reset here since ClearSentenceBuilder only
		// clears its children/visibility, and this same UWrapBox gets reused by the next puzzle.
		ClearSentenceBuilder();
		RandomFadeChars.Reset();
		if (WB_SentenceBuilder)
		{
			WB_SentenceBuilder->SetRenderOpacity(1.0f);
		}
		if (TXT_PopupBody)
		{
			TXT_PopupBody->SetRenderOpacity(1.0f);
		}
		PuzzleSuccessStage = EPuzzleSuccessStage::Inactive;
	}
}

FString UBalhwajeomTabletWidget::BuildFlatSolvedSentenceText(const FText& SentenceTemplate) const
{
	TArray<FString> Segments;
	SentenceTemplate.ToString().ParseIntoArray(Segments, TEXT("[]"), false);

	FString Flat;
	int32 BlankSlotIndex = 0;
	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		// "\r\n" normalized to "\n" but otherwise kept as a real line break -- BuildRandomFadeCharacters
		// splits on it, so the sentence's own authored line breaks survive into the random-fade grid
		// instead of being reflowed into a single line.
		Flat += Segments[SegmentIndex].Replace(TEXT("\r\n"), TEXT("\n"));
		if (SegmentIndex < Segments.Num() - 1)
		{
			if (const TObjectPtr<UBalhwajeomTabletSentenceBlank>* Blank =
					ActiveBlanksBySlot.Find(BlankSlotIndex))
			{
				if (*Blank)
				{
					// Bracketed so BuildRandomFadeCharacters (via ParseTextRuns) can reserve this
					// word's blank's own minimum width instead of letting it shrink to bare text
					// width -- see PuzzleBlankMinWidth's comment.
					Flat += BlankWordRunStart;
					Flat += (*Blank)->GetDisplayText().ToString();
					Flat += BlankWordRunEnd;
				}
			}
			++BlankSlotIndex;
		}
	}
	return Flat;
}

void UBalhwajeomTabletWidget::BuildRandomFadeCharacters(
	const FString& Text,
	const FSlateFontInfo& Font,
	const FSlateColor& Color,
	const bool bStartVisible,
	const float TotalWindow,
	const bool bLeftAligned,
	const float LinePadding)
{
	RandomFadeChars.Reset();
	if (!WB_SentenceBuilder || !WidgetTree)
	{
		return;
	}
	WB_SentenceBuilder->ClearChildren();
	if (Text.IsEmpty())
	{
		return;
	}
	WB_SentenceBuilder->SetVisibility(ESlateVisibility::Visible);

	const float MaxStartDelay = FMath::Max(0.0f, TotalWindow - PuzzleSuccessCharFadeDuration);
	const EHorizontalAlignment LineAlignment = bLeftAligned ? HAlign_Left : HAlign_Center;

	// Preserves the sentence's own authored newlines -- BuildSentenceBuilder's convention is that
	// every intended line break is placed by hand and a line never auto-wraps on its own -- so each
	// line becomes its own non-wrapping UHorizontalBox of one-grapheme UTextBlocks (spaces included
	// as their own blank-width grapheme, so word gaps need no extra handling), stacked in a
	// UVerticalBox. Split via SplitIntoGraphemes, not FString::Mid(i, 1): the latter slices raw
	// UTF-16 code units, which breaks a decomposed (NFD) Hangul syllable's Jamo apart into separate
	// widgets that can no longer compose into one letter (see SplitIntoGraphemes's comment).
	//
	// Also normalizes "\r\n" and the Unicode Line/Paragraph Separator characters (codepoints 0x2028
	// and 0x2029) to a plain newline before splitting: BuildFlatSolvedSentenceText already does the
	// "\r\n" half for the Converge path, but BeginResultRevealStage passes
	// PendingResultText.ToString() straight through, and a DataTable row's multi-line text field
	// inserts a Line/Paragraph Separator (not "\n") when a line break is typed directly into it in
	// the editor rather than arriving via CSV reimport -- so either could otherwise leave a line
	// break this function's own line-splitting does not recognize.
	const TCHAR LineSepBuf[2] = { (TCHAR)0x2028, 0 };
	const TCHAR ParaSepBuf[2] = { (TCHAR)0x2029, 0 };
	TArray<FString> Lines;
	Text.Replace(TEXT("\r\n"), TEXT("\n"))
		.Replace(LineSepBuf, TEXT("\n"))
		.Replace(ParaSepBuf, TEXT("\n"))
		.ParseIntoArray(Lines, TEXT("\n"), false);

	// Builds one grapheme's UTextBlock into TargetBox and registers it for TickRandomFadeChars.
	auto AddGrapheme = [this, &Font, &Color, bStartVisible, MaxStartDelay](
						   UHorizontalBox* TargetBox, const FString& Grapheme)
	{
		UTextBlock* CharText = WidgetTree->ConstructWidget<UTextBlock>();
		CharText->SetText(FText::FromString(Grapheme));
		CharText->SetFont(Font);
		CharText->SetColorAndOpacity(Color);
		// UTextBlock defaults to a (1,1) drop shadow; every other text piece in this file clears it
		// (see e.g. ArrowText/TitleText below), and TXT_PopupBody itself has none -- without this,
		// each character reads with a faint shadow during the reveal that vanishes the instant
		// TickResultRevealStage hands off to the real (shadow-less) TXT_PopupBody.
		CharText->SetShadowOffset(FVector2D::ZeroVector);
		CharText->SetRenderOpacity(bStartVisible ? 1.0f : 0.0f);
		if (UHorizontalBoxSlot* CharSlot = TargetBox->AddChildToHorizontalBox(CharText))
		{
			CharSlot->SetVerticalAlignment(VAlign_Center);
			// FSlateChildSize's default constructor is Fill, not Auto -- AddChildToHorizontalBox
			// slots come out Fill by default. Left alone, a blank word's WordSizeBox (below) forcing
			// extra width onto this character's row divides that space evenly across every Fill
			// character slot, visibly spreading the letters apart instead of leaving them touching
			// with the slack as trailing empty space.
			CharSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		FRandomFadeChar Entry;
		Entry.TextBlock = CharText;
		Entry.StartDelay = FMath::FRandRange(0.0f, MaxStartDelay);
		RandomFadeChars.Add(Entry);
	};

	UVerticalBox* LinesBox = WidgetTree->ConstructWidget<UVerticalBox>();
	for (const FString& Line : Lines)
	{
		UHorizontalBox* LineBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		// A blank authored line (paragraph gap) has no runs to iterate below, which would leave
		// LineBox with zero children -- and a childless UHorizontalBox has zero height, collapsing
		// the gap away entirely instead of reserving a normal line's worth of space the way
		// TXT_PopupBody's native rendering of the same "\n\n" does. A single invisible space
		// character gives it the font's ordinary line height without showing anything.
		const TArray<FTextRun> Runs =
			Line.IsEmpty() ? TArray<FTextRun>{FTextRun{TEXT(" "), false}} : ParseTextRuns(Line);
		for (const FTextRun& Run : Runs)
		{
			// Every run (plain text or a former blank's word) is built into its own box first --
			// graphemes inside it touch as normal text should, but the run itself is what gets
			// spaced from its neighbors below, matching BuildSentenceBuilder's LineItemPadding
			// (the gap it puts between every segment/blank on a line).
			UHorizontalBox* RunBox = WidgetTree->ConstructWidget<UHorizontalBox>();
			for (const FString& Grapheme : SplitIntoGraphemes(Run.Text))
			{
				AddGrapheme(RunBox, Grapheme);
			}

			UWidget* RunWidget = RunBox;
			// UBalhwajeomTabletSentenceBlank::Configure only wraps a photo-analysis blank
			// (bStatementStyle == false, i.e. bLeftAligned == false here -- see BeginPuzzleConvergeStage's
			// bLeftAligned = !bPendingApplyAnalysisResultStyle) in a 60px-minimum SizeBox; a statement
			// blank's root widget is just Background, no minimum width at all. Reserving 60px for every
			// blank word regardless of type was inflating (not preserving) a statement's naturally
			// narrower words the instant this grid replaced them, pushing everything after them further
			// right than the original blank ever did.
			if (Run.bIsBlankWord && !bLeftAligned)
			{
				// Reserve the same minimum width the blank's own box used to enforce (see
				// PuzzleBlankMinWidth's comment) so the surrounding text doesn't slide over to meet
				// it once the blank's box disappears.
				USizeBox* WordSizeBox = WidgetTree->ConstructWidget<USizeBox>();
				WordSizeBox->SetMinDesiredWidth(PuzzleBlankMinWidth);
				WordSizeBox->SetContent(RunBox);
				RunWidget = WordSizeBox;
			}
			if (UHorizontalBoxSlot* RunSlot = LineBox->AddChildToHorizontalBox(RunWidget))
			{
				RunSlot->SetVerticalAlignment(VAlign_Center);
				RunSlot->SetPadding(FMargin(2.0f, 0.0f));
				// Same Fill-by-default gotcha as CharSlot above -- keep every run at its own natural
				// width instead of letting it stretch to share out whatever's left of LineBox.
				RunSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}
		}
		if (UVerticalBoxSlot* LineSlot = LinesBox->AddChildToVerticalBox(LineBox))
		{
			LineSlot->SetHorizontalAlignment(LineAlignment);
			LineSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, LinePadding));
		}
	}

	// Matches BuildSentenceBuilder's own SentenceAreaWidth: without pinning LinesBox to the same
	// fixed width the puzzle text was laid out in, its width would instead shrink to fit this
	// particular piece of text, and every line's HAlign above would center/left-align against that
	// different width -- visible as the whole block suddenly jumping sideways the moment this
	// rebuilds WB_SentenceBuilder's children (Converge exploding the solved puzzle text, then Reveal
	// swapping in ResultText).
	float SentenceAreaWidth = bLeftAligned ? 350.0f : 560.0f;
	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WB_SentenceBuilder->Slot))
	{
		SentenceAreaWidth = CanvasSlot->GetSize().X;
	}
	USizeBox* LinesSizeBox = WidgetTree->ConstructWidget<USizeBox>();
	LinesSizeBox->SetWidthOverride(SentenceAreaWidth);
	LinesSizeBox->SetContent(LinesBox);
	WB_SentenceBuilder->AddChild(LinesSizeBox);
}

void UBalhwajeomTabletWidget::TickRandomFadeChars(const bool bFadeIn)
{
	for (const FRandomFadeChar& Entry : RandomFadeChars)
	{
		UTextBlock* CharText = Entry.TextBlock.Get();
		if (!CharText)
		{
			continue;
		}
		const float LocalAlpha = FMath::Clamp(
			(PuzzleSuccessStageElapsed - Entry.StartDelay) / PuzzleSuccessCharFadeDuration, 0.0f, 1.0f);
		CharText->SetRenderOpacity(bFadeIn ? LocalAlpha : (1.0f - LocalAlpha));
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
	if (TXT_PopupBody)
	{
		// Cache this fresh instance's Designer-authored font before OpenPhoto (or anything else)
		// can override it for a solved analysis result, so every other case can be restored to it.
		DefaultPopupBodyFont = TXT_PopupBody->GetFont();
	}
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
	// Cancel any solved-puzzle transition still flashing/fading from whatever was open before --
	// leaving PuzzleSuccessStage running would keep writing PendingResultText into this new popup's
	// TXT_PopupBody once Reveal catches up. Any half-built RandomFadeChars widgets are cleaned up by
	// HidePuzzleControls -> ClearSentenceBuilder just below (WB_SentenceBuilder->ClearChildren()).
	PuzzleSuccessStage = EPuzzleSuccessStage::Inactive;
	RandomFadeChars.Reset();
	if (TXT_PopupBody)
	{
		// In case a Reveal/Handoff was interrupted mid-fade (see TickResultRevealStage/
		// TickHandoffStage); otherwise this new popup's own text would start out partially
		// transparent.
		TXT_PopupBody->SetRenderOpacity(1.0f);
	}
	if (WB_SentenceBuilder)
	{
		// In case a Handoff was interrupted mid-crossfade; HidePuzzleControls below clears its
		// children/visibility but not this, and it's the same UWrapBox the next puzzle reuses.
		WB_SentenceBuilder->SetRenderOpacity(1.0f);
	}
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
				Chip->OnWordChipClicked.AddUniqueDynamic(this, &ThisClass::HandleWordChipClicked);
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
		Chip->OnWordChipClicked.AddUniqueDynamic(this, &ThisClass::HandleWordChipClicked);
		if (UWrapBoxSlot* ChipSlot = Cast<UWrapBoxSlot>(WB_PuzzleWords->AddChild(Chip)))
		{
			ChipSlot->SetVerticalAlignment(VAlign_Center);
		}
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
	// See the matching comment in ShowPopup: don't let a solved puzzle's transition keep running
	// (and eventually writing into TXT_PopupBody) after the popup itself has been closed.
	PuzzleSuccessStage = EPuzzleSuccessStage::Inactive;
	RandomFadeChars.Reset();
	if (TXT_PopupBody)
	{
		TXT_PopupBody->SetRenderOpacity(1.0f);
	}
	if (WB_SentenceBuilder)
	{
		WB_SentenceBuilder->SetRenderOpacity(1.0f);
	}
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
	VB_FileLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_FileLayout"));
	SB_Thumbnail = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SB_Thumbnail"));
	SB_Thumbnail->SetWidthOverride(ThumbnailWidth);
	SB_Thumbnail->SetHeightOverride(ThumbnailHeight);
	IMG_Thumbnail = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IMG_Thumbnail"));
	SB_Thumbnail->AddChild(IMG_Thumbnail);
	VB_FileLayout->AddChildToVerticalBox(SB_Thumbnail);
	TXT_Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TXT_Label"));
	VB_FileLayout->AddChildToVerticalBox(TXT_Label);
	BTN_File->SetContent(VB_FileLayout);
	WidgetTree->RootWidget = BTN_File;
	BTN_File->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClicked);
}

void UBalhwajeomTabletPhotoButton::SetThumbnailSizeOverride(const float InWidth, const float InHeight)
{
	bHasThumbnailSizeOverride = true;
	ThumbnailWidthOverride = InWidth;
	ThumbnailHeightOverride = InHeight;
	if (SB_Thumbnail)
	{
		SB_Thumbnail->SetWidthOverride(ThumbnailWidthOverride);
		SB_Thumbnail->SetHeightOverride(ThumbnailHeightOverride);
	}
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
	if (VB_FileLayout)
	{
		// Bottom-anchor the thumbnail+label stack within BTN_File instead of the default top
		// anchor, so the label lines up across tiles whose outer box height differs (e.g. the
		// folder's taller statement tile next to regular photo tiles bottom-aligned in the same
		// wrap row -- see UBalhwajeomTabletFolderSection::AddTile).
		if (UButtonSlot* LayoutSlot = Cast<UButtonSlot>(VB_FileLayout->Slot))
		{
			LayoutSlot->SetVerticalAlignment(VAlign_Bottom);
		}
	}
	if (TXT_Label)
	{
		FSlateFontInfo LabelFontInfo = TXT_Label->GetFont();
		if (LabelFont)
		{
			LabelFontInfo.FontObject = LabelFont;
		}
		LabelFontInfo.Size = LabelFontSize;

		// Center-justified by design, so the built-in Ellipsis overflow policy can't be used (see
		// TruncateLabelToFit) -- pre-shorten the string ourselves against LabelWidth instead.
		TXT_Label->SetText(TruncateLabelToFit(InLabel, LabelFontInfo, LabelWidth));
		TXT_Label->SetToolTipText(InLabel);
		TXT_Label->SetJustification(ETextJustify::Center);
		TXT_Label->SetMinDesiredWidth(0.0f);
		TXT_Label->SetAutoWrapText(false);
		TXT_Label->SetClipping(EWidgetClipping::ClipToBounds);
		TXT_Label->SetFont(LabelFontInfo);
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
		// Applied here (not just in BuildFallbackVisuals) so it still takes effect when the WBP
		// provides its own Designer-authored SB_Thumbnail instead of the C++ fallback layout.
		// SetThumbnailSizeOverride (called before Configure for the folder's statement tile)
		// takes priority over the shared Class Default.
		SB_Thumbnail->SetWidthOverride(bHasThumbnailSizeOverride ? ThumbnailWidthOverride : ThumbnailWidth);
		SB_Thumbnail->SetHeightOverride(bHasThumbnailSizeOverride ? ThumbnailHeightOverride : ThumbnailHeight);
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
		// Single line, truncated with "..." like a real folder's filename label, instead of
		// wrapping or overflowing once a long CharacterID.FolderName is used.
		TXT_FolderLabel->SetAutoWrapText(false);
		TXT_FolderLabel->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
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
	// UTextBlock defaults to a (1,1) drop shadow; every other text piece in this file clears it
	// explicitly, but this one was missed.
	ArrowText->SetShadowOffset(FVector2D::ZeroVector);
	UHorizontalBoxSlot* ArrowSlot = HeaderRow->AddChildToHorizontalBox(ArrowText);
	ArrowSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	ArrowSlot->SetVerticalAlignment(VAlign_Center);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	// Font/size applied in Configure() instead (also covers a Designer-authored TitleText).
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.86f, 0.78f, 1.0f)));
	TitleText->SetShadowOffset(FVector2D::ZeroVector);
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

	if (TitleText)
	{
		// Applied here (not just in BuildFallbackVisuals) so it still takes effect when the WBP
		// provides its own Designer-authored TitleText instead of the C++ fallback layout. Same
		// reasoning for the shadow below -- a Designer TitleText keeps whatever Shadow Offset it
		// was authored with unless this clears it every time too.
		FSlateFontInfo Font = TitleText->GetFont();
		if (TitleFont)
		{
			Font.FontObject = TitleFont;
		}
		Font.Size = TitleFontSize;
		TitleText->SetFont(Font);
		TitleText->SetShadowOffset(FVector2D::ZeroVector);
	}
	if (ArrowText)
	{
		ArrowText->SetShadowOffset(FVector2D::ZeroVector);
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
	// The statement tile (144x100) and regular photo/memory tiles (144x81) sit in the same wrap
	// row with different heights; bottom-align every tile so they share one baseline instead of
	// the taller statement tile hanging lower than its row-mates.
	if (UWrapBoxSlot* TileSlot = Cast<UWrapBoxSlot>(ContentWrapBox->AddChild(Tile)))
	{
		TileSlot->SetVerticalAlignment(VAlign_Bottom);
	}
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
	Background->SetPadding(bStatementStyle ? FMargin(6.0f, 11.0f, 5.0f, 3.0f) : FMargin(10.0f, 6.0f));
	if (bStatementStyle)
	{
		ApplyKeywordHoverBrush(Background);
		Background->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	}
	else
	{
		// Captured so NativeOnMouseLeave can undo ApplyKeywordHoverBrush's brush swap.
		NormalBrush = Background->Background;
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
	if (!Background || !LabelText)
	{
		return;
	}
	if (bStatementStyle)
	{
		Background->SetBrushColor(FLinearColor::White);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	}
	else
	{
		ApplyKeywordHoverBrush(Background);
		Background->SetBrushColor(FLinearColor::White);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	}
}

void UBalhwajeomTabletWordChip::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (!Background || !LabelText)
	{
		return;
	}
	if (bStatementStyle)
	{
		Background->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	else
	{
		Background->SetBrush(NormalBrush);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.91f, 0.82f, 1.0f)));
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

FReply UBalhwajeomTabletWordChip::NativeOnMouseButtonUp(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Only reached when NativeOnDragDetected never fired (a plain click, mouse never moved past the
	// drag threshold) -- an actual drag's mouse-up is consumed by the drop target instead.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnWordChipClicked.Broadcast(WordID);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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
		DragVisual->SetPadding(bStatementStyle ? FMargin(6.0f, 9.0f, 5.0f, 3.0f) : FMargin(10.0f, 6.0f));
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
	// 24 matches WBP_CapturePhoto's AnalysisSentenceFontSize (see BuildSentenceBuilder's
	// SegmentFontSize) so a blank's filled keyword reads at the same size as its surrounding text.
	Font.Size = bStatementStyle ? InStatementFontSize : 20;
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
		// horizontally at the same size as the surrounding photo sentence. Narrowed from 83 so
		// short (e.g. particle-only) blanks don't look oversized next to their filled neighbors.
		BlankSize->SetMinDesiredWidth(60.0f);
		BlankSize->SetMinDesiredHeight(30.0f);
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

	// An error blank now always sits empty (see ValidateActivePuzzle), so the box itself is
	// tinted red to mark it as a wrong-then-cleared slot -- a transparent background would just
	// make the empty drop target disappear.
	Background->SetBrushColor(
		bErrorStyle
			? FLinearColor(0.761f, 0.471f, 0.471f, 1.0f)
			: FLinearColor::White);
	if (DisplayText)
	{
		DisplayText->SetColorAndOpacity(FSlateColor(
			bErrorStyle || FilledWordID.IsNone() ? FLinearColor::White : FLinearColor::Black));
	}
}

void UBalhwajeomTabletSentenceBlank::SetSuccessStyle(const bool bInSuccess, const FLinearColor& FlashColor)
{
	// FlashColor matches whatever UBalhwajeomTabletWidget::PlayPuzzleSuccessTransition picked for the
	// surrounding sentence's segments (gold for a solved photo-analysis puzzle, blue for a solved
	// statement).
	// SetFilled always paints a filled blank's text black regardless of style, so that's the color
	// to fall back to once the flash ends (this blank is destroyed by ClearSentenceBuilder shortly
	// after anyway, but keeping the two in sync avoids a stray flash-colored frame if that ever
	// changes).
	if (DisplayText)
	{
		DisplayText->SetColorAndOpacity(FSlateColor(bInSuccess ? FlashColor : FLinearColor::Black));
	}
	if (Background)
	{
		// Drop the blank's own box the instant it flashes, so only the glowing word is left --
		// otherwise the box (white for a photo blank, translucent pink for a statement blank; see
		// Configure) would still be visible fading out underneath the text.
		Background->SetBrushColor(bInSuccess
			? FLinearColor(1.0f, 1.0f, 1.0f, 0.0f)
			: (bStatementStyle ? FLinearColor(1.0f, 0.72f, 0.72f, 0.72f) : FLinearColor::White));
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

FText UBalhwajeomTabletSentenceBlank::GetDisplayText() const
{
	return DisplayText ? DisplayText->GetText() : FText::GetEmpty();
}

FSlateFontInfo UBalhwajeomTabletSentenceBlank::GetDisplayFont() const
{
	return DisplayText ? DisplayText->GetFont() : FSlateFontInfo();
}

FSlateColor UBalhwajeomTabletSentenceBlank::GetDisplayColor() const
{
	return DisplayText ? DisplayText->GetColorAndOpacity() : FSlateColor(FLinearColor::White);
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

FReply UBalhwajeomTabletSentenceBlank::NativeOnMouseButtonUp(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Only reached when NativeOnDragDetected never fired (a plain click) -- an actual drag's
	// mouse-up is consumed by whichever blank it gets dropped on instead.
	if (!FilledWordID.IsNone() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnBlankClicked.Broadcast(SlotIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
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
