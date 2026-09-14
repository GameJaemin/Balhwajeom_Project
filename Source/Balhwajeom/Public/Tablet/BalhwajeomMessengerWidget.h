#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMessengerWidget.generated.h"

class UBalhwajeomMessengerCatalogDataAsset;
class UBalhwajeomMessengerMessageWidget;
class UBalhwajeomMessengerRoomWidget;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMessengerBackRequestedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMessengerUnreadChangedSignature,
	int32,
	TotalUnreadCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMessengerRoomSelectionChangedSignature,
	const FString&,
	RoomID);

/**
 * Presentation-only messenger page.
 *
 * Conversation content is intentionally empty until the final script is ready.
 * The widget only keeps the selected room and the tablet back-navigation contract.
 */
UCLASS()
class BALHWAJEOM_API UBalhwajeomMessengerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void InitializeMessenger();

	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	bool SelectRoomByID(const FString& RoomID);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	const FString& GetCurrentRoomID() const { return CurrentRoomID; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool IsMessengerInitialized() const { return bInitialized; }

	/** Compatibility helpers retained for WBP_Tablet while unread/data-driven UI is removed. */
	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetCurrentUnreadCount(const FString& RoomID) const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetTotalUnreadCount() const { return 0; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetDisplayedRoomCount() const { return 4; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetDisplayedMessageCount() const { return 0; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerRoomWidget* GetDisplayedRoomWidget(const FString& RoomID) const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerMessageWidget* GetDisplayedMessageWidget(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool HasValidRoomData() const { return true; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerCatalogDataAsset* GetMessengerDataAsset() const { return nullptr; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerBackRequestedSignature OnBackRequested;

	/** Always reports zero; retained so the parent tablet can clear its legacy badge. */
	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerUnreadChangedSignature OnTotalUnreadChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerRoomSelectionChangedSignature OnRoomSelectionChanged;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

private:
	void BindRoomButtons();
	void RefreshSelection();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleDadClicked();

	UFUNCTION()
	void HandleMotherClicked();

	UFUNCTION()
	void HandleSisterClicked();

	UFUNCTION()
	void HandleBrotherClicked();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FString CurrentRoomID;

	bool bInitialized = false;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Back;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_RoomDad;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_RoomMother;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_RoomSister;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_RoomBrother;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IMG_RoomSelection;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_CurrentRoomName;
};
