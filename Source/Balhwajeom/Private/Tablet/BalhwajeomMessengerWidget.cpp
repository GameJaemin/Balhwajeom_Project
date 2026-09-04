#include "Tablet/BalhwajeomMessengerWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Tablet/BalhwajeomMessengerDataAssets.h"
#include "Tablet/BalhwajeomMessengerMessageWidget.h"
#include "Tablet/BalhwajeomMessengerRoomWidget.h"

namespace
{
	const TCHAR* RoomWidgetClassPath =
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerRoom.WBP_MessengerRoom_C");
	const TCHAR* MessageWidgetClassPath =
		TEXT("/Game/Balhwajeom/UI/Tablet/WBP_MessengerMessage.WBP_MessengerMessage_C");
	const TCHAR* MessengerCatalogPath =
		TEXT("/Game/Balhwajeom/Data/Messenger/DA_MessengerCatalog.DA_MessengerCatalog");
}

UBalhwajeomMessengerWidget::UBalhwajeomMessengerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MessengerData = TSoftObjectPtr<UBalhwajeomMessengerCatalogDataAsset>(
		FSoftObjectPath(MessengerCatalogPath));
}

void UBalhwajeomMessengerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Back)
	{
		BTN_Back->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
	InitializeMessenger();
}

void UBalhwajeomMessengerWidget::InitializeMessenger()
{
	if (bInitialized)
	{
		return;
	}

	CurrentUnreadCounts.Reset();
	ValidRoomIDs.Reset();
	CurrentRoomID.Reset();
	LoadedMessengerData = MessengerData.LoadSynchronous();
	if (!LoadedMessengerData)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Messenger initialization failed: catalog '%s' could not be loaded."),
			*MessengerData.ToSoftObjectPath().ToString());
		bInitialized = true;
		CreateRoomList();
		BroadcastUnreadCount();
		return;
	}

	TSet<FString> SeenRoomIDs;
	for (const UBalhwajeomMessengerRoomDataAsset* Room : LoadedMessengerData->Rooms)
	{
		if (!Room)
		{
			UE_LOG(LogTemp, Error, TEXT("Messenger room ignored: catalog contains an empty asset reference."));
			continue;
		}
		if (Room->RoomID.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Messenger room ignored: RoomID is empty."));
			continue;
		}
		if (SeenRoomIDs.Contains(Room->RoomID))
		{
			UE_LOG(LogTemp, Error, TEXT("Messenger room ignored: duplicate RoomID '%s'."), *Room->RoomID);
			ValidRoomIDs.Remove(Room->RoomID);
			CurrentUnreadCounts.Remove(Room->RoomID);
			continue;
		}

		SeenRoomIDs.Add(Room->RoomID);
		if (Room->Messages.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Messenger room ignored: '%s' has no archived messages."), *Room->RoomID);
			continue;
		}

		if (Room->InitialUnreadCount < 0 || Room->InitialUnreadCount > Room->Messages.Num())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Messenger room '%s' has invalid InitialUnreadCount %d for %d messages; clamping runtime state."),
				*Room->RoomID,
				Room->InitialUnreadCount,
				Room->Messages.Num());
		}

		ValidRoomIDs.Add(Room->RoomID);
		CurrentUnreadCounts.Add(
			Room->RoomID,
			FMath::Clamp(Room->InitialUnreadCount, 0, Room->Messages.Num()));
	}

	bInitialized = true;
	CreateRoomList();
	if (SB_MessageList)
	{
		SB_MessageList->ClearChildren();
	}
	if (TXT_SelectRoomPrompt)
	{
		TXT_SelectRoomPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (TXT_CurrentRoomName)
	{
		TXT_CurrentRoomName->SetText(FText::GetEmpty());
	}
	BroadcastUnreadCount();
}

