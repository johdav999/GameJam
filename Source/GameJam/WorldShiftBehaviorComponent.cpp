#include "WorldShiftBehaviorComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "ShiftPlatform.h"
#include "WorldManager.h"

UWorldShiftBehaviorComponent::UWorldShiftBehaviorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    CurrentWorld = EWorldState::Light;
    CurrentState = EPlatformState::Solid;
}

void UWorldShiftBehaviorComponent::SetTargetMesh(UStaticMeshComponent* InMesh)
{
    TargetMesh = InMesh;
}

void UWorldShiftBehaviorComponent::SetGhostHintMesh(UStaticMeshComponent* InMesh)
{
    GhostHintMesh = InMesh;

    if (GhostHintMesh)
    {
        GhostHintMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GhostHintMesh->SetVisibility(false, true);
        GhostHintMesh->SetHiddenInGame(true);
    }
}

void UWorldShiftBehaviorComponent::ResetWorldBehaviors()
{
    WorldBehaviors.Reset();
}

void UWorldShiftBehaviorComponent::EnsureWorldBehaviorsFromPrefab(EPlatformPrefabType PrefabType)
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

void UWorldShiftBehaviorComponent::RefreshGhostHintPreview(EWorldState PreviewWorld)
{
    UpdateGhostHint(PreviewWorld);
}

void UWorldShiftBehaviorComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeFromOwner();
    BindToWorldManager();

    if (CachedWorldManager.IsValid())
    {
        CurrentWorld = CachedWorldManager->GetCurrentWorld();
        HandleWorldShift(CurrentWorld);
    }
    else
    {
        HandleWorldShift(CurrentWorld);
    }
}

void UWorldShiftBehaviorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindFromWorldManager();

    Super::EndPlay(EndPlayReason);
}

void UWorldShiftBehaviorComponent::OnRegister()
{
    Super::OnRegister();

    InitializeFromOwner();
}

void UWorldShiftBehaviorComponent::InitializeFromOwner()
{
    if (!TargetMesh)
    {
        if (const AActor* OwnerActor = GetOwner())
        {
            TargetMesh = Cast<UStaticMeshComponent>(OwnerActor->GetRootComponent());
        }
    }

    if (TargetMesh)
    {
        TargetMesh->SetVisibility(true, true);
    }
}

void UWorldShiftBehaviorComponent::BindToWorldManager()
{
    if (CachedWorldManager.IsValid())
    {
        return;
    }

    if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
    {
        CachedWorldManager = Manager;
        Manager->OnWorldShifted.AddDynamic(this, &UWorldShiftBehaviorComponent::HandleWorldShift);
        Manager->OnTimedSolidPhaseChanged.AddDynamic(this, &UWorldShiftBehaviorComponent::HandleGlobalTimedSolidPhaseChanged);
        Manager->OnTimedSolidPreWarning.AddDynamic(this, &UWorldShiftBehaviorComponent::HandleGlobalTimedSolidPreWarning);
    }
}

void UWorldShiftBehaviorComponent::UnbindFromWorldManager()
{
    if (!CachedWorldManager.IsValid())
    {
        return;
    }

    if (AWorldManager* Manager = CachedWorldManager.Get())
    {
        Manager->OnWorldShifted.RemoveDynamic(this, &UWorldShiftBehaviorComponent::HandleWorldShift);
        Manager->OnTimedSolidPhaseChanged.RemoveDynamic(this, &UWorldShiftBehaviorComponent::HandleGlobalTimedSolidPhaseChanged);
        Manager->OnTimedSolidPreWarning.RemoveDynamic(this, &UWorldShiftBehaviorComponent::HandleGlobalTimedSolidPreWarning);
    }

    CachedWorldManager.Reset();
}

void UWorldShiftBehaviorComponent::HandleWorldShift(EWorldState NewWorld)
{
    CurrentWorld = NewWorld;
    const EPlatformState NewState = GetBehaviorForWorld(NewWorld);
    ApplyPlatformState(NewState, NewWorld);
    UpdateGhostHint(NewWorld);
    CurrentState = NewState;
    OnShiftStateChanged(NewState, NewWorld);
}

void UWorldShiftBehaviorComponent::HandleGlobalTimedSolidPhaseChanged(bool bNowSolid)
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

void UWorldShiftBehaviorComponent::HandleGlobalTimedSolidPreWarning(bool bWillBeSolid)
{
    if (GetBehaviorForWorld(CurrentWorld) != EPlatformState::TimedSolid)
    {
        return;
    }

    ApplyPreWarningState();
    OnPreWarningStarted(bWillBeSolid);
}

void UWorldShiftBehaviorComponent::ApplyPlatformState(EPlatformState NewState, EWorldState WorldContext)
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
        if (CachedWorldManager.IsValid())
        {
            HandleGlobalTimedSolidPhaseChanged(CachedWorldManager->IsGlobalTimedSolidSolid());
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

void UWorldShiftBehaviorComponent::ApplySolidState()
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->SetVisibility(true, true);
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    if (SolidMaterial)
    {
        ApplyMaterial(SolidMaterial.Get());
    }
}

void UWorldShiftBehaviorComponent::ApplyGhostState(EWorldState WorldContext)
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->SetVisibility(true, true);
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (const TObjectPtr<UMaterialInterface>* MaterialPtr = GhostMaterials.Find(WorldContext))
    {
        if (UMaterialInterface* Material = MaterialPtr->Get())
        {
            ApplyMaterial(Material);
        }
    }
}

void UWorldShiftBehaviorComponent::ApplyHiddenState()
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->SetVisibility(false, true);
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UWorldShiftBehaviorComponent::ApplyPreWarningState()
{
    if (!TargetMesh)
    {
        return;
    }

    TargetMesh->SetVisibility(true, true);
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (PreWarningMaterial)
    {
        ApplyMaterial(PreWarningMaterial.Get());
    }
}

void UWorldShiftBehaviorComponent::ApplyMaterial(UMaterialInterface* Material) const
{
    if (!TargetMesh || !Material)
    {
        return;
    }

    const int32 MaterialCount = TargetMesh->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
    {
        TargetMesh->SetMaterial(MaterialIndex, Material);
    }
}

void UWorldShiftBehaviorComponent::UpdateGhostHint(EWorldState WorldContext)
{
    if (!GhostHintMesh)
    {
        return;
    }

    const EPlatformState Behavior = GetBehaviorForWorld(WorldContext);
    const bool bActiveInCurrentWorld = Behavior == EPlatformState::Solid || Behavior == EPlatformState::TimedSolid;

    if (bActiveInCurrentWorld)
    {
        GhostHintMesh->SetVisibility(false, true);
        GhostHintMesh->SetHiddenInGame(true);
        OnGhostHintUpdated();
        return;
    }

    for (const TPair<EWorldState, EPlatformState>& Pair : WorldBehaviors)
    {
        if (Pair.Key == WorldContext)
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

            OnGhostHintUpdated();
            return;
        }
    }

    GhostHintMesh->SetVisibility(false, true);
    GhostHintMesh->SetHiddenInGame(true);
    OnGhostHintUpdated();
}

EPlatformState UWorldShiftBehaviorComponent::GetBehaviorForWorld(EWorldState WorldContext) const
{
    if (const EPlatformState* FoundState = WorldBehaviors.Find(WorldContext))
    {
        return *FoundState;
    }

    return EPlatformState::Solid;
}
