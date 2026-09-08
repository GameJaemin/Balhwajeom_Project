#include "Tablet/TabletWidgetBlueprintLibrary.h"

#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WrapBox.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Factories/DataAssetFactory.h"
#include "Factories/DataTableFactory.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Tablet/BalhwajeomMessengerDataAssets.h"
#include "Tablet/BalhwajeomMessengerDateSeparator.h"
#include "Tablet/BalhwajeomMessengerKeywordWidget.h"
#include "Tablet/BalhwajeomMessengerMessageWidget.h"
#include "Tablet/BalhwajeomMessengerRoomWidget.h"
#include "Tablet/BalhwajeomMessengerWidget.h"
#include "Investigation/WordDefinitions.h"
#include "Investigation/CharacterDefinitions.h"
#include "Investigation/EvidenceDefinitions.h"
#include "Investigation/PhotoDefinitions.h"
#include "Investigation/SentenceDefinitions.h"
#include "Tablet/BalhwajeomTabletWidget.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace TabletDesigner
{
	const TCHAR* AssetFolder = TEXT("/Game/Balhwajeom/UI/Tablet");
	const TCHAR* AssetName = TEXT("WBP_Tablet");
	const TCHAR* AssetPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_Tablet.WBP_Tablet");
	const TCHAR* MessengerAssetName = TEXT("WBP_Messenger");
	const TCHAR* MessengerAssetPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_Messenger.WBP_Messenger");
	const TCHAR* MessengerClassPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_Messenger.WBP_Messenger_C");
	const TCHAR* RoomAssetName = TEXT("WBP_MessengerRoom");
	const TCHAR* RoomAssetPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerRoom.WBP_MessengerRoom");
	const TCHAR* MessageAssetName = TEXT("WBP_MessengerMessage");
	const TCHAR* MessageAssetPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerMessage.WBP_MessengerMessage");
	const TCHAR* KeywordAssetName = TEXT("WBP_MessengerKeyword");
	const TCHAR* KeywordAssetPath = TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerKeyword.WBP_MessengerKeyword");
	const TCHAR* MessengerDataFolder = TEXT("/Game/Balhwajeom/Data/Messenger");
	const TCHAR* MessengerRoomDataFolder = TEXT("/Game/Balhwajeom/Data/Messenger/Rooms");
	const TCHAR* MessengerCatalogAssetName = TEXT("DA_MessengerCatalog");
	const TCHAR* MessengerCatalogAssetPath =
		TEXT("/Game/Balhwajeom/Data/Messenger/DA_MessengerCatalog.DA_MessengerCatalog");

	const TCHAR* TabletBodyPath = TEXT("/Game/Balhwajeom/UI/Tablet/Tablet_Body.Tablet_Body");
	const TCHAR* FamilyPath = TEXT("/Game/Balhwajeom/UI/Tablet/Family.Family");
	const TCHAR* SisterPath = TEXT("/Game/Balhwajeom/UI/Tablet/Folder_sister.Folder_sister");
	const TCHAR* BrotherPath = TEXT("/Game/Balhwajeom/UI/Tablet/Folder_Bro.Folder_Bro");
	const TCHAR* MotherPath = TEXT("/Game/Balhwajeom/UI/Tablet/Folder_Mother.Folder_Mother");
	const TCHAR* MessengerPath = TEXT("/Game/Balhwajeom/UI/Tablet/App_Messanger.App_Messanger");
	const TCHAR* InternetPath = TEXT("/Game/Balhwajeom/UI/Tablet/App_Internet.App_Internet");
	const TCHAR* MemoPath = TEXT("/Game/Balhwajeom/UI/Tablet/App_Note.App_Note");
	const TCHAR* StickyPath = TEXT("/Game/Balhwajeom/UI/Tablet/stickey.stickey");

	const FLinearColor WarmWhite(0.96f, 0.91f, 0.82f, 1.0f);
	const FLinearColor WarmMuted(0.78f, 0.70f, 0.60f, 1.0f);
	const FLinearColor WarmDark(0.15f, 0.10f, 0.065f, 1.0f);
	const FLinearColor PageBackground(0.12f, 0.085f, 0.055f, 0.96f);
	const FLinearColor PagePanel(0.22f, 0.15f, 0.09f, 0.92f);
	const FLinearColor RedBadge(0.78f, 0.08f, 0.06f, 1.0f);
	const FLinearColor MessengerAccent(0.91f, 0.60f, 0.18f, 1.0f);

	bool SaveAndCompile(UWidgetBlueprint* Blueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);

		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		const FString PackageFilename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.bSlowTask = false;
		return UPackage::SavePackage(Package, Blueprint, *PackageFilename, SaveArgs);
	}

	bool SaveDataAsset(UDataAsset* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		const FString PackageFilename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.bSlowTask = false;
		return UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
	}

	template <typename T>
	T* LoadOrCreateDataAsset(
		const TCHAR* InAssetName,
		const TCHAR* InAssetFolder,
		bool& bOutCreated)
	{
		const FString DataAssetPath = FString::Printf(
			TEXT("%s/%s.%s"),
			InAssetFolder,
			InAssetName,
			InAssetName);
		if (T* Existing = LoadObject<T>(nullptr, *DataAssetPath))
		{
			bOutCreated = false;
			return Existing;
		}

		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = T::StaticClass();
		IAssetTools& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		T* Created = Cast<T>(AssetTools.CreateAsset(
			InAssetName,
			InAssetFolder,
			T::StaticClass(),
			Factory));
		bOutCreated = Created != nullptr;
		return Created;
	}

	FST_MessengerMessage MakeArchivedMessage(
		const TCHAR* Sender,
		const TCHAR* Message,
		const bool bIsPlayer,
		const TCHAR* Keyword = TEXT(""),
		const TCHAR* WordID = TEXT(""))
	{
		FST_MessengerMessage Result;
		Result.SenderName = FText::FromString(Sender);
		Result.Message = FText::FromString(Message);
		Result.bIsPlayer = bIsPlayer;
		Result.KeywordText = FText::FromString(Keyword);
		Result.WordID = WordID;
		return Result;
	}

	struct FBuilder
	{
		explicit FBuilder(UWidgetBlueprint* InBlueprint)
			: Blueprint(InBlueprint)
			, Tree(InBlueprint->WidgetTree)
		{
		}

		template <typename T>
		T* Make(const FName Name, const bool bVariable = false) const
		{
			T* Widget = Tree->ConstructWidget<T>(T::StaticClass(), Name);
			Widget->bIsVariable = bVariable;
			Blueprint->OnVariableAdded(Widget->GetFName());
			return Widget;
		}

		UUserWidget* MakeUserWidget(UClass* WidgetClass, const FName Name, const bool bVariable = false) const
		{
			if (!WidgetClass || !WidgetClass->IsChildOf(UUserWidget::StaticClass()))
			{
				return nullptr;
			}
			UUserWidget* Widget = Tree->ConstructWidget<UUserWidget>(WidgetClass, Name);
			Widget->bIsVariable = bVariable;
			Blueprint->OnVariableAdded(Widget->GetFName());
			return Widget;
		}

		UTextBlock* MakeText(
			const FName Name,
			const FString& Value,
			const int32 Size,
			const FLinearColor& Color = WarmWhite,
			const bool bVariable = false,
			const bool bUseForeground = false) const
		{
			UTextBlock* Widget = Make<UTextBlock>(Name, bVariable);
			Widget->SetText(FText::FromString(Value));
			Widget->SetColorAndOpacity(bUseForeground ? FSlateColor::UseForeground() : FSlateColor(Color));
			Widget->SetShadowOffset(FVector2D(1.0f, 1.0f));
			Widget->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
			FSlateFontInfo Font = Widget->GetFont();
			Font.Size = Size;
			Widget->SetFont(Font);
			return Widget;
		}

		UImage* MakeTextureImage(
			const FName Name,
			const TCHAR* TexturePath,
			const bool bVariable = true,
			const bool bUseForeground = false) const
		{
			(void)bUseForeground;
			UImage* Image = Make<UImage>(Name, bVariable);
			if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
			{
				Image->SetBrushFromTexture(Texture, true);
				FSlateBrush Brush = Image->GetBrush();
				// Texture art must retain its authored color. Button foreground inheritance
				// made alpha-based folder/app art render effectively black in Standalone.
				Brush.TintColor = FSlateColor(FLinearColor::White);
				Image->SetBrush(Brush);
				Image->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Tablet redesign texture is missing: %s"), TexturePath);
			}
			return Image;
		}

		UImage* MakeColorImage(const FName Name, const FLinearColor& Color) const
		{
			UImage* Image = Make<UImage>(Name);
			Image->SetColorAndOpacity(Color);
			return Image;
		}

		UBorder* MakeBorder(
			const FName Name,
			const FLinearColor& Color,
			const FMargin& Padding = FMargin(0.0f),
			const bool bVariable = false) const
		{
			UBorder* Border = Make<UBorder>(Name, bVariable);
			Border->SetBrushColor(Color);
			Border->SetPadding(Padding);
			return Border;
		}

		UButton* MakeTransparentButton(const FName Name) const
		{
			UButton* Button = Make<UButton>(Name, true);
			FSlateBrush InvisibleBrush;
			InvisibleBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
			FButtonStyle Style;
			Style.SetNormal(InvisibleBrush);
			Style.SetHovered(InvisibleBrush);
			Style.SetPressed(InvisibleBrush);
			Style.SetDisabled(InvisibleBrush);
			Style.SetNormalForeground(FSlateColor(FLinearColor(0.90f, 0.86f, 0.78f, 1.0f)));
			Style.SetHoveredForeground(FSlateColor(FLinearColor::White));
			Style.SetPressedForeground(FSlateColor(FLinearColor(0.72f, 0.68f, 0.62f, 1.0f)));
			Style.SetNormalPadding(FMargin(0.0f));
			Style.SetPressedPadding(FMargin(2.0f));
			Button->SetStyle(Style);
			return Button;
		}

		UButton* MakeTextButton(const FName Name, const FString& Label, const int32 FontSize = 30) const
		{
			UButton* Button = MakeTransparentButton(Name);
			UTextBlock* Text = MakeText(
				*FString::Printf(TEXT("TXT_%s"), *Name.ToString().RightChop(4)),
				Label,
				FontSize,
				WarmWhite,
				false,
				true);
			Text->SetJustification(ETextJustify::Center);
			Button->SetContent(Text);
			return Button;
		}

		UButton* MakeIconButton(
			const FName ButtonName,
			const FName ImageName,
			const TCHAR* TexturePath,
			const FString& Label,
			const FVector2D& IconPosition,
			const FVector2D& IconSize,
			const float LabelY) const
		{
			UButton* Button = MakeTransparentButton(ButtonName);
			UCanvasPanel* Cell = Make<UCanvasPanel>(
				*FString::Printf(TEXT("Canvas_%s"), *ButtonName.ToString().RightChop(4)));
			Button->SetContent(Cell);

			// Direct canvas images render reliably in Standalone and expose an editable
			// Canvas Slot in the WBP Designer.
			Place(
				Cell,
				MakeTextureImage(ImageName, TexturePath, true, false),
				IconPosition.X,
				IconPosition.Y,
				IconSize.X,
				IconSize.Y);

			UTextBlock* LabelText = MakeText(
				*FString::Printf(TEXT("TXT_%s"), *ButtonName.ToString().RightChop(4)),
				Label,
				25,
				WarmWhite,
				false,
				true);
			LabelText->SetJustification(ETextJustify::Center);
			Place(Cell, LabelText, 0.0f, LabelY, 170.0f, 42.0f);
			return Button;
		}

		static UCanvasPanelSlot* Place(
			UCanvasPanel* Canvas,
			UWidget* Widget,
			const float X,
			const float Y,
			const float Width,
			const float Height,
			const int32 ZOrder = 0)
		{
			UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
			Slot->SetAnchors(FAnchors(0.0f, 0.0f));
			Slot->SetPosition(FVector2D(X, Y));
			Slot->SetSize(FVector2D(Width, Height));
			Slot->SetZOrder(ZOrder);
			return Slot;
		}

		static UCanvasPanelSlot* FillCanvas(UCanvasPanel* Canvas, UWidget* Widget, const int32 ZOrder = 0)
		{
			UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetZOrder(ZOrder);
			return Slot;
		}

		static UOverlaySlot* FillOverlay(UOverlay* Overlay, UWidget* Widget)
		{
			UOverlaySlot* Slot = Overlay->AddChildToOverlay(Widget);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
			return Slot;
		}

		UCanvasPanel* BuildHomePage() const
		{
			UCanvasPanel* Page = Make<UCanvasPanel>(TEXT("Page_Home"));

			Place(
				Page,
				MakeIconButton(
					TEXT("BTN_Sister"), TEXT("IMG_Sister"), SisterPath, TEXT("여동생"),
					FVector2D(15, 2), FVector2D(140, 112), 124),
				72, 108, 170, 170);
			Place(
				Page,
				MakeIconButton(
					TEXT("BTN_Brother"), TEXT("IMG_Brother"), BrotherPath, TEXT("형"),
					FVector2D(15, 2), FVector2D(140, 112), 124),
				262, 108, 170, 170);
			Place(
				Page,
				MakeIconButton(
					TEXT("BTN_Mother"), TEXT("IMG_Mother"), MotherPath, TEXT("어머니"),
					FVector2D(15, 2), FVector2D(140, 112), 124),
				452, 108, 170, 170);

			UOverlay* Sticky = Make<UOverlay>(TEXT("Overlay_StickyNote"));
			Sticky->SetClipping(EWidgetClipping::ClipToBounds);
			FillOverlay(Sticky, MakeTextureImage(TEXT("IMG_StickyNote"), StickyPath));
			UTextBlock* StickyText = MakeText(
				TEXT("TXT_StickyNote"),
				TEXT("동생 생일\n05 / 13\n\n선물: 스노우글로브"),
				25,
				WarmDark);
			StickyText->SetJustification(ETextJustify::Center);
			UOverlaySlot* StickyTextSlot = Sticky->AddChildToOverlay(StickyText);
			StickyTextSlot->SetHorizontalAlignment(HAlign_Center);
			StickyTextSlot->SetVerticalAlignment(VAlign_Center);
			StickyTextSlot->SetPadding(FMargin(34, 58, 34, 24));
			Sticky->SetVisibility(ESlateVisibility::HitTestInvisible);
			Place(Page, Sticky, 1000, 105, 320, 240);

			UButton* Messenger = MakeIconButton(
				TEXT("BTN_Messenger"), TEXT("IMG_Messenger"), MessengerPath, TEXT("메신저"),
				FVector2D(25, 0), FVector2D(120, 120), 132);
			UCanvasPanel* MessengerCell = CastChecked<UCanvasPanel>(Messenger->GetContent());
			UBorder* Badge = MakeBorder(TEXT("BRD_MessengerBadge"), RedBadge, FMargin(2.0f), true);
			FSlateBrush BadgeBrush;
			BadgeBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			BadgeBrush.OutlineSettings.CornerRadii = FVector4(18.0f);
			BadgeBrush.TintColor = FSlateColor(RedBadge);
			Badge->SetBrush(BadgeBrush);
			UTextBlock* BadgeText = MakeText(
				TEXT("TXT_UnreadMessageCount"), TEXT("0"), 18, FLinearColor::White, true);
			BadgeText->SetJustification(ETextJustify::Center);
			Badge->SetContent(BadgeText);
			Badge->SetVisibility(ESlateVisibility::Collapsed);
			Place(MessengerCell, Badge, 132, 0, 36, 36, 10);
			Place(Page, Messenger, 72, 820, 170, 180);

			Place(
				Page,
				MakeIconButton(
					TEXT("BTN_Internet"), TEXT("IMG_Internet"), InternetPath, TEXT("인터넷"),
					FVector2D(25, 0), FVector2D(120, 120), 132),
				252, 820, 170, 180);
			Place(
				Page,
				MakeIconButton(
					TEXT("BTN_Memo"), TEXT("IMG_Memo"), MemoPath, TEXT("메모장"),
					FVector2D(25, 0), FVector2D(120, 120), 132),
				432, 820, 170, 180);
			return Page;
		}

		UCanvasPanel* MakePageBase(const FName Name) const
		{
			UCanvasPanel* Page = Make<UCanvasPanel>(Name);
			FillCanvas(Page, MakeColorImage(
				*FString::Printf(TEXT("IMG_%sBackground"), *Name.ToString()),
				PageBackground));
			return Page;
		}

		UCanvasPanel* BuildPersonFolderPage() const
		{
			UCanvasPanel* Page = MakePageBase(TEXT("Page_PersonFolder"));
			Place(Page, MakeTextButton(TEXT("BTN_FolderBack"), TEXT("←"), 38), 38, 78, 86, 64, 5);
			Place(Page, MakeText(TEXT("TXT_FolderTitle"), TEXT("여동생"), 34, WarmWhite, true), 145, 88, 600, 60, 5);
			Place(Page, MakeText(TEXT("TXT_FolderSubtitle"), TEXT("보관된 기록"), 20, WarmMuted), 150, 148, 420, 42, 5);

			UBorder* RecordArea = MakeBorder(TEXT("BRD_FolderRecordArea"), PagePanel, FMargin(28.0f));
			UCanvasPanel* Records = Make<UCanvasPanel>(TEXT("Canvas_FolderRecords"));
			RecordArea->SetContent(Records);
			Place(Records, MakeText(TEXT("TXT_Episode01"), TEXT("EPISODE 01"), 22, WarmMuted), 12, 8, 400, 42);
			UScrollBox* PhotoScroll = Make<UScrollBox>(TEXT("SB_EvidencePhotos"));
			UWrapBox* PhotoWrap = Make<UWrapBox>(TEXT("WB_EvidencePhotos"), true);
			PhotoWrap->SetInnerSlotPadding(FVector2D(12.0f, 12.0f));
			PhotoScroll->AddChild(PhotoWrap);
			Place(Records, PhotoScroll, 18, 64, 664, 150);
			Place(Records, MakeTextButton(TEXT("BTN_EvidenceStatement"), TEXT("▤  진술서"), 22), 699, 76, 210, 110);
			Place(Records, MakeText(TEXT("TXT_Episode02"), TEXT("EPISODE 02     잠김"), 22, WarmMuted), 12, 230, 600, 42);
			Place(Page, RecordArea, 120, 220, 1200, 410, 5);
			return Page;
		}

		UCanvasPanel* BuildAppPage(
			const FName PageName,
			const FName BackButtonName,
			const FString& Title,
			const FString& Message,
			const TCHAR* IconPath,
			const FName IconName) const
		{
			UCanvasPanel* Page = MakePageBase(PageName);
			Place(Page, MakeTextButton(BackButtonName, TEXT("←"), 38), 38, 78, 86, 64, 5);

			Place(Page, MakeTextureImage(IconName, IconPath), 590, 205, 260, 220, 5);

			UTextBlock* TitleText = MakeText(
				*FString::Printf(TEXT("TXT_%sTitle"), *PageName.ToString().RightChop(5)),
				Title,
				38);
			TitleText->SetJustification(ETextJustify::Center);
			Place(Page, TitleText, 320, 460, 800, 70, 5);

			UTextBlock* MessageText = MakeText(
				*FString::Printf(TEXT("TXT_%sMessage"), *PageName.ToString().RightChop(5)),
				Message,
				24,
				WarmMuted);
			MessageText->SetJustification(ETextJustify::Center);
			MessageText->SetAutoWrapText(true);
			Place(Page, MessageText, 320, 550, 800, 150, 5);
			return Page;
		}

		void BuildMessengerKeyword() const
		{
			UButton* Root = MakeTransparentButton(TEXT("BTN_Keyword"));
			UTextBlock* Label = MakeText(
				TEXT("TXT_Keyword"), TEXT("단서"), 22, MessengerAccent, true);
			Label->SetShadowOffset(FVector2D(0.0f, 1.0f));
			Root->SetContent(Label);
			Tree->RootWidget = Root;
		}

		void BuildMessengerRoom() const
		{
			UButton* Root = MakeTransparentButton(TEXT("BTN_Room"));
			USizeBox* RowSize = Make<USizeBox>(TEXT("SB_RoomSize"));
			RowSize->SetWidthOverride(350.0f);
			RowSize->SetHeightOverride(118.0f);
			Root->SetContent(RowSize);

			UCanvasPanel* Canvas = Make<UCanvasPanel>(TEXT("Canvas_Room"));
			RowSize->SetContent(Canvas);
			UBorder* Background = MakeBorder(
				TEXT("BRD_RoomBackground"), FLinearColor(0.15f, 0.105f, 0.065f, 0.96f));
			Background->SetVisibility(ESlateVisibility::HitTestInvisible);
			FillCanvas(Canvas, Background);

			UBorder* Selected = MakeBorder(
				TEXT("BRD_Selected"), FLinearColor(0.48f, 0.29f, 0.09f, 0.72f), FMargin(0.0f), true);
			Selected->SetVisibility(ESlateVisibility::Collapsed);
			FillCanvas(Canvas, Selected, 1);

			UTextBlock* RoomName = MakeText(
				TEXT("TXT_RoomName"), TEXT("대화방"), 25, WarmWhite, true);
			RoomName->SetVisibility(ESlateVisibility::HitTestInvisible);
			Place(Canvas, RoomName, 18, 12, 275, 38, 2);

			UTextBlock* Preview = MakeText(
				TEXT("TXT_LastMessage"), TEXT("마지막 메시지"), 18, WarmMuted, true);
			Preview->SetClipping(EWidgetClipping::ClipToBounds);
			Preview->SetVisibility(ESlateVisibility::HitTestInvisible);
			Place(Canvas, Preview, 18, 60, 300, 34, 2);

			UBorder* Badge = MakeBorder(
				TEXT("BRD_UnreadBadge"), RedBadge, FMargin(2.0f), true);
			FSlateBrush BadgeBrush;
			BadgeBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
			BadgeBrush.OutlineSettings.CornerRadii = FVector4(16.0f);
			BadgeBrush.TintColor = FSlateColor(RedBadge);
			Badge->SetBrush(BadgeBrush);
			Badge->SetVisibility(ESlateVisibility::HitTestInvisible);
			UTextBlock* Unread = MakeText(
				TEXT("TXT_UnreadCount"), TEXT("1"), 17, FLinearColor::White, true);
			Unread->SetJustification(ETextJustify::Center);
			Unread->SetVisibility(ESlateVisibility::HitTestInvisible);
			Badge->SetContent(Unread);
			Place(Canvas, Badge, 304, 14, 32, 32, 3);

			Tree->RootWidget = Root;
		}

		void BuildMessengerMessage() const
		{
			UVerticalBox* Root = Make<UVerticalBox>(TEXT("VB_MessageRoot"));
			UTextBlock* Sender = MakeText(
				TEXT("TXT_SenderName"), TEXT("보낸 사람"), 17, WarmMuted, true);
			Sender->SetVisibility(ESlateVisibility::HitTestInvisible);
			UVerticalBoxSlot* SenderSlot = Root->AddChildToVerticalBox(Sender);
			SenderSlot->SetPadding(FMargin(22.0f, 8.0f, 22.0f, 4.0f));

			UHorizontalBox* Alignment = Make<UHorizontalBox>(TEXT("HB_MessageAlignment"));
			UVerticalBoxSlot* AlignmentSlot = Root->AddChildToVerticalBox(Alignment);
			AlignmentSlot->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 12.0f));

			USpacer* LeftSpacer = Make<USpacer>(TEXT("Spacer_Left"), true);
			LeftSpacer->SetSize(FVector2D(1.0f, 1.0f));
			UHorizontalBoxSlot* LeftSlot = Alignment->AddChildToHorizontalBox(LeftSpacer);
			LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

			auto AddTime = [this, Alignment](const FName Name)
			{
				UTextBlock* Time = MakeText(Name, TEXT("오후 6:30"), 17, WarmMuted, true);
				Time->SetVisibility(ESlateVisibility::HitTestInvisible);
				UHorizontalBoxSlot* TimeSlot = Alignment->AddChildToHorizontalBox(Time);
				TimeSlot->SetVerticalAlignment(VAlign_Bottom);
				TimeSlot->SetPadding(FMargin(10, 0, 10, 3));
			};
			AddTime(TEXT("TXT_TimeLeft"));
			USizeBox* BubbleLimit = Make<USizeBox>(TEXT("SB_BubbleLimit"));
			BubbleLimit->SetMaxDesiredWidth(620.0f);
			UHorizontalBoxSlot* BubbleSlot = Alignment->AddChildToHorizontalBox(BubbleLimit);
			BubbleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

			UBorder* Bubble = MakeBorder(
				TEXT("BRD_Bubble"), PagePanel, FMargin(18.0f, 12.0f), true);
			BubbleLimit->SetContent(Bubble);
			UWrapBox* Content = Make<UWrapBox>(TEXT("WB_MessageContent"), true);
			Content->SetWrapSize(580.0f);
			Content->SetExplicitWrapSize(true);
			Content->SetInnerSlotPadding(FVector2D(0.0f, 2.0f));
			Bubble->SetContent(Content);
			AddTime(TEXT("TXT_TimeRight"));

			USpacer* RightSpacer = Make<USpacer>(TEXT("Spacer_Right"), true);
			RightSpacer->SetSize(FVector2D(1.0f, 1.0f));
			UHorizontalBoxSlot* RightSlot = Alignment->AddChildToHorizontalBox(RightSpacer);
			RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

			Tree->RootWidget = Root;
		}

		void BuildDateSeparator() const
		{
			UBorder* Root = MakeBorder(TEXT("BRD_DateSpacing"), FLinearColor::Transparent, FMargin(24, 30, 24, 26));
			Root->SetVisibility(ESlateVisibility::HitTestInvisible);
			UHorizontalBox* Row = Make<UHorizontalBox>(TEXT("HB_Date"));
			Root->SetContent(Row);
			auto AddLine = [this, Row](const FName Name)
			{
				USizeBox* Size = Make<USizeBox>(Name);
				Size->SetHeightOverride(1.0f);
				Size->SetContent(MakeBorder(*FString::Printf(TEXT("%s_Line"), *Name.ToString()), WarmMuted));
				auto* Slot = Row->AddChildToHorizontalBox(Size);
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Slot->SetVerticalAlignment(VAlign_Center);
			};
			AddLine(TEXT("DateLineLeft"));
			auto* Label = MakeText(TEXT("TXT_Date"), TEXT("날짜"), 18, WarmMuted, true);
			auto* LabelSlot = Row->AddChildToHorizontalBox(Label);
			LabelSlot->SetPadding(FMargin(18, 0));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			AddLine(TEXT("DateLineRight"));
			Tree->RootWidget = Root;
		}

		void BuildMessenger() const
		{
			UCanvasPanel* Root = Make<UCanvasPanel>(TEXT("Canvas_MessengerRoot"));
			FillCanvas(Root, MakeColorImage(
				TEXT("IMG_MessengerBackground"), FLinearColor(0.095f, 0.065f, 0.042f, 0.985f)));

			Place(Root, MakeTextButton(TEXT("BTN_Back"), TEXT("←"), 38), 38, 76, 86, 64, 5);
			Place(Root, MakeText(TEXT("TXT_MessengerTitle"), TEXT("메신저"), 35), 145, 82, 360, 55, 5);
			Place(
				Root,
				MakeText(TEXT("TXT_ArchiveLabel"), TEXT("과거 대화 · 읽기 전용"), 18, WarmMuted),
				1110, 92, 260, 36, 5);

			UBorder* RoomPanel = MakeBorder(
				TEXT("BRD_RoomPanel"), FLinearColor(0.12f, 0.082f, 0.052f, 0.98f), FMargin(18.0f));
			UCanvasPanel* RoomCanvas = Make<UCanvasPanel>(TEXT("Canvas_RoomPanel"));
			RoomPanel->SetContent(RoomCanvas);
			Place(RoomCanvas, MakeText(TEXT("TXT_RoomListTitle"), TEXT("대화방"), 22), 6, 0, 300, 42);
			UScrollBox* RoomList = Make<UScrollBox>(TEXT("SB_ChatRoomList"), true);
			RoomList->SetAnimateWheelScrolling(true);
			RoomList->SetScrollBarVisibility(ESlateVisibility::Visible);
			Place(RoomCanvas, RoomList, 0, 52, 365, 690);
			Place(Root, RoomPanel, 36, 154, 405, 806, 5);

			UBorder* MessagePanel = MakeBorder(
				TEXT("BRD_MessagePanel"), FLinearColor(0.14f, 0.095f, 0.058f, 0.98f), FMargin(18.0f));
			UCanvasPanel* MessageCanvas = Make<UCanvasPanel>(TEXT("Canvas_MessagePanel"));
			MessagePanel->SetContent(MessageCanvas);
			UTextBlock* CurrentRoomName = MakeText(
				TEXT("TXT_CurrentRoomName"), TEXT(""), 26, WarmWhite, true);
			Place(MessageCanvas, CurrentRoomName, 12, 0, 820, 45);

			UScrollBox* MessageList = Make<UScrollBox>(TEXT("SB_MessageList"), true);
			MessageList->SetAnimateWheelScrolling(true);
			MessageList->SetScrollBarVisibility(ESlateVisibility::Visible);
			Place(MessageCanvas, MessageList, 0, 55, 876, 580);

			UTextBlock* Prompt = MakeText(
				TEXT("TXT_SelectRoomPrompt"), TEXT("대화방을 선택하세요."), 26, WarmMuted, true);
			Prompt->SetJustification(ETextJustify::Center);
			Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
			Place(MessageCanvas, Prompt, 90, 290, 696, 60, 2);

			UBorder* DisabledInput = MakeBorder(
				TEXT("BRD_DisabledInputArea"), FLinearColor(0.085f, 0.06f, 0.042f, 1.0f), FMargin(20.0f));
			DisabledInput->SetVisibility(ESlateVisibility::HitTestInvisible);
			UTextBlock* DisabledText = MakeText(
				TEXT("TXT_DisabledInput"), TEXT("현재 대화가 불가능합니다"), 20, WarmMuted);
			DisabledText->SetJustification(ETextJustify::Center);
			DisabledInput->SetContent(DisabledText);
			Place(MessageCanvas, DisabledInput, 0, 660, 876, 82);
			Place(Root, MessagePanel, 462, 154, 942, 806, 5);

			Tree->RootWidget = Root;
		}

		UOverlay* BuildStatusBar() const
		{
			UOverlay* Status = Make<UOverlay>(TEXT("StatusBar"));
			FillOverlay(Status, MakeColorImage(TEXT("IMG_StatusBarShade"), FLinearColor(0, 0, 0, 0.18f)));

			UHorizontalBox* Row = Make<UHorizontalBox>(TEXT("StatusBarRow"));
			UOverlaySlot* RowSlot = FillOverlay(Status, Row);
			RowSlot->SetPadding(FMargin(26, 8, 26, 6));
			UHorizontalBoxSlot* LeftSlot = Row->AddChildToHorizontalBox(
				MakeText(TEXT("TXT_StatusLeft"), TEXT("Wi-Fi     Battery 100%"), 20));
			LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LeftSlot->SetVerticalAlignment(VAlign_Center);
			UTextBlock* Right = MakeText(
				TEXT("TXT_StatusRight"), TEXT("5월 26일 (일)     오후 09:17"), 20);
			Right->SetJustification(ETextJustify::Right);
			UHorizontalBoxSlot* RightSlot = Row->AddChildToHorizontalBox(Right);
			RightSlot->SetVerticalAlignment(VAlign_Center);
			Status->SetVisibility(ESlateVisibility::HitTestInvisible);
			return Status;
		}

		UOverlay* BuildPopup() const
		{
			UOverlay* Popup = Make<UOverlay>(TEXT("PopupLayer"), true);
			FillOverlay(Popup, MakeColorImage(TEXT("IMG_PopupShade"), FLinearColor(0, 0, 0, 0.72f)));

			USizeBox* PopupSize = Make<USizeBox>(TEXT("SB_Popup"));
			PopupSize->SetWidthOverride(820.0f);
			PopupSize->SetHeightOverride(680.0f);
			UOverlaySlot* PopupSizeSlot = Popup->AddChildToOverlay(PopupSize);
			PopupSizeSlot->SetHorizontalAlignment(HAlign_Center);
			PopupSizeSlot->SetVerticalAlignment(VAlign_Center);

			UBorder* Panel = MakeBorder(TEXT("BRD_PopupPanel"), PageBackground, FMargin(30.0f));
			PopupSize->SetContent(Panel);
			UCanvasPanel* Canvas = Make<UCanvasPanel>(TEXT("Canvas_Popup"));
			Panel->SetContent(Canvas);
			Place(Canvas, MakeText(TEXT("TXT_PopupTitle"), TEXT("증거 사진"), 30, WarmWhite, true), 20, 15, 650, 55);
			Place(Canvas, MakeTextButton(TEXT("BTN_PopupClose"), TEXT("×"), 36), 700, 5, 58, 58);
			UBorder* Preview = MakeBorder(TEXT("BRD_PopupPreview"), PagePanel);
			Place(Canvas, Preview, 20, 95, 450, 330);
			Place(Canvas, MakeText(TEXT("TXT_PuzzleKeywordLabel"), TEXT("획득 키워드"), 18, WarmMuted), 500, 95, 240, 34);
			Place(Canvas, MakeTextButton(TEXT("BTN_PuzzleWord01"), TEXT("키워드 1"), 21), 500, 135, 240, 54);
			Place(Canvas, MakeTextButton(TEXT("BTN_PuzzleWord02"), TEXT("키워드 2"), 21), 500, 195, 240, 54);
			Place(Canvas, MakeTextButton(TEXT("BTN_PuzzleWord03"), TEXT("키워드 3"), 21), 500, 255, 240, 54);
			Place(Canvas, MakeText(TEXT("TXT_PuzzlePhotoLabel"), TEXT("완성 사진"), 18, WarmMuted), 500, 325, 240, 34);
			UScrollBox* PuzzlePhotoScroll = Make<UScrollBox>(TEXT("SB_PuzzlePhotos"));
			UWrapBox* PuzzlePhotoWrap = Make<UWrapBox>(TEXT("WB_PuzzlePhotos"), true);
			PuzzlePhotoWrap->SetInnerSlotPadding(FVector2D(8.0f, 8.0f));
			PuzzlePhotoScroll->AddChild(PuzzlePhotoWrap);
			Place(Canvas, PuzzlePhotoScroll, 500, 365, 240, 100);
			UTextBlock* Body = MakeText(
				TEXT("TXT_PopupBody"), TEXT("증거 Placeholder"), 22, WarmMuted, true);
			Body->SetJustification(ETextJustify::Center);
			Body->SetAutoWrapText(true);
			Place(Canvas, Body, 40, 450, 700, 100);
			Place(Canvas, MakeTextButton(TEXT("BTN_StatementSubmit"), TEXT("자백 반증"), 23), 530, 565, 210, 62);
			Popup->SetVisibility(ESlateVisibility::Collapsed);
			return Popup;
		}

		void BuildLogicalScreen(UCanvasPanel* LogicalScreen) const
		{
			UWidgetSwitcher* Switcher = Make<UWidgetSwitcher>(TEXT("WidgetSwitcher_TabletPage"), true);
			Switcher->AddChild(BuildHomePage());
			Switcher->AddChild(BuildPersonFolderPage());
			if (UClass* MessengerClass = LoadClass<UUserWidget>(nullptr, MessengerClassPath))
			{
				Switcher->AddChild(MakeUserWidget(MessengerClass, TEXT("WBP_Messenger"), true));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Tablet redesign could not load WBP_Messenger."));
				Switcher->AddChild(BuildAppPage(
					TEXT("Page_Messenger"), TEXT("BTN_MessengerBack"), TEXT("메신저"),
					TEXT("메신저 UI를 불러올 수 없습니다."), MessengerPath, TEXT("IMG_MessengerPage")));
			}
			Switcher->AddChild(BuildAppPage(
				TEXT("Page_Internet"), TEXT("BTN_InternetBack"), TEXT("인터넷"),
				TEXT("네트워크에 연결할 수 없습니다."), InternetPath, TEXT("IMG_InternetPage")));
			Switcher->AddChild(BuildAppPage(
				TEXT("Page_Memo"), TEXT("BTN_MemoBack"), TEXT("메모장"),
				TEXT("읽기 전용입니다.\n현재는 메모를 입력할 수 없습니다."), MemoPath, TEXT("IMG_MemoPage")));
			Switcher->SetActiveWidgetIndex(0);
			FillCanvas(LogicalScreen, Switcher, 0);
			Place(LogicalScreen, BuildStatusBar(), 0, 0, 1440, 60, 10);
			FillCanvas(LogicalScreen, BuildPopup(), 20);
		}

		void Build() const
		{
			UCanvasPanel* Root = Make<UCanvasPanel>(TEXT("Canvas_ViewportRoot"));
			Tree->RootWidget = Root;

			UButton* WorldBlocker = MakeTransparentButton(TEXT("BTN_WorldInputBlocker"));
			FillCanvas(Root, WorldBlocker, -10);

			UScaleBox* TabletScale = Make<UScaleBox>(TEXT("ScaleBox_Tablet"));
			TabletScale->SetStretch(EStretch::ScaleToFit);
			UCanvasPanelSlot* ScaleSlot = Root->AddChildToCanvas(TabletScale);
			ScaleSlot->SetAnchors(FAnchors(0.0f, 0.05f, 1.0f, 0.95f));
			ScaleSlot->SetOffsets(FMargin(0.0f));
			ScaleSlot->SetZOrder(0);

			USizeBox* TabletSize = Make<USizeBox>(TEXT("SizeBox_Tablet"));
			TabletSize->SetWidthOverride(1448.0f);
			TabletSize->SetHeightOverride(1086.0f);
			TabletScale->SetContent(TabletSize);

			UCanvasPanel* TabletRoot = Make<UCanvasPanel>(TEXT("Canvas_Root"));
			TabletSize->SetContent(TabletRoot);

			UBorder* ScreenContent = MakeBorder(TEXT("ScreenContent"), FLinearColor::Transparent);
			ScreenContent->SetClipping(EWidgetClipping::ClipToBounds);
			UCanvasPanel* ScreenLayers = Make<UCanvasPanel>(TEXT("Canvas_ScreenLayers"));
			ScreenContent->SetContent(ScreenLayers);
			UImage* Wallpaper = MakeTextureImage(TEXT("IMG_FamilyWallpaper"), FamilyPath);
			Wallpaper->SetVisibility(ESlateVisibility::HitTestInvisible);
			FSlateBrush WallpaperBrush = Wallpaper->GetBrush();
			// Family is 4:3 while the measured screen is wider. Crop top/bottom so the
			// direct Image fills the panel without stretching the people.
			WallpaperBrush.SetUVRegion(FBox2f(FVector2f(0.0f, 0.0685f), FVector2f(1.0f, 0.9315f)));
			Wallpaper->SetBrush(WallpaperBrush);
			Place(ScreenLayers, Wallpaper, 0, 0, 1255, 811, 0);
			UImage* WallpaperOverlay = MakeColorImage(
				TEXT("IMG_WallpaperDarkOverlay"), FLinearColor(0, 0, 0, 0.24f));
			WallpaperOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
			FillCanvas(ScreenLayers, WallpaperOverlay, 1);
			UScaleBox* LogicalScale = Make<UScaleBox>(TEXT("ScaleBox_LogicalScreen"));
			LogicalScale->SetStretch(EStretch::ScaleToFit);
			Place(ScreenLayers, LogicalScale, 87, 0, 1081, 811, 2);
			USizeBox* LogicalSize = Make<USizeBox>(TEXT("SizeBox_LogicalScreen_1440x1080"));
			LogicalSize->SetWidthOverride(1440.0f);
			LogicalSize->SetHeightOverride(1080.0f);
			LogicalScale->SetContent(LogicalSize);
			UCanvasPanel* LogicalScreen = Make<UCanvasPanel>(TEXT("Canvas_LogicalScreen_1440x1080"));
			LogicalSize->SetContent(LogicalScreen);
			BuildLogicalScreen(LogicalScreen);
			Place(TabletRoot, ScreenContent, 96, 123, 1255, 811, 0);

			UImage* Body = MakeTextureImage(TEXT("IMG_TabletBody"), TabletBodyPath);
			Body->SetVisibility(ESlateVisibility::HitTestInvisible);
			Place(TabletRoot, Body, 0, 0, 1448, 1086, 10);

			UButton* PhysicalHome = MakeTransparentButton(TEXT("BTN_PhysicalHome"));
			Place(TabletRoot, PhysicalHome, 650, 966, 150, 70, 20);
		}

		UWidgetBlueprint* Blueprint;
		UWidgetTree* Tree;
	};

	bool ClearWidgetTree(UWidgetBlueprint* Blueprint)
	{
		if (!Blueprint || !Blueprint->WidgetTree)
		{
			return false;
		}

		UWidgetTree* Tree = Blueprint->WidgetTree;
		TArray<UWidget*> OldWidgets;
		Tree->GetAllWidgets(OldWidgets);
		Tree->RootWidget = nullptr;

		for (int32 Index = OldWidgets.Num() - 1; Index >= 0; --Index)
		{
			UWidget* Widget = OldWidgets[Index];
			if (Widget)
			{
				Blueprint->OnVariableRemoved(Widget->GetFName());
			}
			Tree->RemoveWidget(Widget);
			if (Widget && Widget->GetOuter() == Tree)
			{
				const FName DiscardedName = MakeUniqueObjectName(
					GetTransientPackage(),
					Widget->GetClass(),
					*FString::Printf(TEXT("Discarded_%s"), *Widget->GetName()));
				Widget->Rename(
					*DiscardedName.ToString(),
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
			}
		}
		return true;
	}

	bool BuildWidgetBlueprint(
		const TCHAR* InAssetName,
		const TCHAR* InAssetPath,
		UClass* ParentClass,
		const bool bRedesignExisting,
		TFunctionRef<void(const FBuilder&)> BuildTree)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, InAssetPath);
		if (Blueprint && !bRedesignExisting)
		{
			return true;
		}

		if (!Blueprint)
		{
			UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
			Factory->ParentClass = ParentClass;
			IAssetTools& AssetTools =
				FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
			Blueprint = Cast<UWidgetBlueprint>(AssetTools.CreateAsset(
				InAssetName,
				AssetFolder,
				UWidgetBlueprint::StaticClass(),
				Factory));
		}
		else if (!Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(ParentClass))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Cannot redesign %s: parent %s is incompatible with %s."),
				InAssetPath,
				*GetNameSafe(Blueprint->ParentClass),
				*GetNameSafe(ParentClass));
			return false;
		}

		if (!Blueprint || !Blueprint->WidgetTree)
		{
			return false;
		}
		if (Blueprint->WidgetTree->RootWidget && !ClearWidgetTree(Blueprint))
		{
			return false;
		}

		BuildTree(FBuilder(Blueprint));
		const bool bSaved = SaveAndCompile(Blueprint);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("TABLET_WIDGET_BUILD Result=%s Asset=%s"),
			bSaved ? TEXT("Success") : TEXT("Failure"),
			InAssetPath);
		return bSaved;
	}

	bool BuildMessengerWidgetBlueprints(const bool bRedesignExisting)
	{
		return BuildWidgetBlueprint(TEXT("WBP_MessengerDateSeparator"),
			TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerDateSeparator.WBP_MessengerDateSeparator"),
			UBalhwajeomMessengerDateSeparator::StaticClass(), bRedesignExisting,
			[](const FBuilder& Builder) { Builder.BuildDateSeparator(); })
			&& BuildWidgetBlueprint(
			KeywordAssetName,
			KeywordAssetPath,
			UBalhwajeomMessengerKeywordWidget::StaticClass(),
			bRedesignExisting,
			[](const FBuilder& Builder) { Builder.BuildMessengerKeyword(); })
			&& BuildWidgetBlueprint(
				RoomAssetName,
				RoomAssetPath,
				UBalhwajeomMessengerRoomWidget::StaticClass(),
				bRedesignExisting,
				[](const FBuilder& Builder) { Builder.BuildMessengerRoom(); })
			&& BuildWidgetBlueprint(
				MessageAssetName,
				MessageAssetPath,
				UBalhwajeomMessengerMessageWidget::StaticClass(),
				bRedesignExisting,
				[](const FBuilder& Builder) { Builder.BuildMessengerMessage(); })
			&& BuildWidgetBlueprint(
				MessengerAssetName,
				MessengerAssetPath,
				UBalhwajeomMessengerWidget::StaticClass(),
				bRedesignExisting,
				[](const FBuilder& Builder) { Builder.BuildMessenger(); });
	}

	bool CreateMessengerDataAssetsInternal()
	{
		auto EnsureRoom = [](
			const TCHAR* InRoomAssetName,
			const TCHAR* RoomID,
			const TCHAR* RoomName,
			const int32 InitialUnreadCount,
			TArray<FST_MessengerMessage>&& Messages)
		{
			bool bCreated = false;
			UBalhwajeomMessengerRoomDataAsset* Room =
				LoadOrCreateDataAsset<UBalhwajeomMessengerRoomDataAsset>(
					InRoomAssetName,
					MessengerRoomDataFolder,
					bCreated);
			if (Room && bCreated)
			{
				Room->RoomID = RoomID;
				Room->RoomName = FText::FromString(RoomName);
				Room->InitialUnreadCount = InitialUnreadCount;
				Room->Messages = MoveTemp(Messages);
				for (int32 Index = 0; Index < Room->Messages.Num(); ++Index)
				{
					Room->Messages[Index].SentAt = FDateTime(2026, 5, 12, 18, 30)
						+ FTimespan(Index >= 2 ? 1 : 0, 0, Index * 2, 0);
				}
				if (!SaveDataAsset(Room))
				{
					return static_cast<UBalhwajeomMessengerRoomDataAsset*>(nullptr);
				}
			}
			return Room;
		};

		TArray<TObjectPtr<UBalhwajeomMessengerRoomDataAsset>> RoomAssets;
		RoomAssets.Add(EnsureRoom(
			TEXT("DA_MessengerRoom_Family"),
			TEXT("Family"),
			TEXT("우리 가족"),
			3,
			{
				MakeArchivedMessage(TEXT("엄마"), TEXT("오늘 저녁은 다 같이 먹을 수 있지?"), false),
				MakeArchivedMessage(TEXT("나"), TEXT("응, 조금 늦어도 꼭 갈게."), true),
				MakeArchivedMessage(TEXT("형"), TEXT("케이크는 내가 찾아갈게."), false),
				MakeArchivedMessage(TEXT("여동생"), TEXT("그럼 사진도 꼭 찍자!"), false),
			}));
		RoomAssets.Add(EnsureRoom(
			TEXT("DA_MessengerRoom_Mother"),
			TEXT("Mother"),
			TEXT("엄마"),
			1,
			{
				MakeArchivedMessage(TEXT("엄마"), TEXT("오늘 저녁 먹고 들어오니?"), false),
				MakeArchivedMessage(TEXT("나"), TEXT("응. 너무 늦지는 않을 거야."), true),
				MakeArchivedMessage(
					TEXT("엄마"),
					TEXT("현관 비밀번호 바뀐 거 잊지 마."),
					false,
					TEXT("현관 비밀번호"),
					TEXT("Mother_DoorCode")),
			}));
		RoomAssets.Add(EnsureRoom(
			TEXT("DA_MessengerRoom_Sister"),
			TEXT("Sister"),
			TEXT("여동생"),
			4,
			{
				MakeArchivedMessage(TEXT("여동생"), TEXT("내 생일 기억하고 있지?"), false),
				MakeArchivedMessage(
					TEXT("나"),
					TEXT("당연하지. 5월 13일."),
					true,
					TEXT("5월 13일"),
					TEXT("Sister_Birthday")),
				MakeArchivedMessage(
					TEXT("여동생"),
					TEXT("내 생일에 스노우 글로브 사준다고 했잖아"),
					false,
					TEXT("스노우 글로브"),
					TEXT("Sister_SnowGlobe")),
				MakeArchivedMessage(TEXT("나"), TEXT("기억하고 있어. 걱정하지 마."), true),
				MakeArchivedMessage(TEXT("여동생"), TEXT("약속이다!"), false),
			}));
		RoomAssets.Add(EnsureRoom(
			TEXT("DA_MessengerRoom_Brother"),
			TEXT("Brother"),
			TEXT("형"),
			0,
			{
				MakeArchivedMessage(
					TEXT("형"),
					TEXT("차 키 식탁 위에 뒀어."),
					false,
					TEXT("차 키"),
					TEXT("Brother_CarKey")),
				MakeArchivedMessage(TEXT("나"), TEXT("확인했어. 내일 가져다줄게."), true),
				MakeArchivedMessage(TEXT("형"), TEXT("그래, 고맙다."), false),
			}));

		if (RoomAssets.Contains(nullptr))
		{
			UE_LOG(LogTemp, Error, TEXT("One or more messenger room Data Assets could not be created."));
			return false;
		}

		bool bCatalogCreated = false;
		UBalhwajeomMessengerCatalogDataAsset* Catalog =
			LoadOrCreateDataAsset<UBalhwajeomMessengerCatalogDataAsset>(
				MessengerCatalogAssetName,
				MessengerDataFolder,
				bCatalogCreated);
		if (!Catalog)
		{
			return false;
		}
		if (bCatalogCreated)
		{
			Catalog->Rooms = MoveTemp(RoomAssets);
			if (!SaveDataAsset(Catalog))
			{
				return false;
			}
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("MESSENGER_DATA_ASSETS Result=Success Catalog=%s"),
			MessengerCatalogAssetPath);
		return true;
	}
}

