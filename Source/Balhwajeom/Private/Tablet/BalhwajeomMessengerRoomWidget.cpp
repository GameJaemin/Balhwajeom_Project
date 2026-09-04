#include "Tablet/BalhwajeomMessengerRoomWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UBalhwajeomMessengerRoomWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Room)
	{
		BTN_Room->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRoomButtonClicked);
	}
	RefreshVisualState();
}

void UBalhwajeomMessengerRoomWidget::SetupRoom(
	const FString& InRoomID,
	const FText& InRoomName,
	const FText& InLastMessagePreview,
	const int32 InUnreadCount,
	const bool bInIsSelected)
{
	RoomID = InRoomID;
	RoomName = InRoomName;
	LastMessagePreview = InLastMessagePreview;
	UnreadCount = FMath::Max(0, InUnreadCount);
	bIsSelected = bInIsSelected;
	RefreshVisualState();
}

void UBalhwajeomMessengerRoomWidget::SetUnreadCount(const int32 InUnreadCount)
{
	UnreadCount = FMath::Max(0, InUnreadCount);
	RefreshVisualState();
}

void UBalhwajeomMessengerRoomWidget::SetSelected(const bool bInIsSelected)
{
	bIsSelected = bInIsSelected;
	RefreshVisualState();
}

void UBalhwajeomMessengerRoomWidget::RefreshVisualState()
{
	if (TXT_RoomName)
	{
		TXT_RoomName->SetText(RoomName);
	}
	if (TXT_LastMessage)
	{
		TXT_LastMessage->SetText(LastMessagePreview);
	}
	if (TXT_UnreadCount)
	{
		TXT_UnreadCount->SetText(FText::AsNumber(UnreadCount));
	}
	if (BRD_UnreadBadge)
	{
		BRD_UnreadBadge->SetVisibility(
			UnreadCount > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (BRD_Selected)
	{
		BRD_Selected->SetVisibility(
			bIsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UBalhwajeomMessengerRoomWidget::HandleRoomButtonClicked()
{
	if (!RoomID.IsEmpty())
	{
		OnRoomClicked.Broadcast(RoomID);
	}
}
