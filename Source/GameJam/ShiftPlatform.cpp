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

    PrefabType = EPlatformPrefabType::LightBridge;

    CurrentWorld = EWorldState::Light;
    bTimedSolidCurrentlySolid = false;
    TimedSolidInterval = 2.0f;
    PreWarningDuration = 1.0f;
}

void AShiftPlatform::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    InitializeWorldBehaviorsFromPrefab();
}

void AShiftPlatform::BeginPlay()
{
    Super::BeginPlay();

    InitializeWorldBehaviorsFromPrefab();

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

void AShiftPlatform::ApplyPreWarningState()
{
    if (!PlatformMesh)
    {
        return;
    }

    PlatformMesh->SetVisibility(true, true);
    PlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (PreWarningMaterial)
    {
        ApplyMaterial(PreWarningMaterial.Get());
    }
}

void AShiftPlatform::StartTimedSolidCycle()
{
    StopTimedSolidCycle();

    bTimedSolidCurrentlySolid = false;

    if (UWorld* World = GetWorld())
    {
        const float Interval = FMath::Max(0.0f, TimedSolidInterval);
        World->GetTimerManager().SetTimer(TimedSolidTimerHandle, this, &AShiftPlatform::HandleTimedSolidToggle, Interval, true, Interval);
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
        World->GetTimerManager().ClearTimer(PreWarningTimerHandle);
    }

    bTimedSolidCurrentlySolid = false;
}

void AShiftPlatform::HandleTimedSolidToggle()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PreWarningTimerHandle);
    }

    bTimedSolidCurrentlySolid = !bTimedSolidCurrentlySolid;

    OnPreWarningStart(CurrentWorld);

    ApplyPreWarningState();

    if (UWorld* World = GetWorld())
    {
        const float WarningDuration = FMath::Max(0.0f, PreWarningDuration);
        World->GetTimerManager().SetTimer(PreWarningTimerHandle, this, &AShiftPlatform::CompleteTimedSolidToggle, WarningDuration, false);
    }
    else
    {
        CompleteTimedSolidToggle();
    }
}

void AShiftPlatform::CompleteTimedSolidToggle()
{
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

void AShiftPlatform::InitializeWorldBehaviorsFromPrefab()
{
    if (WorldBehaviors.Num() > 0)
    {
        return;
    }

    auto Configure = [this](EPlatformState LightState, EPlatformState ShadowState, EPlatformState ChaosState)
    {
        WorldBehaviors.Add(EWorldState::Light, LightState);
        WorldBehaviors.Add(EWorldState::Shadow, ShadowState);
        WorldBehaviors.Add(EWorldState::Chaos, ChaosState);
    };

    switch (PrefabType)
    {
    case EPlatformPrefabType::LightBridge:
        Configure(EPlatformState::Solid, EPlatformState::Hidden, EPlatformState::Hidden);
        break;
    case EPlatformPrefabType::LightToShadow:
        Configure(EPlatformState::Solid, EPlatformState::Ghost, EPlatformState::Hidden);
        break;
    case EPlatformPrefabType::ShadowBridge:
        Configure(EPlatformState::Hidden, EPlatformState::Solid, EPlatformState::Hidden);
        break;
    case EPlatformPrefabType::ShadowIllusion:
        Configure(EPlatformState::Solid, EPlatformState::Ghost, EPlatformState::Solid);
        break;
    case EPlatformPrefabType::ChaosFlicker:
        Configure(EPlatformState::Hidden, EPlatformState::Hidden, EPlatformState::TimedSolid);
        break;
    case EPlatformPrefabType::ChaosTrap:
        Configure(EPlatformState::Solid, EPlatformState::Solid, EPlatformState::Ghost);
        break;
    case EPlatformPrefabType::ChaosBridge:
        Configure(EPlatformState::Hidden, EPlatformState::Hidden, EPlatformState::Solid);
        break;
    case EPlatformPrefabType::ShiftChain:
        Configure(EPlatformState::Solid, EPlatformState::Solid, EPlatformState::TimedSolid);
        break;
    case EPlatformPrefabType::HiddenSurprise:
        Configure(EPlatformState::Hidden, EPlatformState::Ghost, EPlatformState::Solid);
        break;
    case EPlatformPrefabType::DeceptionPlatform:
        Configure(EPlatformState::Solid, EPlatformState::Ghost, EPlatformState::TimedSolid);
        break;
    default:
        Configure(EPlatformState::Solid, EPlatformState::Ghost, EPlatformState::TimedSolid);
        break;
    }
}

/*
Example editor setup:
- WorldBehaviors → Light: Solid, Shadow: Ghost, Chaos: TimedSolid.
- SolidMaterial → assign the opaque platform material.
- GhostMaterials → Light: light-tinted ghost, Shadow: dark-tinted ghost, Chaos: vibrant ghost hue.
*/