bool UTabletWidgetBlueprintLibrary::CreateMessengerDataAssets()
{
	return TabletDesigner::CreateMessengerDataAssetsInternal();
}

bool UTabletWidgetBlueprintLibrary::UpdateMessengerTimeline()
{
	using namespace TabletDesigner;
	// Update only the message tree; preserve the existing tablet layout and animations.
	if (!BuildWidgetBlueprint(TEXT("WBP_MessengerDateSeparator"),
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerDateSeparator.WBP_MessengerDateSeparator"),
		UBalhwajeomMessengerDateSeparator::StaticClass(), true,
		[](const FBuilder& Builder) { Builder.BuildDateSeparator(); })
		|| !BuildWidgetBlueprint(MessageAssetName, MessageAssetPath,
			UBalhwajeomMessengerMessageWidget::StaticClass(), true,
			[](const FBuilder& Builder) { Builder.BuildMessengerMessage(); }))
	{
		return false;
	}
	auto* Catalog = LoadObject<UBalhwajeomMessengerCatalogDataAsset>(nullptr, MessengerCatalogAssetPath);
	if (!Catalog) { return false; }
	for (UBalhwajeomMessengerRoomDataAsset* Room : Catalog->Rooms)
	{
		if (!Room || Room->Messages.IsEmpty()) { continue; }
		bool bAllMissing = true;
		for (const auto& Message : Room->Messages)
		{
			bAllMissing &= Message.SentAt == FDateTime::MinValue();
		}
		// One-time migration only: partially authored timelines require planner input.
		if (bAllMissing)
		{
			for (int32 Index = 0; Index < Room->Messages.Num(); ++Index)
			{
				Room->Messages[Index].SentAt = FDateTime(2026, 5, 12, 18, 30)
					+ FTimespan(Index >= 2 ? 1 : 0, 0, Index * 2, 0);
			}
			if (!SaveDataAsset(Room)) { return false; }
		}
	}
	for (const TCHAR* Path : { MessengerAssetPath, AssetPath })
	{
		auto* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Path);
		if (!Blueprint || !SaveAndCompile(Blueprint)) { return false; }
	}
	UE_LOG(LogTemp, Display, TEXT("MESSENGER_TIMELINE_UPDATE Success"));
	return true;
}

