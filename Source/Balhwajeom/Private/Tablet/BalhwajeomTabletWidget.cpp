#include "Tablet/BalhwajeomTabletWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Animation/WidgetAnimation.h"

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
	if (BTN_MessengerBack)
	{
		BTN_MessengerBack->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
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
}

void UBalhwajeomTabletWidget::ShowPopup(const FText& Title, const FText& Body)
{
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
	SetTabletPage(ETabletPage::Messenger);
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
	ShowPopup(
		FText::FromString(TEXT("증거 사진")),
		FText::FromString(TEXT("가족사진 증거 Placeholder")));
}

void UBalhwajeomTabletWidget::HandlePhoto02Clicked()
{
	ShowPopup(
		FText::FromString(TEXT("추가 사진")),
		FText::FromString(TEXT("추가 증거 Placeholder")));
}

void UBalhwajeomTabletWidget::HandleStatementClicked()
{
	ShowPopup(
		FText::FromString(TEXT("진술서")),
		FText::FromString(TEXT("그 시간에는 집에 있었습니다.\n\n현재는 임시 진술 내용입니다.")));
}

void UBalhwajeomTabletWidget::HandlePopupCloseClicked()
{
	HidePopup();
}
