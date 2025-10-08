#pragma once

#include "CoreMinimal.h"
#include "WorldShiftTypes.h"
#include "WorldState.generated.h"

UENUM(BlueprintType)
enum class EWorldMaterialState : uint8
{
    Solid UMETA(DisplayName = "Solid"),
    Ghost UMETA(DisplayName = "Ghost"),
    Hidden UMETA(DisplayName = "Hidden")
};

