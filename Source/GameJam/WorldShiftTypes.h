#pragma once

#include "CoreMinimal.h"
#include "WorldShiftTypes.generated.h"

UENUM(BlueprintType)
enum class EWorldState : uint8
{
    Light UMETA(DisplayName = "Light"),
    Shadow UMETA(DisplayName = "Shadow"),
    Dream UMETA(DisplayName = "Dream")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShifted, EWorldState, NewWorld);
