#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BalhwajeomMessengerDateSeparator.generated.h"

class UTextBlock;

namespace MessengerDate
{
	BALHWAJEOM_API FText TimeLabel(const FDateTime& Value);
	BALHWAJEOM_API FText DateLabel(const FDateTime& Value);
}

UCLASS()
class BALHWAJEOM_API UBalhwajeomMessengerDateSeparator : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetupDate(const FDateTime& Value);
private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_Date;
};
