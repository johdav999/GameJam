#include "WorldDoor.h"

#include "ShiftPlatform.h"
#include "WorldManager.h"
#include "WorldShiftBehaviorComponent.h"
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

    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    DoorMesh->SetGenerateOverlapEvents(false);

    bAnimateOnToggle = true;
    bIsCurrentlySolid = true;
    bHasInitialized = false;
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
        SetDoorState(IsSolidInWorld(EWorldState::Light));
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
    SetDoorState(IsSolidInWorld(NewWorld));
}

void AWorldDoor::SetDoorState(bool bShouldBeSolid)
{
    if (bHasInitialized && bIsCurrentlySolid == bShouldBeSolid)
    {
        return;
    }

    bHasInitialized = true;
    bIsCurrentlySolid = bShouldBeSolid;

    if (bAnimateOnToggle)
    {
        PlayDoorAnimation(!bShouldBeSolid);
    }

    if (!bShouldBeSolid && OpenSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
    }
    else if (bShouldBeSolid && CloseSound)
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
