#pragma once
#include "CoreMinimal.h"



UENUM(BlueprintType)
enum class EWorldState : uint8
{
    Light UMETA(DisplayName = "Light"),
    Shadow UMETA(DisplayName = "Shadow"),
    Chaos UMETA(DisplayName = "Chaos")
};





