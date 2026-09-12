#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tablet/BalhwajeomMessengerTypes.h"
#include "BalhwajeomMessengerMessageWidget.generated.h"

class UBalhwajeomMessengerKeywordWidget;
class UBorder;
class USpacer;
class UTextBlock;
class UWrapBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FMessengerMessageKeywordClickedSignature,
	FName, WordID,
	FName, MessageID);

/** Renders one archived message and safely falls back to plain text for invalid keyword data. */
UCLASS()
class BALHWAJEOM_API UBalhwajeomMessengerMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetupMessage(const FST_MessengerMessage& InMessageData);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool HasInteractiveKeyword() const { return bHasInteractiveKeyword; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	bool IsPlayerMessage() const { return MessageData.bIsPlayer; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	FText GetDisplayedMessage() const { return MessageData.Message; }

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	FName GetKeywordWordID() const { return FName(*MessageData.WordID); }

	UFUNCTION(BlueprintCallable, Category = "Tablet|Messenger")
	void SetKeywordAcquired(bool bAcquired);

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	UBalhwajeomMessengerKeywordWidget* GetActiveKeywordWidget() const { return ActiveKeywordWidget; }

	UPROPERTY(BlueprintAssignable, Category = "Tablet|Messenger")
	FMessengerMessageKeywordClickedSignature OnKeywordClicked;

	UFUNCTION(BlueprintPure, Category = "Tablet|Messenger")
	static bool IsKeywordDataValid(const FST_MessengerMessage& InMessageData);

#if WITH_EDITOR
	void InitializeForAutomatedTest() { NativeOnInitialized(); }
#endif

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tablet|Messenger")
	TSubclassOf<UBalhwajeomMessengerKeywordWidget> KeywordWidgetClass;

	/** Font and size used by runtime-created message body text segments. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tablet|Messenger|Style")
	FSlateFontInfo MessageTextFont;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_TimeLeft;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_TimeRight;

	void RenderMessage();
	void AddTextSegment(const FString& Segment);
	TSubclassOf<UBalhwajeomMessengerKeywordWidget> ResolveKeywordWidgetClass();

	UFUNCTION()
	void HandleKeywordClicked(const FString& WordID);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	FST_MessengerMessage MessageData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tablet|Messenger", meta = (AllowPrivateAccess = "true"))
	bool bHasInteractiveKeyword = false;

	UPROPERTY(Transient)
	TObjectPtr<UBalhwajeomMessengerKeywordWidget> ActiveKeywordWidget;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_SenderName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpacer> Spacer_Left;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpacer> Spacer_Right;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BRD_Bubble;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_MessageContent;
};
