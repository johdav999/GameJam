#include "ShiftPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "WorldShiftBehaviorComponent.h"

AShiftPlatform::AShiftPlatform()
{
    PrimaryActorTick.bCanEverTick = false;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;

    WorldShiftBehavior = CreateDefaultSubobject<UWorldShiftBehaviorComponent>(TEXT("WorldShiftBehavior"));
    WorldShiftBehavior->SetTargetMesh(PlatformMesh);

    if (UStaticMeshComponent* GhostHint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostHintMesh")))
    {
        GhostHint->SetupAttachment(PlatformMesh);
        GhostHint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GhostHint->SetVisibility(false, true);
        GhostHint->SetHiddenInGame(true);
        GhostHint->SetRelativeTransform(FTransform::Identity);
        WorldShiftBehavior->SetGhostHintMesh(GhostHint);
    }

    PrefabType = EPlatformPrefabType::LightBridge;
}

void AShiftPlatform::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->SetTargetMesh(PlatformMesh);
        WorldShiftBehavior->EnsureWorldBehaviorsFromPrefab(PrefabType);
        WorldShiftBehavior->RefreshGhostHintPreview(WorldShiftBehavior->CurrentWorld);
    }
}

/*
Example editor setup:
- WorldBehaviors → Light: Solid, Shadow: Ghost, Chaos: TimedSolid.
- SolidMaterial → assign the opaque platform material.
- GhostMaterials → Light: light-tinted ghost, Shadow: dark-tinted ghost, Chaos: vibrant ghost hue.
*/
