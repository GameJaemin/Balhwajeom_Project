#pragma once

#include "CoreMinimal.h"
#include "BalhwajeomMessengerTypes.generated.h"

/** One immutable, archived messenger entry. A message supports at most one keyword. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FST_MessengerMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	FText SenderName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger", meta = (MultiLine = "true"))
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	bool bIsPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	FText KeywordText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	FString WordID;
};

/** Designer-authored room data. Runtime read state is deliberately stored elsewhere. */
USTRUCT(BlueprintType)
struct BALHWAJEOM_API FST_MessengerRoom
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	FString RoomID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	FText RoomName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger", meta = (ClampMin = "0"))
	int32 InitialUnreadCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Messenger")
	TArray<FST_MessengerMessage> Messages;
};