bool UTabletWidgetBlueprintLibrary::TestMessengerTimeline()
{
	using namespace TabletDesigner;
	bool bPassed = true;
	auto Require = [&bPassed](bool bValue, const TCHAR* Label)
	{
		if (!bValue) { UE_LOG(LogTemp, Error, TEXT("TIMELINE FAIL: %s"), Label); bPassed = false; }
	};
	Require(MessengerDate::TimeLabel(FDateTime(2026, 5, 12, 0, 5)).ToString() == TEXT("오전 12:05"), TEXT("midnight"));
	Require(MessengerDate::TimeLabel(FDateTime(2026, 5, 12, 12, 0)).ToString() == TEXT("오후 12:00"), TEXT("noon"));
	Require(MessengerDate::TimeLabel(FDateTime()).ToString() == TEXT("시간 미상"), TEXT("missing timestamp"));
	Require(MessengerDate::DateLabel(FDateTime(2026, 5, 12)).ToString() == TEXT("2026년 5월 12일 화요일"), TEXT("weekday"));
	UWorld* World = GEditor->GetEditorWorldContext().World();
	auto* WidgetClass = LoadClass<UBalhwajeomMessengerWidget>(nullptr, MessengerClassPath);
	auto* Messenger = World && WidgetClass ? CreateWidget<UBalhwajeomMessengerWidget>(World, WidgetClass) : nullptr;
	if (!Messenger) { return false; }
	Messenger->InitializeForAutomatedTest();
	auto* Catalog = Messenger->GetMessengerDataAsset();
	if (!Catalog) { return false; }
	auto* List = Cast<UScrollBox>(Messenger->GetWidgetFromName(TEXT("SB_MessageList")));
	if (!List) { return false; }
	Require(List->GetChildrenCount() == 0, TEXT("no automatic selection"));
	for (UBalhwajeomMessengerRoomDataAsset* Room : Catalog->Rooms)
	{
		if (!Room || Room->Messages.IsEmpty()) { continue; }
		const int32 SourceUnread = Room->InitialUnreadCount;
		Require(Messenger->SelectRoomByID(Room->RoomID), TEXT("selection"));
		int32 Row = 0;
		FDateTime PreviousDate;
		for (int32 Index = 0; Index < Room->Messages.Num(); ++Index)
		{
			const auto& Data = Room->Messages[Index];
			if (Index == 0 || Data.SentAt.GetDate() != PreviousDate)
			{
				auto* Separator = Cast<UBalhwajeomMessengerDateSeparator>(List->GetChildAt(Row++));
				Require(Separator != nullptr, TEXT("date boundary inserts separator"));
				auto* Label = Separator ? Cast<UTextBlock>(Separator->GetWidgetFromName(TEXT("TXT_Date"))) : nullptr;
				Require(Label && Label->GetText().EqualTo(MessengerDate::DateLabel(Data.SentAt)), TEXT("date text"));
			}
			PreviousDate = Data.SentAt.GetDate();
			auto* Message = Messenger->GetDisplayedMessageWidget(Index);
			Require(Message && List->GetChildAt(Row++) == Message, TEXT("message indexing excludes date rows"));
			auto* Left = Message ? Cast<UTextBlock>(Message->GetWidgetFromName(TEXT("TXT_TimeLeft"))) : nullptr;
			auto* Right = Message ? Cast<UTextBlock>(Message->GetWidgetFromName(TEXT("TXT_TimeRight"))) : nullptr;
			auto* VisibleTime = Data.bIsPlayer ? Left : Right;
			auto* HiddenTime = Data.bIsPlayer ? Right : Left;
			Require(VisibleTime && VisibleTime->GetVisibility() == ESlateVisibility::HitTestInvisible
				&& VisibleTime->GetText().EqualTo(MessengerDate::TimeLabel(Data.SentAt)), TEXT("time on bubble outer edge"));
			Require(HiddenTime && HiddenTime->GetVisibility() == ESlateVisibility::Collapsed, TEXT("opposite time hidden"));
		}
		Require(List->GetChildrenCount() == Row, TEXT("no extra or previous-room rows"));
		Require(Messenger->GetDisplayedMessageCount() == Room->Messages.Num(), TEXT("message count excludes dates"));
		Require(Room->InitialUnreadCount == SourceUnread && Messenger->GetCurrentUnreadCount(Room->RoomID) == 0, TEXT("source unread immutable"));
		Messenger->InitializeMessenger();
		Require(List->GetChildrenCount() == Row, TEXT("re-entry does not duplicate timeline"));
	}
	UE_LOG(LogTemp, Display, TEXT("MESSENGER_TIMELINE_TEST %s"), bPassed ? TEXT("Success") : TEXT("Failure"));
	return bPassed;
}

