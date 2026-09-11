#include "Tablet/BalhwajeomInternetWidget.h"

#include "Balhwajeom.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Tablet/BalhwajeomInternetPageWidget.h"
#include "Tablet/BalhwajeomInternetTabWidget.h"

namespace
{
	TSoftClassPtr<UBalhwajeomInternetPageWidget> PageClass(const TCHAR* Path)
	{
		return TSoftClassPtr<UBalhwajeomInternetPageWidget>(FSoftObjectPath(Path));
	}
}

UBalhwajeomInternetWidget::UBalhwajeomInternetWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PageWidgetClasses.Add(
		EBalhwajeomInternetPage::Main,
		PageClass(TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Main.WBP_InternetPage_Main_C")));
	PageWidgetClasses.Add(
		EBalhwajeomInternetPage::Weather,
		PageClass(TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Weather.WBP_InternetPage_Weather_C")));
	PageWidgetClasses.Add(
		EBalhwajeomInternetPage::News1,
		PageClass(TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News1.WBP_InternetPage_News1_C")));
	PageWidgetClasses.Add(
		EBalhwajeomInternetPage::News2,
		PageClass(TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_News2.WBP_InternetPage_News2_C")));
	PageWidgetClasses.Add(
		EBalhwajeomInternetPage::Ad,
		PageClass(TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetPage_Ad.WBP_InternetPage_Ad_C")));
	TabWidgetClass = TSoftClassPtr<UBalhwajeomInternetTabWidget>(FSoftObjectPath(
		TEXT("/Game/Balhwajeom/UI/Tablet/Internet/WBP_InternetTab.WBP_InternetTab_C")));
}

void UBalhwajeomInternetWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Maximize)
	{
		BTN_Maximize->SetIsEnabled(false);
		BTN_Maximize->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (BTN_Close)
	{
		BTN_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
	InitializeInternet();
}

void UBalhwajeomInternetWidget::NativeDestruct()
{
	SaveActiveScrollOffset();
	bDraggingWindow = false;
	Super::NativeDestruct();
}

void UBalhwajeomInternetWidget::InitializeInternet()
{
	if (!bSessionInitialized)
	{
		SessionState.Reset();
		bSessionInitialized = true;
	}

	if (GetWorld() && WS_PageContent)
	{
		for (const EBalhwajeomInternetPage Page : SessionState.GetOpenPages())
		{
			EnsurePageWidget(Page);
		}
		RebuildTabs();
		ShowActivePage();
	}
	ApplyWindowGeometry();
	RefreshMaximizeLabel();
}

bool UBalhwajeomInternetWidget::OpenPage(const EBalhwajeomInternetPage Page)
{
	if (!IsValidInternetPage(Page))
	{
		return false;
	}
	if (GetWorld() && WS_PageContent && !ResolvePageClass(Page))
	{
		UE_LOG(LogBalhwajeom, Error, TEXT("Internet page class is missing for %s."), *UEnum::GetValueAsString(Page));
		return false;
	}

	SaveActiveScrollOffset();
	const bool bAdded = SessionState.OpenPage(Page);
	if (GetWorld() && WS_PageContent && !EnsurePageWidget(Page))
	{
		if (bAdded)
		{
			SessionState.ClosePage(Page);
		}
		return false;
	}
	RebuildTabs();
	ShowActivePage();
	return true;
}

bool UBalhwajeomInternetWidget::ActivatePage(const EBalhwajeomInternetPage Page)
{
	SaveActiveScrollOffset();
	if (!SessionState.ActivatePage(Page))
	{
		return false;
	}
	RebuildTabs();
	ShowActivePage();
	return true;
}

bool UBalhwajeomInternetWidget::ClosePage(const EBalhwajeomInternetPage Page)
{
	SaveActiveScrollOffset();
	if (!SessionState.ClosePage(Page))
	{
		return false;
	}

	if (TObjectPtr<UBalhwajeomInternetPageWidget>* PageWidget = PageWidgets.Find(Page))
	{
		if (*PageWidget)
		{
			(*PageWidget)->RemoveFromParent();
		}
	}
	PageWidgets.Remove(Page);
	RebuildTabs();
	ShowActivePage();
	return true;
}

void UBalhwajeomInternetWidget::ToggleMaximize()
{
	SessionState.SetMaximized(!SessionState.IsMaximized());
	bDraggingWindow = false;
	ApplyWindowGeometry();
	RefreshMaximizeLabel();
}

void UBalhwajeomInternetWidget::PrepareForDesktopOpen()
{
	SessionState.SetMaximized(false);
	bDraggingWindow = false;
	ApplyWindowGeometry();
	RefreshMaximizeLabel();
	ShowActivePage();
}

void UBalhwajeomInternetWidget::CloseInternetWindow()
{
	SaveActiveScrollOffset();
	SessionState.SetMaximized(false);
	bDraggingWindow = false;
	ApplyWindowGeometry();
	RefreshMaximizeLabel();
	OnCloseRequested.Broadcast();
}

void UBalhwajeomInternetWidget::SetNormalWindowPosition(const FVector2D Position)
{
	SessionState.SetNormalWindowPosition(Position);
	if (!SessionState.IsMaximized())
	{
		ApplyWindowGeometry();
	}
}

TSubclassOf<UBalhwajeomInternetPageWidget> UBalhwajeomInternetWidget::ResolvePageClass(
	const EBalhwajeomInternetPage Page) const
{
	if (const TSoftClassPtr<UBalhwajeomInternetPageWidget>* Class = PageWidgetClasses.Find(Page))
	{
		return Class->LoadSynchronous();
	}
	return nullptr;
}

TSubclassOf<UBalhwajeomInternetTabWidget> UBalhwajeomInternetWidget::ResolveTabClass() const
{
	return TabWidgetClass.LoadSynchronous();
}

UBalhwajeomInternetPageWidget* UBalhwajeomInternetWidget::EnsurePageWidget(
	const EBalhwajeomInternetPage Page)
{
	if (const TObjectPtr<UBalhwajeomInternetPageWidget>* Existing = PageWidgets.Find(Page))
	{
		return *Existing;
	}
	if (!GetWorld() || !WS_PageContent)
	{
		return nullptr;
	}

	const TSubclassOf<UBalhwajeomInternetPageWidget> Class = ResolvePageClass(Page);
	UBalhwajeomInternetPageWidget* PageWidget = Class
		? CreateWidget<UBalhwajeomInternetPageWidget>(GetWorld(), Class)
		: nullptr;
	if (!PageWidget)
	{
		UE_LOG(LogBalhwajeom, Error, TEXT("Internet page could not be created: %s"), *UEnum::GetValueAsString(Page));
		return nullptr;
	}

#if WITH_EDITOR
	if (!GetOwningLocalPlayer())
	{
		PageWidget->InitializeForAutomatedTest();
	}
#endif
	PageWidget->SetupPage(Page);
	PageWidget->SetSavedScrollOffset(SessionState.GetScrollOffset(Page));
	PageWidget->OnPageLinkRequested.AddUniqueDynamic(this, &ThisClass::HandlePageLinkRequested);
	WS_PageContent->AddChild(PageWidget);
	PageWidgets.Add(Page, PageWidget);
	return PageWidget;
}

void UBalhwajeomInternetWidget::SaveActiveScrollOffset()
{
	if (const TObjectPtr<UBalhwajeomInternetPageWidget>* PageWidget =
		PageWidgets.Find(SessionState.GetActivePage()))
	{
		if (*PageWidget)
		{
			SessionState.SetScrollOffset(
				SessionState.GetActivePage(),
				(*PageWidget)->GetSavedScrollOffset());
		}
	}
}

void UBalhwajeomInternetWidget::ShowActivePage()
{
	if (!WS_PageContent)
	{
		return;
	}
	if (UBalhwajeomInternetPageWidget* PageWidget = EnsurePageWidget(SessionState.GetActivePage()))
	{
		PageWidget->SetSavedScrollOffset(SessionState.GetScrollOffset(SessionState.GetActivePage()));
		WS_PageContent->SetActiveWidget(PageWidget);
	}
}

void UBalhwajeomInternetWidget::RebuildTabs()
{
	if (!HB_TabBar || !GetWorld())
	{
		return;
	}

	HB_TabBar->ClearChildren();
	const TSubclassOf<UBalhwajeomInternetTabWidget> Class = ResolveTabClass();
	if (!Class)
	{
		UE_LOG(LogBalhwajeom, Error, TEXT("Internet tab class could not be loaded."));
		return;
	}

	for (const EBalhwajeomInternetPage Page : SessionState.GetOpenPages())
	{
		UBalhwajeomInternetTabWidget* Tab =
			CreateWidget<UBalhwajeomInternetTabWidget>(GetWorld(), Class);
		if (!Tab)
		{
			continue;
		}
#if WITH_EDITOR
		if (!GetOwningLocalPlayer())
		{
			Tab->InitializeForAutomatedTest();
		}
#endif
		Tab->SetupTab(Page, Page == SessionState.GetActivePage());
		Tab->OnTabSelected.AddUniqueDynamic(this, &ThisClass::HandleTabSelected);
		Tab->OnTabCloseRequested.AddUniqueDynamic(this, &ThisClass::HandleTabCloseRequested);
		HB_TabBar->AddChild(Tab);
	}
}

void UBalhwajeomInternetWidget::ApplyWindowGeometry()
{
	if (!SizeBox_BrowserWindow)
	{
		return;
	}
	UCanvasPanelSlot* WindowSlot = Cast<UCanvasPanelSlot>(SizeBox_BrowserWindow->Slot);
	if (!WindowSlot)
	{
		return;
	}

	WindowSlot->SetAnchors(FAnchors(0.0f));
	WindowSlot->SetAlignment(FVector2D::ZeroVector);
	WindowSlot->SetAutoSize(false);
	WindowSlot->SetPosition(FVector2D::ZeroVector);
	WindowSlot->SetSize(FVector2D(
		FBalhwajeomInternetSessionState::TabletWidth,
		FBalhwajeomInternetSessionState::TabletHeight));
	SizeBox_BrowserWindow->SetWidthOverride(FBalhwajeomInternetSessionState::TabletWidth);
	SizeBox_BrowserWindow->SetHeightOverride(FBalhwajeomInternetSessionState::TabletHeight);
}

void UBalhwajeomInternetWidget::RefreshMaximizeLabel()
{
	if (TXT_Maximize)
	{
		TXT_Maximize->SetText(FText::FromString(SessionState.IsMaximized() ? TEXT("❐") : TEXT("□")));
	}
}

void UBalhwajeomInternetWidget::HandlePageLinkRequested(const EBalhwajeomInternetPage Page)
{
	OpenPage(Page);
}

void UBalhwajeomInternetWidget::HandleTabSelected(const EBalhwajeomInternetPage Page)
{
	ActivatePage(Page);
}

void UBalhwajeomInternetWidget::HandleTabCloseRequested(const EBalhwajeomInternetPage Page)
{
	ClosePage(Page);
}

void UBalhwajeomInternetWidget::HandleMaximizeClicked()
{
	ToggleMaximize();
}

void UBalhwajeomInternetWidget::HandleCloseClicked()
{
	CloseInternetWindow();
}

FReply UBalhwajeomInternetWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UBalhwajeomInternetWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UBalhwajeomInternetWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bDraggingWindow && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDraggingWindow = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UBalhwajeomInternetWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDraggingWindow = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}
