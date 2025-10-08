#include "WorldDoor.h"

#include "ShiftPlatform.h"
#include "WorldManager.h"
#include "WorldShiftBehaviorComponent.h"
#include "Engine/CollisionProfile.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AWorldDoor::AWorldDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;

    WorldShiftBehavior = CreateDefaultSubobject<UWorldShiftBehaviorComponent>(TEXT("WorldShiftBehavior"));
    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->SetTargetMesh(DoorMesh);
    }

    DoorMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    DoorMesh->SetGenerateOverlapEvents(false);

    bAnimateOnToggle = true;
    bIsCurrentlySolid = true;
    bHasInitialized = false;
    CachedWorldState = EWorldState::Light;
    InitialDoorRotation = FRotator::ZeroRotator;

    SolidInWorlds = {EWorldState::Light};
}

void AWorldDoor::BeginPlay()
{
    Super::BeginPlay();

    if (DoorMesh)
    {
        InitialDoorRotation = DoorMesh->GetRelativeRotation();
    }

    InitializeWorldBehaviors();

    if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
    {
        CachedWorldManager = Manager;
        Manager->OnWorldShifted.AddDynamic(this, &AWorldDoor::HandleWorldShift);
        HandleWorldShift(Manager->GetCurrentWorld());
    }
    else
    {
        SetDoorState(IsSolidInWorld(EWorldState::Light), EWorldState::Light);
    }
}

void AWorldDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CachedWorldManager.IsValid())
    {
        if (AWorldManager* Manager = CachedWorldManager.Get())
        {
            Manager->OnWorldShifted.RemoveDynamic(this, &AWorldDoor::HandleWorldShift);
        }

        CachedWorldManager.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void AWorldDoor::HandleWorldShift(EWorldState NewWorld)
{
    SetDoorState(IsSolidInWorld(NewWorld), NewWorld);
}

void AWorldDoor::SetDoorState(bool bShouldBeSolid, EWorldState CurrentWorld)
{
    const bool bStateChanged = bIsCurrentlySolid != bShouldBeSolid;
    const bool bWorldChanged = CachedWorldState != CurrentWorld;
    CachedWorldState = CurrentWorld;

    if (bHasInitialized && !bStateChanged && !bWorldChanged)
    {
        return;
    }

    bHasInitialized = true;
    bIsCurrentlySolid = bShouldBeSolid;

    if (DoorMesh)
    {
        if (bShouldBeSolid)
        {
            DoorMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            DoorMesh->SetHiddenInGame(false);
            DoorMesh->SetVisibility(true, true);
        }
        else
        {
            DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            DoorMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

            EPlatformState EffectiveState = EPlatformState::Ghost;
            if (WorldShiftBehavior)
            {
                if (const EPlatformState* Behavior = WorldShiftBehavior->WorldBehaviors.Find(CurrentWorld))
                {
                    EffectiveState = *Behavior;
                }
                else
                {
                    EffectiveState = WorldShiftBehavior->CurrentState;
                }
            }

            const bool bHideDoor = EffectiveState == EPlatformState::Hidden;
            DoorMesh->SetHiddenInGame(bHideDoor);
            DoorMesh->SetVisibility(!bHideDoor, true);
        }
    }

    if (bAnimateOnToggle && bStateChanged)
    {
        PlayDoorAnimation(!bShouldBeSolid);
    }

    if (!bShouldBeSolid && bStateChanged && OpenSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
    }
    else if (bShouldBeSolid && bStateChanged && CloseSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, CloseSound, GetActorLocation());
    }
}

void AWorldDoor::PlayDoorAnimation(bool bOpening)
{
    if (!DoorMesh)
    {
        return;
    }

    const FRotator TargetRotation = bOpening ? (InitialDoorRotation + FRotator(0.f, 90.f, 0.f)) : InitialDoorRotation;
    DoorMesh->SetRelativeRotation(TargetRotation);
}

bool AWorldDoor::IsSolidInWorld(EWorldState World) const
{
    if (WorldShiftBehavior)
    {
        if (const EPlatformState* FoundState = WorldShiftBehavior->WorldBehaviors.Find(World))
        {
            return *FoundState == EPlatformState::Solid || *FoundState == EPlatformState::TimedSolid;
        }
    }

    return SolidInWorlds.Contains(World);
}

void AWorldDoor::InitializeWorldBehaviors()
{
    if (!WorldShiftBehavior)
    {
        return;
    }

    WorldShiftBehavior->SetTargetMesh(DoorMesh);

    if (WorldShiftBehavior->WorldBehaviors.Num() > 0)
    {
        return;
    }

    const TArray<EWorldState> AllWorlds = {EWorldState::Light, EWorldState::Shadow, EWorldState::Chaos};
    for (EWorldState World : AllWorlds)
    {
        const bool bSolid = SolidInWorlds.Contains(World);
        WorldShiftBehavior->WorldBehaviors.Add(World, bSolid ? EPlatformState::Solid : EPlatformState::Ghost);
    }
}