bool UTabletWidgetBlueprintLibrary::InspectTabletWidgetBlueprint()
{
	using namespace TabletDesigner;
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
	if (!Blueprint || !Blueprint->WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("Tablet inspection failed: %s could not be loaded."), AssetPath);
		return false;
	}

	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("TABLET_INSPECT Asset=%s Parent=%s Widgets=%d Functions=%d UberGraphs=%d Delegates=%d Animations=%d Variables=%d"),
		AssetPath,
		*GetNameSafe(Blueprint->ParentClass),
		Widgets.Num(),
		Blueprint->FunctionGraphs.Num(),
		Blueprint->UbergraphPages.Num(),
		Blueprint->DelegateSignatureGraphs.Num(),
		Blueprint->Animations.Num(),
		Blueprint->NewVariables.Num());
	for (const UWidget* Widget : Widgets)
	{
		UE_LOG(LogTemp, Display, TEXT("TABLET_WIDGET Name=%s Class=%s Variable=%s Visibility=%s"),
			*Widget->GetName(),
			*Widget->GetClass()->GetName(),
			Widget->bIsVariable ? TEXT("true") : TEXT("false"),
			*UEnum::GetValueAsString(Widget->GetVisibility()));
	}
	return true;
}

bool UTabletWidgetBlueprintLibrary::CreateTabletWidgetBlueprint()
{
	using namespace TabletDesigner;
	if (!CreateMessengerDataAssetsInternal() || !BuildMessengerWidgetBlueprints(false))
	{
		return false;
	}
	if (LoadObject<UWidgetBlueprint>(nullptr, AssetPath))
	{
		UE_LOG(LogTemp, Display, TEXT("%s already exists; preserving Designer edits."), AssetPath);
		return true;
	}

	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UBalhwajeomTabletWidget::StaticClass();
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(AssetTools.CreateAsset(
		AssetName, AssetFolder, UWidgetBlueprint::StaticClass(), Factory));
	if (!Blueprint || !Blueprint->WidgetTree)
	{
		return false;
	}
	FBuilder(Blueprint).Build();
	return SaveAndCompile(Blueprint);
}