bool UBalhwajeomMessengerWidget::SelectRoomByID(const FString& RoomID)
{
	const UBalhwajeomMessengerRoomDataAsset* Room = FindUniqueRoom(RoomID);
	if (!Room)
	{
		UE_LOG(LogTemp, Warning, TEXT("Messenger selection ignored: RoomID '%s' was not found or is ambiguous."), *RoomID);
		return false;
	}

	CurrentRoomID = RoomID;
	UpdateRoomSelection();
	LoadMessages(*Room);

	CurrentUnreadCounts.FindOrAdd(RoomID) = 0;
	if (TObjectPtr<UBalhwajeomMessengerRoomWidget>* RoomWidget = RoomWidgets.Find(RoomID))
	{
		if (*RoomWidget)
		{
			(*RoomWidget)->SetUnreadCount(0);
		}
	}
	BroadcastUnreadCount();
	return true;
}

int32 UBalhwajeomMessengerWidget::GetCurrentUnreadCount(const FString& RoomID) const
{
	if (const int32* Count = CurrentUnreadCounts.Find(RoomID))
	{
		return *Count;
	}
	return 0;
}

int32 UBalhwajeomMessengerWidget::GetTotalUnreadCount() const
{
	int32 Total = 0;
	for (const TPair<FString, int32>& Pair : CurrentUnreadCounts)
	{
		Total += Pair.Value;
	}
	return Total;
}

int32 UBalhwajeomMessengerWidget::GetDisplayedRoomCount() const
{
	return SB_ChatRoomList ? SB_ChatRoomList->GetChildrenCount() : 0;
}

int32 UBalhwajeomMessengerWidget::GetDisplayedMessageCount() const
{
	return SB_MessageList ? SB_MessageList->GetChildrenCount() : 0;
}

UBalhwajeomMessengerRoomWidget* UBalhwajeomMessengerWidget::GetDisplayedRoomWidget(
	const FString& RoomID) const
{
	if (const TObjectPtr<UBalhwajeomMessengerRoomWidget>* RoomWidget = RoomWidgets.Find(RoomID))
	{
		return *RoomWidget;
	}
	return nullptr;
}

UBalhwajeomMessengerMessageWidget* UBalhwajeomMessengerWidget::GetDisplayedMessageWidget(
	const int32 Index) const
{
	return SB_MessageList
		&& Index >= 0
		&& Index < SB_MessageList->GetChildrenCount()
		&& SB_MessageList->GetChildAt(Index)
		? Cast<UBalhwajeomMessengerMessageWidget>(SB_MessageList->GetChildAt(Index))
		: nullptr;
}

bool UBalhwajeomMessengerWidget::HasValidRoomData() const
{
	if (!LoadedMessengerData || LoadedMessengerData->Rooms.IsEmpty())
	{
		return false;
	}

	TSet<FString> SeenRoomIDs;
	for (const UBalhwajeomMessengerRoomDataAsset* Room : LoadedMessengerData->Rooms)
	{
		if (!Room
			|| Room->RoomID.IsEmpty()
			|| SeenRoomIDs.Contains(Room->RoomID)
			|| Room->Messages.IsEmpty()
			|| Room->InitialUnreadCount < 0
			|| Room->InitialUnreadCount > Room->Messages.Num())
		{
			return false;
		}
		SeenRoomIDs.Add(Room->RoomID);
	}
	return true;
}

void UBalhwajeomMessengerWidget::CreateRoomList()
{
	RoomWidgets.Reset();
	if (!SB_ChatRoomList)
	{
		return;
	}

	SB_ChatRoomList->ClearChildren();
	const TSubclassOf<UBalhwajeomMessengerRoomWidget> ResolvedClass = ResolveRoomWidgetClass();
	UWorld* World = GetWorld();
	if (!World || !ResolvedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Messenger room list could not resolve WBP_MessengerRoom."));
		return;
	}

	if (!LoadedMessengerData)
	{
		return;
	}

	for (const UBalhwajeomMessengerRoomDataAsset* Room : LoadedMessengerData->Rooms)
	{
		if (!Room || !ValidRoomIDs.Contains(Room->RoomID))
		{
			continue;
		}

		UBalhwajeomMessengerRoomWidget* RoomWidget =
			CreateWidget<UBalhwajeomMessengerRoomWidget>(World, ResolvedClass);
		if (!RoomWidget)
		{
			continue;
		}

#if WITH_EDITOR
		if (!GetOwningLocalPlayer())
		{
			RoomWidget->InitializeForAutomatedTest();
		}
#endif
		const FText LastMessagePreview = Room->Messages.Last().Message;
		RoomWidget->SetupRoom(
			Room->RoomID,
			Room->RoomName,
			LastMessagePreview,
			GetCurrentUnreadCount(Room->RoomID),
			Room->RoomID == CurrentRoomID);
		RoomWidget->OnRoomClicked.AddUniqueDynamic(this, &ThisClass::HandleRoomClicked);
		SB_ChatRoomList->AddChild(RoomWidget);
		RoomWidgets.Add(Room->RoomID, RoomWidget);
	}
}

