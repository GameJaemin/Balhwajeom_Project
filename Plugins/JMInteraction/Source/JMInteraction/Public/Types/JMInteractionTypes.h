#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/TextLayout.h"
#include "GameplayTagContainer.h"
#include "Layout/Margin.h"
#include "JMInteractionTypes.generated.h"

class AController;

UENUM(BlueprintType)
enum class EJMInteractionState : uint8
{
    Idle,
    Targeting,
    Interacting,
    Holding,
    Disabled
};

UENUM(BlueprintType)
enum class EJMInteractionResultCode : uint8
{
    Succeeded,
    Failed,
    Disabled,
    NoTarget,
    TargetInvalid,
    NotAllowed,
    AlreadyInteracting
};

UENUM(BlueprintType)
enum class EJMInteractionDetectionMode : uint8
{
    CenterScreenTrace,
    MouseCursorTrace
};

UENUM(BlueprintType)
enum class EJMInteractionTraceMode : uint8
{
    Manual,
    OnInput,
    Timer,
    Tick
};

UENUM(BlueprintType)
enum class EJMInteractionPromptHiddenMode : uint8
{
    Collapsed,
    Hidden
};

USTRUCT(BlueprintType)
struct JMINTERACTION_API FJMInteractionPromptStyle
{
    GENERATED_BODY()

    /** Offset from the viewport center. Positive Y moves the prompt downward. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Prompt UI")
    FVector2D ScreenOffset = FVector2D(0.0, 80.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Prompt UI", meta = (ClampMin = "1", ClampMax = "128"))
    int32 FontSize = 24;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Prompt UI")
    FVector2D RenderScale = FVector2D(1.0, 1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Prompt UI")
    FMargin Padding = FMargin(12.0f, 6.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Prompt UI")
    TEnumAsByte<ETextJustify::Type> TextJustification = ETextJustify::Center;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction|Prompt UI")
    EJMInteractionPromptHiddenMode HiddenMode = EJMInteractionPromptHiddenMode::Collapsed;
};

USTRUCT(BlueprintType)
struct JMINTERACTION_API FJMInteractionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    bool bSucceeded = false;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    EJMInteractionResultCode Code = EJMInteractionResultCode::Failed;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    FText Message;

    static FJMInteractionResult Success(const FText& InMessage = FText::GetEmpty());
    static FJMInteractionResult Failure(EJMInteractionResultCode InCode, const FText& InMessage);
};

USTRUCT(BlueprintType)
struct JMINTERACTION_API FJMInteractionContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    TObjectPtr<AActor> InstigatorActor = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    TObjectPtr<AController> InstigatorController = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    FVector InteractionLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    FVector InteractionDirection = FVector::ForwardVector;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    FGameplayTag InteractionTag;

    UPROPERTY(BlueprintReadWrite, Category = "JM Interaction")
    FGameplayTagContainer InstigatorTags;
};

USTRUCT(BlueprintType)
struct JMINTERACTION_API FJMInteractionPrompt
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    FText DisplayText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    FGameplayTag InteractionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction", meta = (ClampMin = "0.0"))
    float HoldDuration = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JM Interaction")
    bool bVisible = true;
};
