#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tablet/BalhwajeomMessengerTypes.h"
#include "BalhwajeomMessengerWidget.generated.h"

class UBalhwajeomMessengerMessageWidget;
class UBalhwajeomMessengerRoomWidget;
class UBalhwajeomMessengerCatalogDataAsset;
class UBalhwajeomMessengerRoomDataAsset;
class UButton;
class UScrollBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMessengerBackRequestedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMessengerUnreadChangedSignature,
	int32,
	TotalUnreadCount);

/** Owns archived room data, runtime read state, selection, and child widget creation. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomMessengerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBalhwajeomMessengerWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void InitializeMessenger();

	/** Ignores an invalid/ambiguous ID without altering the current UI or read state. */
	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	bool SelectRoomByID(const FString& RoomID);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetCurrentUnreadCount(const FString& RoomID) const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetTotalUnreadCount() const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	const FString& GetCurrentRoomID() const { return CurrentRoomID; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool IsMessengerInitialized() const { return bInitialized; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetDisplayedRoomCount() const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetDisplayedMessageCount() const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerRoomWidget* GetDisplayedRoomWidget(const FString& RoomID) const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerMessageWidget* GetDisplayedMessageWidget(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool HasValidRoomData() const;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerCatalogDataAsset* GetMessengerDataAsset() const { return LoadedMessengerData; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerBackRequestedSignature OnBackRequested;

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerUnreadChangedSignature OnTotalUnreadChanged;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

	/** Planner-owned source data. InitialUnreadCount and messages are never mutated at runtime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Messenger")
	TSoftObjectPtr<UBalhwajeomMessengerCatalogDataAsset> MessengerData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Messenger")
	TSubclassOf<UBalhwajeomMessengerRoomWidget> RoomWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Messenger")
	TSubclassOf<UBalhwajeomMessengerMessageWidget> MessageWidgetClass;

private:
	void CreateRoomList();
	void LoadMessages(const UBalhwajeomMessengerRoomDataAsset& Room);
	void UpdateRoomSelection();
	const UBalhwajeomMessengerRoomDataAsset* FindUniqueRoom(const FString& RoomID) const;
	TSubclassOf<UBalhwajeomMessengerRoomWidget> ResolveRoomWidgetClass();
	TSubclassOf<UBalhwajeomMessengerMessageWidget> ResolveMessageWidgetClass();
	void BroadcastUnreadCount();

	UFUNCTION()
	void HandleRoomClicked(const FString& RoomID);

	UFUNCTION()
	void HandleBackClicked();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FString CurrentRoomID;

	UPROPERTY(Transient)
	TMap<FString, int32> CurrentUnreadCounts;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UBalhwajeomMessengerRoomWidget>> RoomWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomMessengerCatalogDataAsset> LoadedMessengerData;

	TSet<FString> ValidRoomIDs;
	bool bInitialized = false;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Back;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SB_ChatRoomList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> SB_MessageList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_SelectRoomPrompt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_CurrentRoomName;
};
