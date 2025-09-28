#include "ShiftPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "WorldShiftComponent.h"

AShiftPlatform::AShiftPlatform()
{
    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;

    WorldShiftComponent = CreateDefaultSubobject<UWorldShiftComponent>(TEXT("WorldShiftComponent"));
}