bool UTabletWidgetBlueprintLibrary::RedesignTabletWidgetBlueprint()
{
	using namespace TabletDesigner;
	if (!CreateMessengerDataAssetsInternal() || !BuildMessengerWidgetBlueprints(true))
	{
		UE_LOG(LogTemp, Error, TEXT("Messenger widget blueprints could not be generated."));
		return false;
	}
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, AssetPath);
	if (!Blueprint || !ClearWidgetTree(Blueprint))
	{
		UE_LOG(LogTemp, Error, TEXT("Existing WBP_Tablet could not be prepared for redesign."));
		return false;
	}

	FBuilder(Blueprint).Build();
	const bool bSaved = SaveAndCompile(Blueprint);
	UE_LOG(LogTemp, Display, TEXT("TABLET_REDESIGN Result=%s Asset=%s"), bSaved ? TEXT("Success") : TEXT("Failure"), AssetPath);
	return bSaved;
}

bool UTabletWidgetBlueprintLibrary::RunTabletWidgetSmokeTest()
{
	using namespace TabletDesigner;
	UClass* WidgetClass = LoadClass<UBalhwajeomTabletWidget>(
		nullptr,
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_Tablet.WBP_Tablet_C"));
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	UBalhwajeomTabletWidget* Tablet = World && WidgetClass
		? CreateWidget<UBalhwajeomTabletWidget>(World, WidgetClass)
		: nullptr;
	if (!Tablet)
	{
		UE_LOG(LogTemp, Error, TEXT("TABLET_SMOKE failed to instantiate WBP_Tablet."));
		return false;
	}
	Tablet->InitializeForAutomatedTest();

	auto FindButton = [Tablet](const TCHAR* Name)
	{
		return Cast<UButton>(Tablet->GetWidgetFromName(Name));
	};
	auto FindBorder = [Tablet](const TCHAR* Name)
	{
		return Cast<UBorder>(Tablet->GetWidgetFromName(Name));
	};
	auto FindText = [Tablet](const TCHAR* Name)
	{
		return Cast<UTextBlock>(Tablet->GetWidgetFromName(Name));
	};
	auto FindWidget = [Tablet](const TCHAR* Name)
	{
		return Tablet->GetWidgetFromName(Name);
	};
	auto Click = [&FindButton](const TCHAR* Name)
	{
		if (UButton* Button = FindButton(Name))
		{
			Button->OnClicked.Broadcast();
			return true;
		}
		return false;
	};
	auto Require = [](const bool bCondition, const TCHAR* Description)
	{
		if (!bCondition)
		{
			UE_LOG(LogTemp, Error, TEXT("TABLET_SMOKE FAIL: %s"), Description);
		}
		return bCondition;
	};

	bool bPassed = true;
	bPassed &= Require(Tablet->HasTabletTransitionAnimation(), TEXT("TabletUpAnim binds to the native tablet widget"));
	if (const UScaleBox* TabletScale = Cast<UScaleBox>(FindWidget(TEXT("ScaleBox_Tablet"))))
	{
		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(TabletScale->Slot);
		const FAnchors Anchors = Slot ? Slot->GetAnchors() : FAnchors();
		bPassed &= Require(
			Slot && FMath::IsNearlyEqual(Anchors.Minimum.Y, 0.05f) && FMath::IsNearlyEqual(Anchors.Maximum.Y, 0.95f),
			TEXT("tablet occupies centered 90 percent viewport height"));
		bPassed &= Require(TabletScale->GetStretch() == EStretch::ScaleToFit, TEXT("tablet preserves body aspect ratio"));
	}
	else
	{
		bPassed &= Require(false, TEXT("tablet ScaleBox exists"));
	}
	if (const USizeBox* TabletSize = Cast<USizeBox>(FindWidget(TEXT("SizeBox_Tablet"))))
	{
		bPassed &= Require(
			FMath::IsNearlyEqual(TabletSize->GetWidthOverride(), 1448.0f)
			&& FMath::IsNearlyEqual(TabletSize->GetHeightOverride(), 1086.0f),
			TEXT("tablet uses measured body dimensions"));
	}
	if (const UScaleBox* LogicalScale = Cast<UScaleBox>(FindWidget(TEXT("ScaleBox_LogicalScreen"))))
	{
		bPassed &= Require(LogicalScale->GetStretch() == EStretch::ScaleToFit, TEXT("logical UI is never cropped or distorted"));
	}
	if (const UBorder* Screen = Cast<UBorder>(FindWidget(TEXT("ScreenContent"))))
	{
		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Screen->Slot);
		bPassed &= Require(
			Slot && Slot->GetPosition().Equals(FVector2D(96.0f, 123.0f))
			&& Slot->GetSize().Equals(FVector2D(1255.0f, 811.0f)),
			TEXT("screen uses measured transparent rect"));
		bPassed &= Require(Screen->GetClipping() == EWidgetClipping::ClipToBounds, TEXT("screen clips children to bezel"));
	}
	if (const UImage* Body = Cast<UImage>(FindWidget(TEXT("IMG_TabletBody"))))
	{
		bPassed &= Require(Body->GetVisibility() == ESlateVisibility::HitTestInvisible, TEXT("body bezel does not intercept clicks"));
	}
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Home, TEXT("initial page is Home"));
	bPassed &= Require(Click(TEXT("BTN_Internet")), TEXT("Internet button exists"));
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Internet, TEXT("Internet opens"));
	Tablet->SetVisibility(ESlateVisibility::Collapsed);
	Tablet->SetVisibility(ESlateVisibility::Visible);
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Internet, TEXT("page persists across close/reopen visibility"));
	bPassed &= Require(Click(TEXT("BTN_InternetBack")), TEXT("Internet back exists"));
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Home, TEXT("Back returns Home"));
	bPassed &= Require(Click(TEXT("BTN_Memo")), TEXT("Memo button exists"));
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Memo, TEXT("Memo opens"));
	bPassed &= Require(Click(TEXT("BTN_PhysicalHome")), TEXT("physical Home exists"));
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Home, TEXT("physical Home clears to Home"));

	UBalhwajeomMessengerWidget* Messenger = Tablet->GetMessengerWidget();
	bPassed &= Require(Messenger != nullptr, TEXT("WBP_Messenger is embedded in WBP_Tablet"));
	if (Messenger)
	{
		bPassed &= Require(
			Messenger->GetMessengerDataAsset() != nullptr,
			TEXT("messenger loads the planner-owned Data Asset catalog"));
		bPassed &= Require(
			Messenger->GetMessengerDataAsset()
			&& Messenger->GetMessengerDataAsset()->Rooms.Num() == 4,
			TEXT("catalog references four room Data Assets"));
		bPassed &= Require(Messenger->IsMessengerInitialized(), TEXT("messenger initializes once"));
		bPassed &= Require(Messenger->HasValidRoomData(), TEXT("room IDs, messages, and unread counts are valid"));
		bPassed &= Require(Messenger->GetDisplayedRoomCount() == 4, TEXT("four room rows are created"));
		bPassed &= Require(Messenger->GetDisplayedMessageCount() == 0, TEXT("first entry does not auto-select a room"));
		bPassed &= Require(Messenger->GetCurrentRoomID().IsEmpty(), TEXT("first entry has no current room"));
		bPassed &= Require(Messenger->GetTotalUnreadCount() == 8, TEXT("initial unread state is copied once"));
		bPassed &= Require(
			Cast<UScrollBox>(Messenger->GetWidgetFromName(TEXT("SB_ChatRoomList"))) != nullptr,
			TEXT("room list is scrollable"));
		bPassed &= Require(
			Cast<UScrollBox>(Messenger->GetWidgetFromName(TEXT("SB_MessageList"))) != nullptr,
			TEXT("message list is scrollable"));
		if (const UTextBlock* DisabledInput =
			Cast<UTextBlock>(Messenger->GetWidgetFromName(TEXT("TXT_DisabledInput"))))
		{
			bPassed &= Require(
				DisabledInput->GetText().ToString() == TEXT("현재 대화가 불가능합니다"),
				TEXT("read-only input notice is present"));
		}
		else
		{
			bPassed &= Require(false, TEXT("read-only input notice exists"));
		}

		if (const UBalhwajeomMessengerRoomWidget* MotherRoom =
			Messenger->GetDisplayedRoomWidget(TEXT("Mother")))
		{
			bPassed &= Require(
				MotherRoom->GetLastMessagePreview().ToString() == TEXT("현관 비밀번호 바뀐 거 잊지 마."),
				TEXT("room preview comes from Messages.Last"));
		}
		else
		{
			bPassed &= Require(false, TEXT("Mother room widget exists"));
		}

		Messenger->InitializeMessenger();
		bPassed &= Require(Messenger->GetDisplayedRoomCount() == 4, TEXT("initialization guard prevents duplicate rooms"));
		bPassed &= Require(Click(TEXT("BTN_Messenger")), TEXT("Messenger button exists"));
		bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Messenger, TEXT("Messenger opens"));

		if (UBalhwajeomMessengerRoomWidget* MotherRoom =
			Messenger->GetDisplayedRoomWidget(TEXT("Mother")))
		{
			if (UButton* RoomButton = Cast<UButton>(MotherRoom->GetWidgetFromName(TEXT("BTN_Room"))))
			{
				RoomButton->OnClicked.Broadcast();
			}
			else
			{
				bPassed &= Require(false, TEXT("room row button exists"));
			}
		}
		else
		{
			bPassed &= Require(false, TEXT("Mother room can be selected"));
		}
		bPassed &= Require(Messenger->GetCurrentRoomID() == TEXT("Mother"), TEXT("current room tracks RoomID"));
		bPassed &= Require(Messenger->GetDisplayedMessageCount() == 3, TEXT("Mother messages load"));
		bPassed &= Require(Messenger->GetCurrentUnreadCount(TEXT("Mother")) == 0, TEXT("selected room becomes read"));
		bPassed &= Require(Messenger->GetCurrentUnreadCount(TEXT("Sister")) == 4, TEXT("other room unread state is unchanged"));
		bPassed &= Require(Messenger->GetTotalUnreadCount() == 7, TEXT("aggregate badge follows room state"));

		const FString ValidRoomBeforeInvalidSelection = Messenger->GetCurrentRoomID();
		const int32 MessageCountBeforeInvalidSelection = Messenger->GetDisplayedMessageCount();
		bPassed &= Require(!Messenger->SelectRoomByID(TEXT("Missing")), TEXT("invalid RoomID is rejected"));
		bPassed &= Require(
			Messenger->GetCurrentRoomID() == ValidRoomBeforeInvalidSelection
			&& Messenger->GetDisplayedMessageCount() == MessageCountBeforeInvalidSelection,
			TEXT("invalid RoomID preserves the current UI"));

		bPassed &= Require(Messenger->SelectRoomByID(TEXT("Sister")), TEXT("Sister room can be selected"));
		bPassed &= Require(Messenger->GetDisplayedMessageCount() == 5, TEXT("changing rooms clears old messages"));
		bPassed &= Require(Messenger->GetCurrentUnreadCount(TEXT("Sister")) == 0, TEXT("Sister room becomes read"));
		bPassed &= Require(Messenger->GetTotalUnreadCount() == 3, TEXT("unread totals remain isolated per room"));
		if (const UBalhwajeomMessengerMessageWidget* PlayerMessage = Messenger->GetDisplayedMessageWidget(1))
		{
			bPassed &= Require(PlayerMessage->IsPlayerMessage(), TEXT("player message retains right-side alignment state"));
			bPassed &= Require(PlayerMessage->HasInteractiveKeyword(), TEXT("valid WordID keyword is interactive"));
		}
		else
		{
			bPassed &= Require(false, TEXT("player message widget exists"));
		}
		if (const UBalhwajeomMessengerMessageWidget* FamilyMessage = Messenger->GetDisplayedMessageWidget(0))
		{
			bPassed &= Require(!FamilyMessage->IsPlayerMessage(), TEXT("family message retains left-side alignment state"));
		}

		bPassed &= Require(Messenger->SelectRoomByID(TEXT("Sister")), TEXT("selected room can be clicked again"));
		bPassed &= Require(Messenger->GetCurrentUnreadCount(TEXT("Sister")) == 0, TEXT("reselecting a room is idempotent"));

		if (UButton* MessengerBack = Cast<UButton>(Messenger->GetWidgetFromName(TEXT("BTN_Back"))))
		{
			MessengerBack->OnClicked.Broadcast();
			bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::Home, TEXT("Messenger back requests Desktop"));
			bPassed &= Require(Click(TEXT("BTN_Messenger")), TEXT("Messenger can reopen"));
			bPassed &= Require(
				Messenger->GetCurrentUnreadCount(TEXT("Sister")) == 0
				&& Messenger->GetDisplayedRoomCount() == 4,
				TEXT("read state and room list persist across Messenger re-entry"));
			MessengerBack->OnClicked.Broadcast();
		}
		else
		{
			bPassed &= Require(false, TEXT("Messenger back button exists"));
		}

		UClass* MessageClass = LoadClass<UBalhwajeomMessengerMessageWidget>(
			nullptr,
			TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerMessage.WBP_MessengerMessage_C"));
		UBalhwajeomMessengerMessageWidget* InvalidKeywordMessage = MessageClass
			? CreateWidget<UBalhwajeomMessengerMessageWidget>(World, MessageClass)
			: nullptr;
		if (InvalidKeywordMessage)
		{
			FST_MessengerMessage InvalidData;
			InvalidData.SenderName = FText::FromString(TEXT("테스트"));
			InvalidData.Message = FText::FromString(TEXT("생일 선물 사준다고 했잖아"));
			InvalidData.KeywordText = FText::FromString(TEXT("스노우 글로브"));
			InvalidData.WordID = TEXT("Sister_SnowGlobe");
			InvalidKeywordMessage->SetupMessage(InvalidData);
			bPassed &= Require(
				!InvalidKeywordMessage->HasInteractiveKeyword()
				&& InvalidKeywordMessage->GetDisplayedMessage().EqualTo(InvalidData.Message),
				TEXT("invalid keyword data falls back to the full plain message"));
		}
		else
		{
			bPassed &= Require(false, TEXT("message widget can be instantiated for fallback validation"));
		}
	}

	bPassed &= Require(Click(TEXT("BTN_Sister")), TEXT("Sister folder button exists"));
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::PersonFolder, TEXT("folder page opens"));
	bPassed &= Require(Tablet->GetActiveFamilyMember() == EFamilyMember::Sister, TEXT("active member is Sister"));
	bPassed &= Require(Cast<UWrapBox>(FindWidget(TEXT("WB_EvidencePhotos"))) != nullptr,
		TEXT("dynamic evidence photo list exists"));
	bPassed &= Require(Cast<UWrapBox>(FindWidget(TEXT("WB_PuzzlePhotos"))) != nullptr,
		TEXT("dynamic puzzle photo candidate list exists"));
	if (UTextBlock* FolderTitle = FindText(TEXT("TXT_FolderTitle")))
	{
		bPassed &= Require(FolderTitle->GetText().ToString() == TEXT("여동생"), TEXT("folder title updates"));
	}
	else
	{
		bPassed &= Require(false, TEXT("folder title exists"));
	}

	Tablet->SetUnreadMessageCount(0);
	UBorder* Badge = FindBorder(TEXT("BRD_MessengerBadge"));
	UTextBlock* BadgeText = FindText(TEXT("TXT_UnreadMessageCount"));
	bPassed &= Require(Badge && Badge->GetVisibility() == ESlateVisibility::Collapsed, TEXT("zero unread hides badge"));
	Tablet->SetUnreadMessageCount(3);
	bPassed &= Require(Badge && Badge->GetVisibility() == ESlateVisibility::HitTestInvisible, TEXT("positive unread shows badge"));
	bPassed &= Require(BadgeText && BadgeText->GetText().ToString() == TEXT("3"), TEXT("badge count updates"));

	UE_LOG(LogTemp, Display, TEXT("TABLET_SMOKE Result=%s"), bPassed ? TEXT("Success") : TEXT("Failure"));
	return bPassed;
}

