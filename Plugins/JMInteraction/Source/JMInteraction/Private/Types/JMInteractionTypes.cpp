#include "Types/JMInteractionTypes.h"

FJMInteractionResult FJMInteractionResult::Success(const FText& InMessage)
{
    FJMInteractionResult Result;
    Result.bSucceeded = true;
    Result.Code = EJMInteractionResultCode::Succeeded;
    Result.Message = InMessage;
    return Result;
}

FJMInteractionResult FJMInteractionResult::Failure(EJMInteractionResultCode InCode, const FText& InMessage)
{
    FJMInteractionResult Result;
    Result.Code = InCode;
    Result.Message = InMessage;
    return Result;
}
