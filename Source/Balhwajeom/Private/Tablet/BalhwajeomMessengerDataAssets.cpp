#include "Tablet/BalhwajeomMessengerDataAssets.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

namespace
{
	bool HasExactlyOneKeywordOccurrence(const FST_MessengerMessage& Message)
	{
		const FString FullMessage = Message.Message.ToString();
		const FString Keyword = Message.KeywordText.ToString();
		const bool bHasKeyword = !Keyword.IsEmpty();
		const bool bHasWordID = !Message.WordID.IsEmpty();
		if (!bHasKeyword && !bHasWordID)
		{
			return true;
		}
		if (!bHasKeyword || !bHasWordID)
		{
			return false;
		}

		const int32 FirstIndex = FullMessage.Find(Keyword, ESearchCase::CaseSensitive);
		return FirstIndex != INDEX_NONE
			&& FullMessage.Find(
				Keyword,
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				FirstIndex + Keyword.Len()) == INDEX_NONE;
	}
}

EDataValidationResult UBalhwajeomMessengerRoomDataAsset::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context),
		EDataValidationResult::Valid);

	auto AddError = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};

	if (RoomID.IsEmpty())
	{
		AddError(NSLOCTEXT("MessengerValidation", "EmptyRoomID", "RoomID must not be empty."));
	}
	if (Messages.IsEmpty())
	{
		AddError(NSLOCTEXT("MessengerValidation", "EmptyMessages", "A messenger room needs at least one message."));
	}
	if (InitialUnreadCount < 0 || InitialUnreadCount > Messages.Num())
	{
		AddError(FText::Format(
			NSLOCTEXT(
				"MessengerValidation",
				"InvalidUnreadCount",
				"InitialUnreadCount ({0}) must be between 0 and the message count ({1})."),
			FText::AsNumber(InitialUnreadCount),
			FText::AsNumber(Messages.Num())));
	}

	for (int32 Index = 0; Index < Messages.Num(); ++Index)
	{
		if (!HasExactlyOneKeywordOccurrence(Messages[Index]))
		{
			AddError(FText::Format(
				NSLOCTEXT(
					"MessengerValidation",
					"InvalidKeyword",
					"Message {0} has mismatched, missing, or repeated KeywordText/WordID data."),
				FText::AsNumber(Index)));
		}
	}
	return Result;
}

EDataValidationResult UBalhwajeomMessengerCatalogDataAsset::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context),
		EDataValidationResult::Valid);
	TSet<FString> SeenRoomIDs;

	if (Rooms.IsEmpty())
	{
		Context.AddError(NSLOCTEXT(
			"MessengerValidation",
			"EmptyCatalog",
			"The messenger catalog needs at least one room asset."));
		Result = EDataValidationResult::Invalid;
	}

	for (int32 Index = 0; Index < Rooms.Num(); ++Index)
	{
		const UBalhwajeomMessengerRoomDataAsset* Room = Rooms[Index];
		if (!Room)
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("MessengerValidation", "NullRoom", "Catalog entry {0} has no room asset."),
				FText::AsNumber(Index)));
			Result = EDataValidationResult::Invalid;
			continue;
		}
		if (Room->RoomID.IsEmpty() || SeenRoomIDs.Contains(Room->RoomID))
		{
			Context.AddError(FText::Format(
				NSLOCTEXT(
					"MessengerValidation",
					"DuplicateRoom",
					"RoomID '{0}' is empty or duplicated in the catalog."),
				FText::FromString(Room->RoomID)));
			Result = EDataValidationResult::Invalid;
		}
		SeenRoomIDs.Add(Room->RoomID);
	}
	return Result;
}
#endif
