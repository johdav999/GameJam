#include "ShiftPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "WorldManager.h"
#include "WorldShiftComponent.h"

AShiftPlatform::AShiftPlatform()
{
    PrimaryActorTick.bCanEverTick = false;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;

    WorldShiftComponent = CreateDefaultSubobject<UWorldShiftComponent>(TEXT("WorldShiftComponent"));

    WorldBehaviors.Add(EWorldState::Light, EPlatformState::Solid);
    WorldBehaviors.Add(EWorldState::Shadow, EPlatformState::Ghost);
    WorldBehaviors.Add(EWorldState::Chaos, EPlatformState::TimedSolid);

    CurrentWorld = EWorldState::Light;
    bTimedSolidCurrentlySolid = false;
}

void AShiftPlatform::BeginPlay()
{
    Super::BeginPlay();

    if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
    {
        CachedWorldManager = Manager;
        Manager->OnWorldShifted.AddDynamic(this, &AShiftPlatform::HandleWorldShift);
        HandleWorldShift(Manager->GetCurrentWorld());
    }
    else
    {
        HandleWorldShift(CurrentWorld);
    }
}

void AShiftPlatform::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopTimedSolidCycle();

    if (CachedWorldManager.IsValid())
    {
        CachedWorldManager->OnWorldShifted.RemoveDynamic(this, &AShiftPlatform::HandleWorldShift);
        CachedWorldManager.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void AShiftPlatform::HandleWorldShift(EWorldState NewWorld)
{
    CurrentWorld = NewWorld;
    const EPlatformState Behavior = GetBehaviorForWorld(NewWorld);

    ApplyPlatformState(Behavior, NewWorld);
}

void AShiftPlatform::ApplyPlatformState(EPlatformState NewState, EWorldState WorldContext)
{
    switch (NewState)
    {
    case EPlatformState::Solid:
        StopTimedSolidCycle();
        ApplySolidState();
        break;
    case EPlatformState::Ghost:
        StopTimedSolidCycle();
        ApplyGhostState(WorldContext);
        break;
    case EPlatformState::Hidden:
        StopTimedSolidCycle();
        ApplyHiddenState();
        break;
    case EPlatformState::TimedSolid:
        if (WorldContext == EWorldState::Chaos)
        {
            StartTimedSolidCycle();
        }
        else
        {
            StopTimedSolidCycle();
            ApplySolidState();
        }
        break;
    default:
        StopTimedSolidCycle();
        ApplySolidState();
        break;
    }
}

void AShiftPlatform::ApplySolidState()
{
    if (!PlatformMesh)
    {
        return;
    }

   // PlatformMesh->SetHiddenInGame(false);
    PlatformMesh->SetVisibility(true, true);
    PlatformMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    if (SolidMaterial)
    {
        ApplyMaterial(SolidMaterial.Get());
    }
}

void AShiftPlatform::ApplyGhostState(EWorldState WorldContext)
{
    if (!PlatformMesh)
    {
        return;
    }

   // PlatformMesh->SetHiddenInGame(false);
   PlatformMesh->SetVisibility(true, true);
    PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (const TObjectPtr<UMaterialInterface>* MaterialPtr = GhostMaterials.Find(WorldContext))
    {
        if (UMaterialInterface* Material = MaterialPtr->Get())
        {
            ApplyMaterial(Material);
        }
    }
}

void AShiftPlatform::ApplyHiddenState()
{
    if (!PlatformMesh)
    {
        return;
    }

    PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PlatformMesh->SetVisibility(false, true);
}

void AShiftPlatform::StartTimedSolidCycle()
{
    StopTimedSolidCycle();

    bTimedSolidCurrentlySolid = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(TimedSolidTimerHandle, this, &AShiftPlatform::HandleTimedSolidToggle, 2.0f, true, 2.0f);
        HandleTimedSolidToggle();
    }
    else
    {
        ApplySolidState();
    }
}

void AShiftPlatform::StopTimedSolidCycle()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimedSolidTimerHandle);
    }

    bTimedSolidCurrentlySolid = false;
}

void AShiftPlatform::HandleTimedSolidToggle()
{
    bTimedSolidCurrentlySolid = !bTimedSolidCurrentlySolid;

    if (bTimedSolidCurrentlySolid)
    {
        ApplySolidState();
    }
    else
    {
        ApplyGhostState(CurrentWorld);
    }
}

EPlatformState AShiftPlatform::GetBehaviorForWorld(EWorldState WorldContext) const
{
    if (const EPlatformState* FoundState = WorldBehaviors.Find(WorldContext))
    {
        return *FoundState;
    }

    return EPlatformState::Solid;
}

void AShiftPlatform::ApplyMaterial(UMaterialInterface* Material)
{
    if (!PlatformMesh || !Material)
    {
        return;
    }

    const int32 MaterialCount = PlatformMesh->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
    {
        PlatformMesh->SetMaterial(MaterialIndex, Material);
    }
}

/*
Example editor setup:
- WorldBehaviors → Light: Solid, Shadow: Ghost, Chaos: TimedSolid.
- SolidMaterial → assign the opaque platform material.
- GhostMaterials → Light: light-tinted ghost, Shadow: dark-tinted ghost, Chaos: vibrant ghost hue.
*/