bool UTabletWidgetBlueprintLibrary::UpgradeInvestigationDataTables()
{
	const TCHAR* DocumentPath =
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_KeywordDocuments.DT_KeywordDocuments");
	const TCHAR* ChoicePath =
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_KeywordChoices.DT_KeywordChoices");
	const TCHAR* CharacterPath =
		TEXT("/Game/Balhwajeom/Data/Investigation/DT_Characters.DT_Characters");
	const TCHAR* AssetFolder = TEXT("/Game/Balhwajeom/Data/Investigation");

	UDataTable* Documents = LoadObject<UDataTable>(nullptr, DocumentPath);
	if (!Documents || Documents->GetRowStruct() != FKeywordDocumentDefinition::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("Investigation upgrade: DT_KeywordDocuments is missing or has the wrong row struct."));
		return false;
	}

	UDataTable* Choices = LoadObject<UDataTable>(nullptr, ChoicePath);
	if (!Choices)
	{
		UDataTableFactory* Factory = NewObject<UDataTableFactory>();
		Factory->Struct = FKeywordChoiceDefinition::StaticStruct();
		IAssetTools& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		Choices = Cast<UDataTable>(AssetTools.CreateAsset(
			TEXT("DT_KeywordChoices"), AssetFolder, UDataTable::StaticClass(), Factory));
	}
	if (!Choices || Choices->GetRowStruct() != FKeywordChoiceDefinition::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("Investigation upgrade: could not create DT_KeywordChoices."));
		return false;
	}

	UDataTable* Characters = LoadObject<UDataTable>(nullptr, CharacterPath);
	if (!Characters)
	{
		UDataTableFactory* Factory = NewObject<UDataTableFactory>();
		Factory->Struct = FCharacterDefinition::StaticStruct();
		IAssetTools& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		Characters = Cast<UDataTable>(AssetTools.CreateAsset(
			TEXT("DT_Characters"), AssetFolder, UDataTable::StaticClass(), Factory));
	}
	if (!Characters || Characters->GetRowStruct() != FCharacterDefinition::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("Investigation upgrade: could not create DT_Characters."));
		return false;
	}

	UDataTable* EvidenceDefinitions = LoadObject<UDataTable>(
		nullptr, TEXT("/Game/Balhwajeom/Data/Investigation/DT_EvidenceDefinitions.DT_EvidenceDefinitions"));
	UDataTable* EvidenceStates = LoadObject<UDataTable>(
		nullptr, TEXT("/Game/Balhwajeom/Data/Investigation/DT_EvidenceStates.DT_EvidenceStates"));
	UDataTable* Words = LoadObject<UDataTable>(
		nullptr, TEXT("/Game/Balhwajeom/Data/Investigation/DT_Words.DT_Words"));
	UDataTable* Photos = LoadObject<UDataTable>(
		nullptr, TEXT("/Game/Balhwajeom/Data/Investigation/DT_Photos.DT_Photos"));
	UDataTable* Sentences = LoadObject<UDataTable>(
		nullptr, TEXT("/Game/Balhwajeom/Data/Investigation/DT_Sentences.DT_Sentences"));
	if (!EvidenceDefinitions || EvidenceDefinitions->GetRowStruct() != FEvidenceDefinition::StaticStruct() ||
		!EvidenceStates || EvidenceStates->GetRowStruct() != FEvidenceStateDefinition::StaticStruct() ||
		!Words || Words->GetRowStruct() != FWordDefinition::StaticStruct() ||
		!Photos || Photos->GetRowStruct() != FPhotoDefinition::StaticStruct() ||
		!Sentences || Sentences->GetRowStruct() != FSentenceDefinition::StaticStruct())
	{
		UE_LOG(LogTemp, Error, TEXT("Investigation upgrade: one or more core DataTables have the wrong row struct."));
		return false;
	}

	int32 MigratedCount = 0;
	for (const TPair<FName, uint8*>& Pair : Documents->GetRowMap())
	{
		FKeywordDocumentDefinition* Document =
			reinterpret_cast<FKeywordDocumentDefinition*>(Pair.Value);
		for (int32 Index = 0; Index < Document->KeywordChoices.Num(); ++Index)
		{
			FKeywordChoiceDefinition Choice = Document->KeywordChoices[Index];
			Choice.KeywordDocumentID = Document->KeywordDocumentID;
			Choice.SortOrder = Index;
			if (Choice.ChoiceID.IsNone())
			{
				Choice.ChoiceID = FName(*FString::Printf(
					TEXT("%s_CHOICE_%02d"), *Document->KeywordDocumentID.ToString(), Index + 1));
			}
			if (!Choices->GetRowMap().Contains(Choice.ChoiceID))
			{
				Choices->AddRow(Choice.ChoiceID, Choice);
				++MigratedCount;
			}
		}
		Document->KeywordChoices.Reset();
		Document->bCloseAfterSelection = false;
	}

	int32 SeededRows = 0;
	const FName SisterCharacterID(TEXT("SISTER"));
	if (!Characters->GetRowMap().Contains(SisterCharacterID))
	{
		FCharacterDefinition Sister;
		Sister.CharacterID = SisterCharacterID;
		Sister.FolderName = FText::FromString(TEXT("여동생"));
		Sister.FolderSortOrder = 0;
		Characters->AddRow(Sister.CharacterID, Sister);
		++SeededRows;
	}
	if (Characters->GetRowMap().Num() == 1)
	{
		const FName SoleCharacterID = Characters->GetRowMap().CreateConstIterator().Key();
		for (const TPair<FName, uint8*>& Pair : Photos->GetRowMap())
		{
			FPhotoDefinition* Photo = reinterpret_cast<FPhotoDefinition*>(Pair.Value);
			if (Photo->CharacterID.IsNone())
			{
				Photo->CharacterID = SoleCharacterID;
				++SeededRows;
			}
		}
	}

	if (EvidenceDefinitions->GetRowMap().IsEmpty() && EvidenceStates->GetRowMap().IsEmpty() &&
		Words->GetRowMap().IsEmpty() && Photos->GetRowMap().IsEmpty() &&
		Documents->GetRowMap().IsEmpty() && Choices->GetRowMap().IsEmpty() && Sentences->GetRowMap().IsEmpty())
	{
		auto AddWord = [Words, &SeededRows](const TCHAR* ID, const TCHAR* Display, const TCHAR* Description)
		{
			FWordDefinition Row;
			Row.WordID = ID;
			Row.DisplayWord = FText::FromString(Display);
			Row.Description = FText::FromString(Description);
			Words->AddRow(Row.WordID, Row);
			++SeededRows;
		};
		AddWord(TEXT("WORD_PIG"), TEXT("돼지"), TEXT("거울 앞에 놓여 있던 작은 돼지 장식."));
		AddWord(TEXT("WORD_MIRROR"), TEXT("거울"), TEXT("불에 그을렸지만 반사면 일부가 남아 있다."));
		AddWord(TEXT("WORD_SNOW_GLOBE"), TEXT("스노우글로브"), TEXT("가족이 생일 선물로 준비했던 장식품."));

		FKeywordDocumentDefinition Document;
		Document.KeywordDocumentID = TEXT("DOC_MIRROR_LABEL");
		Document.DocumentText = FText::FromString(
			TEXT("그을린 라벨 아래로 ‘거울’이라는 글자가 희미하게 남아 있다."));
		Documents->AddRow(Document.KeywordDocumentID, Document);
		++SeededRows;

		FKeywordChoiceDefinition Choice;
		Choice.ChoiceID = TEXT("CHOICE_MIRROR");
		Choice.KeywordDocumentID = Document.KeywordDocumentID;
		Choice.DisplayText = FText::FromString(TEXT("거울"));
		Choice.GrantedWordID = TEXT("WORD_MIRROR");
		Choice.SortOrder = 0;
		Choices->AddRow(Choice.ChoiceID, Choice);
		++SeededRows;

		FSentenceDefinition Analysis;
		Analysis.SentenceID = TEXT("SENT_PHOTO_PIG_MIRROR");
		Analysis.SentenceType = ESentenceType::PhotoAnalysis;
		Analysis.SentenceTemplate = FText::FromString(TEXT("[   ]가 바라보던 것은 [   ]이었다."));
		Analysis.ResultText = FText::FromString(TEXT("돼지가 바라보던 것은 거울이었다."));
		Analysis.DesignerNote = FText::FromString(TEXT("촬영으로 돼지, F 조사로 거울 키워드를 획득한다."));
		Analysis.WordSlots.Add({0, TEXT("WORD_PIG")});
		Analysis.WordSlots.Add({1, TEXT("WORD_MIRROR")});
		Sentences->AddRow(Analysis.SentenceID, Analysis);
		++SeededRows;

		FSentenceDefinition Statement;
		Statement.SentenceID = TEXT("SENT_STATEMENT_SISTER_01");
		Statement.CharacterID = SisterCharacterID;
		Statement.LieText = FText::FromString(TEXT("나는 돼지 장식이 놓인 거울을 본 적이 없어."));
		Statement.SentenceType = ESentenceType::Statement;
		Statement.SentenceTemplate = FText::FromString(TEXT("현장에 남은 [   ]이 그 말을 반박한다."));
		Statement.ResultText = FText::FromString(TEXT("현장에 남은 거울과 완성된 사진이 그 말을 반박한다."));
		Statement.RequiredPhotoCount = 1;
		Statement.WordSlots.Add({0, TEXT("WORD_MIRROR")});
		Statement.PhotoSlots.Add({0, TEXT("PHOTO_PIG_MIRROR")});
		Sentences->AddRow(Statement.SentenceID, Statement);
		++SeededRows;

		FPhotoDefinition MirrorPhoto;
		MirrorPhoto.PhotoID = TEXT("PHOTO_PIG_MIRROR");
		MirrorPhoto.PhotoName = FText::FromString(TEXT("돼지가 보던 거울"));
		MirrorPhoto.DescriptionSource = EPhotoDescriptionSource::ObservationText;
		MirrorPhoto.PhotoSentenceID = Analysis.SentenceID;
		MirrorPhoto.CharacterID = SisterCharacterID;
		MirrorPhoto.GrantedWordIDs.Add(TEXT("WORD_PIG"));
		MirrorPhoto.WorldStoryLines.Add(FText::FromString(TEXT("불탄 거울 속에 돼지 장식의 실루엣이 남아 있다.")));
		Photos->AddRow(MirrorPhoto.PhotoID, MirrorPhoto);
		++SeededRows;

		FPhotoDefinition StoryPhoto;
		StoryPhoto.PhotoID = TEXT("PHOTO_SNOW_GLOBE");
		StoryPhoto.PhotoName = FText::FromString(TEXT("가족의 스노우글로브"));
		StoryPhoto.DescriptionSource = EPhotoDescriptionSource::Custom;
		StoryPhoto.CustomDescription = FText::FromString(TEXT("그을린 유리 안에서 작은 눈송이가 흔들린다."));
		StoryPhoto.CharacterID = SisterCharacterID;
		StoryPhoto.GrantedWordIDs.Add(TEXT("WORD_SNOW_GLOBE"));
		StoryPhoto.WorldStoryLines.Add(FText::FromString(TEXT("가족의 대화가 잠시 귓가에 되살아난다.")));
		Photos->AddRow(StoryPhoto.PhotoID, StoryPhoto);
		++SeededRows;

		FEvidenceDefinition MirrorObject;
		MirrorObject.ObjectID = TEXT("OBJ_PIG_MIRROR");
		MirrorObject.ObjectName = FText::FromString(TEXT("돼지 장식과 거울"));
		MirrorObject.InitialStateID = TEXT("STATE_PIG_MIRROR");
		EvidenceDefinitions->AddRow(MirrorObject.ObjectID, MirrorObject);
		++SeededRows;

		FEvidenceStateDefinition MirrorState;
		MirrorState.StateID = MirrorObject.InitialStateID;
		MirrorState.ObjectID = MirrorObject.ObjectID;
		MirrorState.StateName = FText::FromString(TEXT("그을린 상태"));
		MirrorState.InteractionBehavior = EEvidenceInteractionBehavior::Once;
		MirrorState.InteractionPresentation = EEvidenceInteractionPresentation::KeywordSelectionWindow;
		MirrorState.KeywordDocumentID = Document.KeywordDocumentID;
		MirrorState.MidLabel = FText::FromString(TEXT("그을린 거울이 있다"));
		MirrorState.ObservationText = FText::FromString(TEXT("돼지 장식이 거울을 향해 놓여 있다."));
		MirrorState.bCanCapture = true;
		MirrorState.PhotoID = MirrorPhoto.PhotoID;
		EvidenceStates->AddRow(MirrorState.StateID, MirrorState);
		++SeededRows;

		FEvidenceDefinition SnowGlobeObject;
		SnowGlobeObject.ObjectID = TEXT("OBJ_SNOW_GLOBE");
		SnowGlobeObject.ObjectName = FText::FromString(TEXT("스노우글로브"));
		SnowGlobeObject.InitialStateID = TEXT("STATE_SNOW_GLOBE");
		EvidenceDefinitions->AddRow(SnowGlobeObject.ObjectID, SnowGlobeObject);
		++SeededRows;

		FEvidenceStateDefinition SnowGlobeState;
		SnowGlobeState.StateID = SnowGlobeObject.InitialStateID;
		SnowGlobeState.ObjectID = SnowGlobeObject.ObjectID;
		SnowGlobeState.StateName = FText::FromString(TEXT("발견 상태"));
		SnowGlobeState.InteractionBehavior = EEvidenceInteractionBehavior::Repeatable;
		SnowGlobeState.InteractionPresentation = EEvidenceInteractionPresentation::SimpleText;
		SnowGlobeState.InteractionText = FText::FromString(TEXT("가족이 주고받던 생일 선물이다."));
		SnowGlobeState.MidLabel = FText::FromString(TEXT("유리 장식품이 있다"));
		SnowGlobeState.ObservationText = FText::FromString(TEXT("불길을 견딘 스노우글로브가 놓여 있다."));
		SnowGlobeState.bCanCapture = true;
		SnowGlobeState.PhotoID = StoryPhoto.PhotoID;
		EvidenceStates->AddRow(SnowGlobeState.StateID, SnowGlobeState);
		++SeededRows;
	}

	const auto SaveTable = [](UDataTable* Table)
	{
		UPackage* Package = Table->GetOutermost();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.bSlowTask = false;
		return UPackage::SavePackage(Package, Table, *Filename, SaveArgs);
	};

	const bool bSaved = SaveTable(EvidenceDefinitions) && SaveTable(EvidenceStates) &&
		SaveTable(Words) && SaveTable(Photos) && SaveTable(Documents) &&
		SaveTable(Choices) && SaveTable(Sentences) && SaveTable(Characters);
	if (bSaved)
	{
		UE_LOG(LogTemp, Display, TEXT("INVESTIGATION_UPGRADE Result=Success MigratedChoices=%d SeededRows=%d"), MigratedCount, SeededRows);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("INVESTIGATION_UPGRADE Result=Failure MigratedChoices=%d SeededRows=%d"), MigratedCount, SeededRows);
	}
	return bSaved;
}
