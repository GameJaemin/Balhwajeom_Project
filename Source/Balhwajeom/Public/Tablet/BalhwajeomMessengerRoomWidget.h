#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMessengerRoomWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FMessengerRoomClickedSignature,
	const FString&,
	RoomID);

/** Presentation-only room row. The messenger owns all mutable read state. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomMessengerRoomWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetupRoom(
		const FString& InRoomID,
		const FText& InRoomName,
		const FText& InLastMessagePreview,
		int32 InUnreadCount,
		bool bInIsSelected);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetUnreadCount(int32 InUnreadCount);

	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetSelected(bool bInIsSelected);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	const FString& GetRoomID() const { return RoomID; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	int32 GetUnreadCount() const { return UnreadCount; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool IsSelected() const { return bIsSelected; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	FText GetLastMessagePreview() const { return LastMessagePreview; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerRoomClickedSignature OnRoomClicked;

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

private:
	void RefreshVisualState();

	UFUNCTION()
	void HandleRoomButtonClicked();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FString RoomID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FText RoomName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FText LastMessagePreview;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	int32 UnreadCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	bool bIsSelected = false;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BTN_Room;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_Selected;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_UnreadBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_RoomName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_LastMessage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_UnreadCount;
};