void UBalhwajeomMessengerWidget::LoadMessages(const UBalhwajeomMessengerRoomDataAsset& Room)
{
	if (!SB_MessageList)
	{
		return;
	}

	SB_MessageList->ClearChildren();
	const TSubclassOf<UBalhwajeomMessengerMessageWidget> ResolvedClass = ResolveMessageWidgetClass();
	UWorld* World = GetWorld();
	if (!World || !ResolvedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Messenger message list could not resolve WBP_MessengerMessage."));
		return;
	}

	for (const FST_MessengerMessage& Message : Room.Messages)
	{
		UBalhwajeomMessengerMessageWidget* MessageWidget =
			CreateWidget<UBalhwajeomMessengerMessageWidget>(World, ResolvedClass);
		if (!MessageWidget)
		{
			continue;
		}
#if WITH_EDITOR
		if (!GetOwningLocalPlayer())
		{
			MessageWidget->InitializeForAutomatedTest();
		}
#endif
		MessageWidget->SetupMessage(Message);
		SB_MessageList->AddChild(MessageWidget);
	}

	if (TXT_SelectRoomPrompt)
	{
		TXT_SelectRoomPrompt->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TXT_CurrentRoomName)
	{
		TXT_CurrentRoomName->SetText(Room.RoomName);
	}
	SB_MessageList->ScrollToEnd();
}

void UBalhwajeomMessengerWidget::UpdateRoomSelection()
{
	for (const TPair<FString, TObjectPtr<UBalhwajeomMessengerRoomWidget>>& Pair : RoomWidgets)
	{
		if (Pair.Value)
		{
			Pair.Value->SetSelected(Pair.Key == CurrentRoomID);
		}
	}
}

const UBalhwajeomMessengerRoomDataAsset* UBalhwajeomMessengerWidget::FindUniqueRoom(
	const FString& RoomID) const
{
	if (RoomID.IsEmpty() || !ValidRoomIDs.Contains(RoomID) || !LoadedMessengerData)
	{
		return nullptr;
	}

	const UBalhwajeomMessengerRoomDataAsset* Match = nullptr;
	for (const UBalhwajeomMessengerRoomDataAsset* Room : LoadedMessengerData->Rooms)
	{
		if (!Room || Room->RoomID != RoomID)
		{
			continue;
		}
		if (Match)
		{
			return nullptr;
		}
		Match = Room;
	}
	return Match;
}

TSubclassOf<UBalhwajeomMessengerRoomWidget> UBalhwajeomMessengerWidget::ResolveRoomWidgetClass()
{
	if (!RoomWidgetClass)
	{
		RoomWidgetClass = LoadClass<UBalhwajeomMessengerRoomWidget>(nullptr, RoomWidgetClassPath);
	}
	return RoomWidgetClass;
}

TSubclassOf<UBalhwajeomMessengerMessageWidget> UBalhwajeomMessengerWidget::ResolveMessageWidgetClass()
{
	if (!MessageWidgetClass)
	{
		MessageWidgetClass = LoadClass<UBalhwajeomMessengerMessageWidget>(nullptr, MessageWidgetClassPath);
	}
	return MessageWidgetClass;
}

void UBalhwajeomMessengerWidget::BroadcastUnreadCount()
{
	OnTotalUnreadChanged.Broadcast(GetTotalUnreadCount());
}

void UBalhwajeomMessengerWidget::HandleRoomClicked(const FString& RoomID)
{
	SelectRoomByID(RoomID);
}

void UBalhwajeomMessengerWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}
