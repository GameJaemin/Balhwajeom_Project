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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
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
			Place(Records, MakeTextButton(TEXT("BTN_EvidencePhoto01"), TEXT("▣  증거 사진"), 26), 18, 76, 280, 110);
			Place(Records, MakeTextButton(TEXT("BTN_EvidencePhoto02"), TEXT("▣  추가 사진"), 26), 330, 76, 280, 110);
			Place(Records, MakeTextButton(TEXT("BTN_EvidenceStatement"), TEXT("▤  진술서"), 26), 642, 76, 280, 110);
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
			PopupSize->SetHeightOverride(560.0f);
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
			Place(Canvas, Preview, 20, 95, 740, 300);
			UTextBlock* Body = MakeText(
				TEXT("TXT_PopupBody"), TEXT("증거 Placeholder"), 22, WarmMuted, true);
			Body->SetJustification(ETextJustify::Center);
			Body->SetAutoWrapText(true);
			Place(Canvas, Body, 40, 420, 700, 70);
			Popup->SetVisibility(ESlateVisibility::Collapsed);
			return Popup;
		}

		void BuildLogicalScreen(UCanvasPanel* LogicalScreen) const
		{
			UWidgetSwitcher* Switcher = Make<UWidgetSwitcher>(TEXT("WidgetSwitcher_TabletPage"), true);
			Switcher->AddChild(BuildHomePage());
			Switcher->AddChild(BuildPersonFolderPage());
			Switcher->AddChild(BuildAppPage(
				TEXT("Page_Messenger"), TEXT("BTN_MessengerBack"), TEXT("메신저"),
				TEXT("아직 연결된 메시지가 없습니다."), MessengerPath, TEXT("IMG_MessengerPage")));
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
	bPassed &= Require(Click(TEXT("BTN_Sister")), TEXT("Sister folder button exists"));
	bPassed &= Require(Tablet->GetCurrentPage() == ETabletPage::PersonFolder, TEXT("folder page opens"));
	bPassed &= Require(Tablet->GetActiveFamilyMember() == EFamilyMember::Sister, TEXT("active member is Sister"));
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
