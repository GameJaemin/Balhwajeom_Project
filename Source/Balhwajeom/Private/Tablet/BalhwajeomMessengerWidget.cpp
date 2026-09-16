#include "Tablet/BalhwajeomMessengerWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	const FString DadRoomID(TEXT("Dad"));
	const FString MotherRoomID(TEXT("Mother"));
	const FString SisterRoomID(TEXT("Sister"));
	const FString BrotherRoomID(TEXT("Brother"));

	bool IsStaticRoomID(const FString& RoomID)
	{
		return RoomID == DadRoomID
			|| RoomID == MotherRoomID
			|| RoomID == SisterRoomID
			|| RoomID == BrotherRoomID;
	}

	FText RoomDisplayName(const FString& RoomID)
	{
		if (RoomID == DadRoomID) return NSLOCTEXT("Messenger", "DadRoom", "아버지");
		if (RoomID == MotherRoomID) return NSLOCTEXT("Messenger", "MotherRoom", "어머니");
		if (RoomID == SisterRoomID) return NSLOCTEXT("Messenger", "SisterRoom", "막내");
		if (RoomID == BrotherRoomID) return NSLOCTEXT("Messenger", "BrotherRoom", "형");
		return FText::GetEmpty();
	}

	float SelectionOffset(const FString& RoomID)
	{
		if (RoomID == MotherRoomID) return 58.0f;
		if (RoomID == SisterRoomID) return 116.0f;
		if (RoomID == BrotherRoomID) return 174.0f;
		return 0.0f;
	}

	/** Per-room chat background art. Paths are literal here (rather than shared constants) because
	 * TabletWidgetBlueprintLibrary's equivalents live in the editor-only BalhwajeomEditor module,
	 * which this runtime module cannot depend on. */
	UTexture2D* RoomBackgroundTexture(const FString& RoomID)
	{
		const TCHAR* Path = TEXT("/Game/Balhwajeom/UI/Tablet/Messenger/Messenger_talk_dad.Messenger_talk_dad");
		if (RoomID == MotherRoomID)
		{
			Path = TEXT("/Game/Balhwajeom/UI/Tablet/Messenger/Messenger_talk_mom.Messenger_talk_mom");
		}
		else if (RoomID == SisterRoomID)
		{
			Path = TEXT("/Game/Balhwajeom/UI/Tablet/Messenger/Messenger_talk_sis.Messenger_talk_sis");
		}
		else if (RoomID == BrotherRoomID)
		{
			Path = TEXT("/Game/Balhwajeom/UI/Tablet/Messenger/Messenger_talk_bro.Messenger_talk_bro");
		}
		return LoadObject<UTexture2D>(nullptr, Path);
	}
}

void UBalhwajeomMessengerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Back)
	{
		BTN_Back->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
	BindRoomButtons();
	InitializeMessenger();
}

void UBalhwajeomMessengerWidget::InitializeMessenger()
{
	BindRoomButtons();
	if (CurrentRoomID.IsEmpty())
	{
		CurrentRoomID = DadRoomID;
	}
	bInitialized = true;
	RefreshSelection();
	OnTotalUnreadChanged.Broadcast(0);
}

bool UBalhwajeomMessengerWidget::SelectRoomByID(const FString& RoomID)
{
	if (!IsStaticRoomID(RoomID))
	{
		return false;
	}

	CurrentRoomID = RoomID;
	RefreshSelection();
	OnRoomSelectionChanged.Broadcast(CurrentRoomID);
	return true;
}

int32 UBalhwajeomMessengerWidget::GetCurrentUnreadCount(const FString& RoomID) const
{
	(void)RoomID;
	return 0;
}

UBalhwajeomMessengerRoomWidget* UBalhwajeomMessengerWidget::GetDisplayedRoomWidget(
	const FString& RoomID) const
{
	(void)RoomID;
	return nullptr;
}

UBalhwajeomMessengerMessageWidget* UBalhwajeomMessengerWidget::GetDisplayedMessageWidget(
	const int32 Index) const
{
	(void)Index;
	return nullptr;
}

void UBalhwajeomMessengerWidget::BindRoomButtons()
{
	if (BTN_RoomDad)
	{
		BTN_RoomDad->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDadClicked);
	}
	if (BTN_RoomMother)
	{
		BTN_RoomMother->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMotherClicked);
	}
	if (BTN_RoomSister)
	{
		BTN_RoomSister->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSisterClicked);
	}
	if (BTN_RoomBrother)
	{
		BTN_RoomBrother->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBrotherClicked);
	}
}

void UBalhwajeomMessengerWidget::RefreshSelection()
{
	if (IMG_RoomSelection)
	{
		IMG_RoomSelection->SetRenderTranslation(FVector2D(0.0f, SelectionOffset(CurrentRoomID)));
	}
	if (TXT_CurrentRoomName)
	{
		TXT_CurrentRoomName->SetText(RoomDisplayName(CurrentRoomID));
	}
	if (IMG_ChatBackground)
	{
		if (UTexture2D* Background = RoomBackgroundTexture(CurrentRoomID))
		{
			IMG_ChatBackground->SetBrushFromTexture(Background, true);
		}
	}
}

void UBalhwajeomMessengerWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}

void UBalhwajeomMessengerWidget::HandleDadClicked()
{
	SelectRoomByID(DadRoomID);
}

void UBalhwajeomMessengerWidget::HandleMotherClicked()
{
	SelectRoomByID(MotherRoomID);
}

void UBalhwajeomMessengerWidget::HandleSisterClicked()
{
	SelectRoomByID(SisterRoomID);
}

void UBalhwajeomMessengerWidget::HandleBrotherClicked()
{
	SelectRoomByID(BrotherRoomID);
}
