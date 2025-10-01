#include "ShiftPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "WorldManager.h"
#include "WorldShiftComponent.h"

AShiftPlatform::AShiftPlatform()
{
    PrimaryActorTick.bCanEverTick = false;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;

    GhostHintMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostHintMesh"));
    GhostHintMesh->SetupAttachment(PlatformMesh);
    GhostHintMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GhostHintMesh->SetVisibility(false, true);
    GhostHintMesh->SetHiddenInGame(true);
    GhostHintMesh->SetRelativeTransform(FTransform::Identity);

    WorldShiftComponent = CreateDefaultSubobject<UWorldShiftComponent>(TEXT("WorldShiftComponent"));

    PrefabType = EPlatformPrefabType::LightBridge;

    CurrentWorld = EWorldState::Light;
    bTimedSolidCurrentlySolid = false;
    TimedSolidInterval = 5.0f;
    PreWarningDuration = 1.0f;
}

void AShiftPlatform::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    InitializeWorldBehaviorsFromPrefab();

    UpdateGhostHint(CurrentWorld);
}

void AShiftPlatform::BeginPlay()
{
    Super::BeginPlay();

    InitializeWorldBehaviorsFromPrefab();

    if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
    {
        CachedWorldManager = Manager;
        Manager->OnWorldShifted.AddDynamic(this, &AShiftPlatform::HandleWorldShift);
        Manager->OnTimedSolidPhaseChanged.AddDynamic(this, &AShiftPlatform::OnGlobalTimedSolidPhaseChanged);
        Manager->OnTimedSolidPreWarning.AddDynamic(this, &AShiftPlatform::OnGlobalTimedSolidPreWarning);
        HandleWorldShift(Manager->GetCurrentWorld());
    }
    else
    {
        HandleWorldShift(CurrentWorld);
    }
}

void AShiftPlatform::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CachedWorldManager.IsValid())
    {
        CachedWorldManager->OnWorldShifted.RemoveDynamic(this, &AShiftPlatform::HandleWorldShift);
        CachedWorldManager->OnTimedSolidPhaseChanged.RemoveDynamic(this, &AShiftPlatform::OnGlobalTimedSolidPhaseChanged);
        CachedWorldManager->OnTimedSolidPreWarning.RemoveDynamic(this, &AShiftPlatform::OnGlobalTimedSolidPreWarning);
        CachedWorldManager.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void AShiftPlatform::HandleWorldShift(EWorldState NewWorld)
{
    CurrentWorld = NewWorld;
    const EPlatformState Behavior = GetBehaviorForWorld(NewWorld);

    ApplyPlatformState(Behavior, NewWorld);
    UpdateGhostHint(NewWorld);
}

void AShiftPlatform::ApplyPlatformState(EPlatformState NewState, EWorldState WorldContext)
{
    switch (NewState)
    {
    case EPlatformState::Solid:
        ApplySolidState();
        break;
    case EPlatformState::Ghost:
        ApplyGhostState(WorldContext);
        break;
    case EPlatformState::Hidden:
        ApplyHiddenState();
        break;
    case EPlatformState::TimedSolid:
        if (WorldContext == EWorldState::Chaos)
        {
            if (CachedWorldManager.IsValid())
            {
                OnGlobalTimedSolidPhaseChanged(CachedWorldManager->IsGlobalTimedSolidSolid());
            }
            else
            {
                ApplySolidState();
            }
        }
        else
        {
            ApplySolidState();
        }
        break;
    default:
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

void AShiftPlatform::OnGlobalTimedSolidPhaseChanged(bool bNowSolid)
{
    if (GetBehaviorForWorld(CurrentWorld) != EPlatformState::TimedSolid)
    {
        return;
    }

    if (bNowSolid)
    {
        ApplySolidState();
    }
    else
    {
        ApplyGhostState(CurrentWorld);
    }
}

void AShiftPlatform::OnGlobalTimedSolidPreWarning(bool bWillBeSolid)
{
    if (GetBehaviorForWorld(CurrentWorld) != EPlatformState::TimedSolid)
    {
        return;
    }

    ApplyPreWarningState();
    OnPreWarningStarted(bWillBeSolid);
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

void AShiftPlatform::UpdateGhostHint(EWorldState CurrentWorldState)
{
    if (!GhostHintMesh)
    {
        return;
    }

    const EPlatformState CurrentState = GetBehaviorForWorld(CurrentWorldState);
    const bool bIsActiveInCurrentWorld = CurrentState == EPlatformState::Solid || CurrentState == EPlatformState::TimedSolid;
    if (bIsActiveInCurrentWorld)
    {
        GhostHintMesh->SetVisibility(false, true);
        GhostHintMesh->SetHiddenInGame(true);
        return;
    }

    for (const TPair<EWorldState, EPlatformState>& Pair : WorldBehaviors)
    {
        if (Pair.Key == CurrentWorldState)
        {
            continue;
        }

        if (Pair.Value == EPlatformState::Solid || Pair.Value == EPlatformState::TimedSolid)
        {
            GhostHintMesh->SetVisibility(true, true);
            GhostHintMesh->SetHiddenInGame(false);

            if (UMaterialInterface* HintMaterial = GhostHintMaterials.FindRef(Pair.Key))
            {
                GhostHintMesh->SetMaterial(0, HintMaterial);
            }

            return;
        }
    }

    GhostHintMesh->SetVisibility(false, true);
    GhostHintMesh->SetHiddenInGame(true);
}

/*
Example editor setup:
- WorldBehaviors → Light: Solid, Shadow: Ghost, Chaos: TimedSolid.
- SolidMaterial → assign the opaque platform material.
- GhostMaterials → Light: light-tinted ghost, Shadow: dark-tinted ghost, Chaos: vibrant ghost hue.
*/
