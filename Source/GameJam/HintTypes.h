#pragma once

#include "CoreMinimal.h"
#include "HintTypes.generated.h"

UENUM(BlueprintType)
enum class EHintTemporalState : uint8
{
    Past,
    Present,
    Future
};

/** Data describing a single hint and its persistence across loops. */
USTRUCT(BlueprintType)
struct FHintData
{
    GENERATED_BODY()

    FHintData()
        : HintID(NAME_None)
        , bIsPersistent(false)
        , TemporalState(EHintTemporalState::Present)
        , LoopToUnlock(0)
    {
    }

    /** Unique identifier that can be referenced from code or data tables. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
    FName HintID;

    /** Text shown to the player when the hint is visible. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
    FText HintText;

    /** Whether the hint should survive across loop resets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
    bool bIsPersistent;

    /** Temporal classification for how the hint is presented to the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
    EHintTemporalState TemporalState;

    /** Loop number when the hint becomes known if it starts as a future memory. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hint")
    int32 LoopToUnlock;
};
