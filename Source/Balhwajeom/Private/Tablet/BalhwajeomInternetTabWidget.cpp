#include "Tablet/BalhwajeomInternetTabWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UBalhwajeomInternetTabWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Tab)
	{
		BTN_Tab->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTabClicked);
	}
	if (BTN_CloseTab)
	{
		BTN_CloseTab->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
	RefreshVisualState();
}

void UBalhwajeomInternetTabWidget::SetupTab(
	const EBalhwajeomInternetPage InPageID,
	const bool bInSelected)
{
	if (!IsValidInternetPage(InPageID))
	{
		return;
	}
	PageID = InPageID;
	bCloseable = IsInternetPageCloseable(PageID);
	bSelected = bInSelected;
	RefreshVisualState();
}

void UBalhwajeomInternetTabWidget::SetSelected(const bool bInSelected)
{
	bSelected = bInSelected;
	RefreshVisualState();
}

void UBalhwajeomInternetTabWidget::RefreshVisualState()
{
	if (TXT_TabTitle)
	{
		TXT_TabTitle->SetText(InternetPageTitle(PageID));
	}
	if (BRD_Selected)
	{
		BRD_Selected->SetVisibility(
			bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (BTN_CloseTab)
	{
		BTN_CloseTab->SetVisibility(
			bCloseable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomInternetTabWidget::HandleTabClicked()
{
	OnTabSelected.Broadcast(PageID);
}

void UBalhwajeomInternetTabWidget::HandleCloseClicked()
{
	if (bCloseable)
	{
		OnTabCloseRequested.Broadcast(PageID);
	}
}
