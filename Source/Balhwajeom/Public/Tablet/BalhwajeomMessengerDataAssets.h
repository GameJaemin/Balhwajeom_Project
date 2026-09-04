#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Tablet/BalhwajeomMessengerTypes.h"
#include "BalhwajeomMessengerDataAssets.generated.h"

/** One planner-owned conversation room. Runtime read state is never written back here. */
UCLASS(BlueprintType)
class BALHWAJEOM_API UBalhwajeomMessengerRoomDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Messenger")
	FString RoomID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Messenger")
	FText RoomName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Messenger", meta = (ClampMin = "0"))
	int32 InitialUnreadCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Messenger", meta = (TitleProperty = "Message"))
	TArray<FST_MessengerMessage> Messages;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/** Ordered room catalog assigned once to WBP_Messenger. */
UCLASS(BlueprintType)
class BALHWAJEOM_API UBalhwajeomMessengerCatalogDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Messenger")
	TArray<TObjectPtr<UBalhwajeomMessengerRoomDataAsset>> Rooms;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
