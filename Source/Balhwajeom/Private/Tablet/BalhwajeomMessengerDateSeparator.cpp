#include "Tablet/BalhwajeomMessengerDateSeparator.h"
#include "Components/TextBlock.h"

FText MessengerDate::TimeLabel(const FDateTime& Value)
{
	if (Value == FDateTime::MinValue())
	{
		return FText::FromString(TEXT("시간 미상"));
	}
	const int32 Hour = Value.GetHour();
	return FText::FromString(FString::Printf(TEXT("%s %d:%02d"),
		Hour < 12 ? TEXT("오전") : TEXT("오후"),
		Hour % 12 == 0 ? 12 : Hour % 12, Value.GetMinute()));
}

FText MessengerDate::DateLabel(const FDateTime& Value)
{
	if (Value == FDateTime::MinValue())
	{
		return FText::FromString(TEXT("날짜 미상"));
	}
	const TCHAR* Days[] = { TEXT("월요일"), TEXT("화요일"), TEXT("수요일"),
		TEXT("목요일"), TEXT("금요일"), TEXT("토요일"), TEXT("일요일") };
	return FText::FromString(FString::Printf(TEXT("%d년 %d월 %d일 %s"),
		Value.GetYear(), Value.GetMonth(), Value.GetDay(), Days[static_cast<int32>(Value.GetDayOfWeek())]));
}

void UBalhwajeomMessengerDateSeparator::SetupDate(const FDateTime& Value)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (TXT_Date)
	{
		TXT_Date->SetText(MessengerDate::DateLabel(Value));
	}
}
